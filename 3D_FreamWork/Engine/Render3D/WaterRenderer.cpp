#include "WaterRenderer.h"
#include "../Core/GameObject.h"
#include "../Render/Graphics.h"
#include "../Render/Image.h"
#include "Lighting.h"
#include "Mesh.h"
#include "Shadow.h"
#include "RenderStats.h"
#include "../Core/Time.h"
#include "../Render/Visibility.h"
#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

namespace {
    // ─── 波の式(Shader3DWater.hlsl の EvalWaves と同じ式。片方を変えたら、もう片方も直す) ───
    // 波は4本。一番大きい波(設定の値そのまま)を基準に、少しずつ細かく・弱く・向きをずらして重ねる
    constexpr float ampRatio[waterWaveCount] = { 0.50f, 0.25f, 0.15f, 0.10f };      // 振幅の割合(合計1。waveHeight に掛ける)
    constexpr float lengthRatio[waterWaveCount] = { 1.00f, 0.62f, 0.37f, 0.23f };   // 波の長さの割合
    constexpr float angleDegrees[waterWaveCount] = { 0.f, 25.f, -40.f, 65.f };      // waveDirection からの向きのずれ(度)
    constexpr int maxCells = 512;

    struct Wave {
        float dirX, dirZ;     // 進む向き(長さ1)
        float amplitude;      // 振幅
        float frequency;      // 周波数(2π ÷ 波長)
        float speed;          // 進む速さ(1秒に進む距離)
    };

    // 設定から4本の波を作る。範囲外の値は丸める(波の長さ <= 0 や向き (0,0) で壊れない)
    void BuildWaves(const WaterSettings& s, Wave out[waterWaveCount]) {
        float dx = s.waveDirection.x, dz = s.waveDirection.y;
        float length = sqrtf(dx * dx + dz * dz);
        if (length < 1e-6f) { dx = 1.f; dz = 0.f; length = 1.f; }
        dx /= length;
        dz /= length;
        float baseLength = (std::max)(s.waveLength, 1.f);

        for (int i = 0; i < waterWaveCount; i++) {
            float a = XMConvertToRadians(angleDegrees[i]);
            float c = cosf(a), sn = sinf(a);
            float waveLength = baseLength * lengthRatio[i];
            out[i].dirX = dx * c - dz * sn;
            out[i].dirZ = dx * sn + dz * c;
            out[i].amplitude = s.waveHeight * ampRatio[i];
            out[i].frequency = XM_2PI / waveLength;
            out[i].speed = s.waveSpeed * sqrtf(lengthRatio[i]);   // 長い波ほど速い(深い水の波と同じ向きの関係)
        }
    }

    // 時間 time での、波ごとの位相(0〜2π)。double で求めるので、何時間遊んでも精度が落ちない
    void BuildPhases(const Wave waves[waterWaveCount], float time, float out[waterWaveCount]) {
        const double twoPi = 6.283185307179586476925286766559;
        for (int i = 0; i < waterWaveCount; i++) {
            double phase = fmod((double)waves[i].frequency * (double)waves[i].speed * (double)time, twoPi);
            if (phase < 0.0) phase += twoPi;
            out[i] = (float)phase;
        }
    }

    // ワールドのxzでの波の高さ(戻り値)。grad が nullptr でなければ、高さの傾き(x方向とz方向の微分)も求める
    float EvalWaves(const Wave waves[waterWaveCount], const float phases[waterWaveCount], float x, float z, XMFLOAT2* grad) {
        float h = 0.f, gx = 0.f, gz = 0.f;
        for (int i = 0; i < waterWaveCount; i++) {
            const Wave& w = waves[i];
            float phase = (w.dirX * x + w.dirZ * z) * w.frequency - phases[i];
            h += w.amplitude * sinf(phase);
            float slope = w.amplitude * w.frequency * cosf(phase);
            gx += w.dirX * slope;
            gz += w.dirZ * slope;
        }
        if (grad) *grad = { gx, gz };
        return h;
    }

    int ClampCells(int cells) {
        return (std::min)((std::max)(cells, 1), maxCells);
    }

    XMFLOAT2 ClampSize(const XMFLOAT2& size) {
        return { (std::max)(size.x, 1.f), (std::max)(size.y, 1.f) };
    }

    // ─── 全部の WaterRenderer で共有するもの(Mesh と同じ。最後の1体が手放した時に解放する) ───
    struct Shared {
        ID3D11VertexShader* vertexShader = nullptr;
        ID3D11PixelShader* pixelShader = nullptr;
        ID3D11InputLayout* inputLayout = nullptr;
        ID3D11BlendState* blendState = nullptr;               // 色 + 背景 × (1 - α)(色はαを掛けた形で出力する)
        ID3D11DepthStencilState* depthState = nullptr;        // 深度テストはする・書き込みはしない
        ID3D11RasterizerState* rasterizerState = nullptr;     // 両面を描く
        ID3D11Buffer* meshBuffer = nullptr;                   // b0: MeshConstantBuffer
        ID3D11Buffer* cameraBuffer = nullptr;                 // b3: MeshCameraBuffer
        ID3D11Buffer* waterBuffer = nullptr;                  // b6: WaterConstantBuffer
        int refCount = 0;
    };
    Shared shared;

    void ReleaseShared() {
        if (shared.waterBuffer) { shared.waterBuffer->Release(); shared.waterBuffer = nullptr; }
        if (shared.cameraBuffer) { shared.cameraBuffer->Release(); shared.cameraBuffer = nullptr; }
        if (shared.meshBuffer) { shared.meshBuffer->Release(); shared.meshBuffer = nullptr; }
        if (shared.rasterizerState) { shared.rasterizerState->Release(); shared.rasterizerState = nullptr; }
        if (shared.depthState) { shared.depthState->Release(); shared.depthState = nullptr; }
        if (shared.blendState) { shared.blendState->Release(); shared.blendState = nullptr; }
        if (shared.inputLayout) { shared.inputLayout->Release(); shared.inputLayout = nullptr; }
        if (shared.pixelShader) { shared.pixelShader->Release(); shared.pixelShader = nullptr; }
        if (shared.vertexShader) { shared.vertexShader->Release(); shared.vertexShader = nullptr; }
    }

    bool CompileShader(const char* entry, const char* target, ID3DBlob** blob) {
        ID3DBlob* err = nullptr;
        // Shader3DCommon.hlsli を #include しているので、D3D_COMPILE_STANDARD_FILE_INCLUDE を渡す
        HRESULT hr = D3DCompileFromFile(L"Shader3DWater.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entry, target, 0, 0, blob, &err);
        if (FAILED(hr)) {
            if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
            else { OutputDebugStringA("★ Shader3DWater.hlsl が見つかりません\n"); }
            return false;
        }
        if (err) err->Release();
        return true;
    }

    bool CreateShared() {
        if (shared.vertexShader) return true;   // 他の水面が作成済み

        ID3D11Device* device = Graphics::device;
        if (!device) return false;

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        if (!CompileShader("VS", "vs_5_0", &vsBlob)) return false;
        if (!CompileShader("PS", "ps_5_0", &psBlob)) { vsBlob->Release(); return false; }

        bool ok = SUCCEEDED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &shared.vertexShader)) &&
                  SUCCEEDED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &shared.pixelShader));
        if (ok) {
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            ok = SUCCEEDED(device->CreateInputLayout(layout, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &shared.inputLayout));
        }
        vsBlob->Release();
        psBlob->Release();

        // ブレンド: 出力は「αを掛けた色」なので、色はそのまま足し(ONE)、背景は (1 - α) 倍に薄める。きらめきがαで薄まらない
        if (ok) {
            D3D11_BLEND_DESC bd = {};
            bd.RenderTarget[0].BlendEnable = TRUE;
            bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            ok = SUCCEEDED(device->CreateBlendState(&bd, &shared.blendState));
        }
        // 深度: テストはする(手前の物に隠される)が、書き込みはしない(水の中の物を隠さない)
        if (ok) {
            D3D11_DEPTH_STENCIL_DESC dsd = {};
            dsd.DepthEnable = TRUE;
            dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            dsd.DepthFunc = D3D11_COMPARISON_LESS;
            ok = SUCCEEDED(device->CreateDepthStencilState(&dsd, &shared.depthState));
        }
        // 両面を描く(水の中から見上げても水面が見える)
        if (ok) {
            D3D11_RASTERIZER_DESC rd = {};
            rd.FillMode = D3D11_FILL_SOLID;
            rd.CullMode = D3D11_CULL_NONE;
            rd.DepthClipEnable = TRUE;
            ok = SUCCEEDED(device->CreateRasterizerState(&rd, &shared.rasterizerState));
        }
        if (ok) {
            D3D11_BUFFER_DESC cbd = {};
            cbd.Usage = D3D11_USAGE_DYNAMIC;
            cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            cbd.ByteWidth = sizeof(MeshConstantBuffer);
            ok = SUCCEEDED(device->CreateBuffer(&cbd, nullptr, &shared.meshBuffer));
            cbd.ByteWidth = sizeof(MeshCameraBuffer);
            ok = ok && SUCCEEDED(device->CreateBuffer(&cbd, nullptr, &shared.cameraBuffer));
            cbd.ByteWidth = sizeof(WaterConstantBuffer);
            ok = ok && SUCCEEDED(device->CreateBuffer(&cbd, nullptr, &shared.waterBuffer));
        }

        if (!ok) {
            OutputDebugStringA("★ WaterRenderer: シェーダー・ステートの作成に失敗\n");
            ReleaseShared();
            return false;
        }
        return true;
    }

    // 動的な定数バッファに内容を書き込む
    void Upload(ID3D11DeviceContext* context, ID3D11Buffer* buffer, const void* data, size_t size) {
        D3D11_MAPPED_SUBRESOURCE msr;
        if (FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) return;
        memcpy(msr.pData, data, size);
        context->Unmap(buffer, 0);
    }

    // オーナーのワールド座標(親がいる時も正しいよう、ワールド行列の位置を使う)
    XMFLOAT3 GetOwnerPosition(const Component& component) {
        XMFLOAT3 position = { 0.f, 0.f, 0.f };
        if (component.GetOwner()) XMStoreFloat3(&position, component.GetOwner()->transform.GetWorldMatrix().r[3]);
        return position;
    }
}

WaterRenderer::WaterRenderer(const WaterSettings& value) : settings(value) {
    if (!CreateShared()) return;
    shared.refCount++;
    countedInShared = true;
    if (!BuildGrid(ClampCells(settings.cells))) {
        OutputDebugStringA("★ WaterRenderer: 格子の作成に失敗\n");
        Release();
        return;
    }
    ready = true;
}

WaterRenderer::~WaterRenderer() {
    Release();   // Uninit が呼ばれずに壊されても、共有物の数が合うように
}

// 単位の格子(-0.5〜0.5)を作り、頂点とインデックス(32ビット)を1回だけ送る(変更不可)。失敗したら今のバッファはそのまま
bool WaterRenderer::BuildGrid(int cells) {
    if (cells != settings.cells) OutputDebugStringA("★ WaterRenderer: cells は 1から512の範囲に丸めました\n");

    int count = cells + 1;
    std::vector<XMFLOAT2> vertices;
    vertices.reserve((size_t)count * count);
    for (int j = 0; j < count; j++) {
        for (int i = 0; i < count; i++) {
            vertices.push_back({ (float)i / cells - 0.5f, (float)j / cells - 0.5f });
        }
    }
    std::vector<UINT> indices;
    indices.reserve((size_t)cells * cells * 6);
    for (int j = 0; j < cells; j++) {
        for (int i = 0; i < cells; i++) {
            UINT v00 = (UINT)(j * count + i), v10 = v00 + 1, v01 = v00 + count, v11 = v01 + 1;
            indices.insert(indices.end(), { v00, v01, v11, v00, v11, v10 });
        }
    }

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = (UINT)(sizeof(XMFLOAT2) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vdata = {};
    vdata.pSysMem = vertices.data();
    ID3D11Buffer* newVertexBuffer = nullptr;
    if (FAILED(Graphics::device->CreateBuffer(&vbd, &vdata, &newVertexBuffer))) return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = (UINT)(sizeof(UINT) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA idata = {};
    idata.pSysMem = indices.data();
    ID3D11Buffer* newIndexBuffer = nullptr;
    if (FAILED(Graphics::device->CreateBuffer(&ibd, &idata, &newIndexBuffer))) {
        newVertexBuffer->Release();
        return false;
    }

    // 作り直しの時に前のバッファが残らないよう、後から手放す
    if (vertexBuffer) vertexBuffer->Release();
    if (indexBuffer) indexBuffer->Release();
    vertexBuffer = newVertexBuffer;
    indexBuffer = newIndexBuffer;
    indexCount = (UINT)indices.size();
    builtCells = cells;
    return true;
}

// 自分の分のバッファを手放し、共有物の数を1つ減らす(最後の1体なら共有物も解放する)。何回呼んでもよい
void WaterRenderer::Release() {
    if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
    if (indexBuffer) { indexBuffer->Release(); indexBuffer = nullptr; }
    indexCount = 0;
    builtCells = 0;
    ready = false;
    if (countedInShared) {
        countedInShared = false;
        shared.refCount--;
        if (shared.refCount <= 0) {
            ReleaseShared();
            shared.refCount = 0;
        }
    }
}

float WaterRenderer::GetHeightAt(float worldX, float worldZ) const {
    return GetHeightAt(worldX, worldZ, Time::GetTime());
}

float WaterRenderer::GetHeightAt(float worldX, float worldZ, float time) const {
    Wave waves[waterWaveCount];
    float phases[waterWaveCount];
    BuildWaves(settings, waves);
    BuildPhases(waves, time, phases);
    return GetOwnerPosition(*this).y + EvalWaves(waves, phases, worldX, worldZ, nullptr);
}

bool WaterRenderer::Contains(float worldX, float worldZ) const {
    XMFLOAT3 center = GetOwnerPosition(*this);
    XMFLOAT2 size = ClampSize(settings.size);
    return fabsf(worldX - center.x) <= size.x * 0.5f && fabsf(worldZ - center.z) <= size.y * 0.5f;
}

void WaterRenderer::Draw() {
    if (!ready) return;
    ID3D11DeviceContext* context = Graphics::context;
    if (!context) return;

    // 格子の分割数が変わっていたら作り直す(失敗したら、今の格子のまま描く)
    int cells = ClampCells(settings.cells);
    if (cells != builtCells && !BuildGrid(cells)) {
        OutputDebugStringA("★ WaterRenderer: 格子の作り直しに失敗(今の分割数のまま描きます)\n");
        settings.cells = builtCells;
    }

    XMFLOAT2 size = ClampSize(settings.size);
    XMFLOAT3 center = GetOwnerPosition(*this);

    // 画面の外なら描かない: 水面の箱(波の高さの余裕込み)をすっぽり囲む球で調べる
    if (frustumCulling) {
        float halfY = fabsf(settings.waveHeight);
        float radius = sqrtf(size.x * size.x * 0.25f + size.y * size.y * 0.25f + halfY * halfY);
        if (!Visibility::IsSphereInView3D(center, radius)) {
            Visibility::CountCulled();
            return;
        }
    }

    // 波: 時間(Time::GetTime。ポーズ中は止まる)から、波ごとの位相を求めて送る
    Wave waves[waterWaveCount];
    float phases[waterWaveCount];
    BuildWaves(settings, waves);
    BuildPhases(waves, Time::GetTime(), phases);

    const Camera& camera = Image::GetCamera3D();
    const Light& light = Image::GetLight();
    float aspectRatio = (Graphics::height > 0.f) ? (Graphics::width / Graphics::height) : 1.f;

    // b0: ワールド(オーナーの位置だけ)・ビュー・射影・平行光源(Mesh と同じ形)
    MeshConstantBuffer cb;
    cb.world = XMMatrixTranspose(XMMatrixTranslation(center.x, center.y, center.z));
    cb.view = XMMatrixTranspose(camera.GetViewMatrix());
    cb.proj = XMMatrixTranspose(camera.GetProjectionMatrix(aspectRatio));
    cb.lightDir = { light.direction.x, light.direction.y, light.direction.z, 0.f };
    cb.lightColor = { light.color.x, light.color.y, light.color.z, 0.f };
    cb.ambient = { light.ambient.x, light.ambient.y, light.ambient.z, 0.f };
    Upload(context, shared.meshBuffer, &cb, sizeof(cb));

    // b3: カメラの位置
    MeshCameraBuffer eye = { { camera.position.x, camera.position.y, camera.position.z, 1.f } };
    Upload(context, shared.cameraBuffer, &eye, sizeof(eye));

    // b6: 水面の設定
    WaterConstantBuffer wb;
    wb.color = settings.color;
    wb.sky = { settings.skyColor.x, settings.skyColor.y, settings.skyColor.z, settings.receiveShadow ? 1.f : 0.f };
    wb.params = { settings.specularStrength, settings.fresnelPower, settings.shininess, 0.f };
    wb.size = { size.x, size.y, 0.f, 0.f };
    for (int i = 0; i < waterWaveCount; i++) {
        wb.waves[i] = { waves[i].dirX, waves[i].dirZ, waves[i].amplitude, waves[i].frequency };
    }
    wb.phases = { phases[0], phases[1], phases[2], phases[3] };
    Upload(context, shared.waterBuffer, &wb, sizeof(wb));

    // 描画状態(ブレンド・深度・ラスタライザ)を覚えておいて、描いた後に必ず元へ戻す
    ID3D11BlendState* oldBlend = nullptr;
    FLOAT oldBlendFactor[4] = {};
    UINT oldSampleMask = 0xFFFFFFFF;
    context->OMGetBlendState(&oldBlend, oldBlendFactor, &oldSampleMask);
    ID3D11DepthStencilState* oldDepth = nullptr;
    UINT oldStencilRef = 0;
    context->OMGetDepthStencilState(&oldDepth, &oldStencilRef);
    ID3D11RasterizerState* oldRasterizer = nullptr;
    context->RSGetState(&oldRasterizer);

    float blendFactor[4] = {};
    context->OMSetBlendState(shared.blendState, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(shared.depthState, 0);
    context->RSSetState(shared.rasterizerState);

    UINT stride = sizeof(XMFLOAT2), offset = 0;
    context->IASetInputLayout(shared.inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(shared.vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &shared.meshBuffer);
    context->VSSetConstantBuffers(6, 1, &shared.waterBuffer);
    context->PSSetShader(shared.pixelShader, nullptr, 0);
    context->PSSetConstantBuffers(0, 1, &shared.meshBuffer);
    context->PSSetConstantBuffers(3, 1, &shared.cameraBuffer);
    context->PSSetConstantBuffers(6, 1, &shared.waterBuffer);
    Lighting::Bind();   // b4: 点光源・スポットライト
    Shadow::Bind();     // b5 / t1 / t2 / s1: 影(影を受ける設定の時だけ使われる)

    RenderStats::AddDraw((int)(indexCount / 3));
    context->DrawIndexed(indexCount, 0, 0);

    // 水面だけが使った b6 を外し、描画状態を元へ戻す
    ID3D11Buffer* noBuffer = nullptr;
    context->VSSetConstantBuffers(6, 1, &noBuffer);
    context->PSSetConstantBuffers(6, 1, &noBuffer);
    context->OMSetBlendState(oldBlend, oldBlendFactor, oldSampleMask);
    context->OMSetDepthStencilState(oldDepth, oldStencilRef);
    context->RSSetState(oldRasterizer);
    if (oldBlend) oldBlend->Release();
    if (oldDepth) oldDepth->Release();
    if (oldRasterizer) oldRasterizer->Release();
}

void WaterRenderer::Uninit() {
    Release();
}
