#include "SkinMesh.h"
#include "../Render/Graphics.h"
#include "Lighting.h"
#include "Shadow.h"
#include "RenderStats.h"
#include <cassert>
#include <cstring>

ID3D11DeviceContext* SkinMesh::context = nullptr;
ID3D11Buffer* SkinMesh::constantBuffer = nullptr;
ID3D11Buffer* SkinMesh::skinBuffer = nullptr;
ID3D11Buffer* SkinMesh::materialBuffer = nullptr;
ID3D11Buffer* SkinMesh::cameraBuffer = nullptr;
ID3D11VertexShader* SkinMesh::vertexShader = nullptr;
ID3D11PixelShader* SkinMesh::pixelShader = nullptr;
ID3D11VertexShader* SkinMesh::shadowVertexShader = nullptr;
ID3D11VertexShader* SkinMesh::outlineVertexShader = nullptr;
ID3D11PixelShader* SkinMesh::outlinePixelShader = nullptr;
ID3D11InputLayout* SkinMesh::inputLayout = nullptr;
ID3D11InputLayout* SkinMesh::outlineInputLayout = nullptr;
ID3D11SamplerState* SkinMesh::sampler = nullptr;
ID3D11BlendState* SkinMesh::blendState = nullptr;
ID3D11DepthStencilState* SkinMesh::depthStencilState = nullptr;
ID3D11DepthStencilState* SkinMesh::depthStencilStateNoWrite = nullptr;
ID3D11DepthStencilState* SkinMesh::outlineMarkState = nullptr;
ID3D11DepthStencilState* SkinMesh::outlineRingState = nullptr;
ID3D11DepthStencilState* SkinMesh::outlineMarkAllState = nullptr;
ID3D11DepthStencilState* SkinMesh::outlineRingSeeThroughState = nullptr;
ID3D11RasterizerState* SkinMesh::rasterizerState = nullptr;
ID3D11ShaderResourceView* SkinMesh::whiteTexture = nullptr;
float SkinMesh::aspectRatio = 1.f;
int SkinMesh::refCount = 0;

namespace {
    // 骨の行列(最大64本)。Shader3DSkin.hlsl の SkinBuffer と同じ並び(色は b2 のマテリアルで送る)
    struct SkinConstantBuffer {
        XMMATRIX bones[SkinMesh::maxBones];
    };
}

bool SkinMesh::Init(const std::vector<SkinVertex>& verts) {
    context = Graphics::context;
    aspectRatio = (Graphics::height > 0.f) ? (Graphics::width / Graphics::height) : 1.f;
    if (verts.empty()) return false;

    bool firstUser = (refCount == 0);
    if (!CreateSharedResources()) {
        OutputDebugStringA("★ SkinMesh CreateSharedResources 失敗(Shader3DSkin.hlsl を確認してください)\n");
        if (firstUser) ReleaseSharedResources();
        return false;
    }

    // 頂点バッファは変更不可(IMMUTABLE)。読み込み時に1回作るだけで、毎フレームの書き込みはしない
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(SkinVertex) * (UINT)verts.size();
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = verts.data();
    if (FAILED(Graphics::device->CreateBuffer(&bd, &data, &vertexBuffer))) {
        OutputDebugStringA("★ SkinMesh CreateVertexBuffer 失敗\n");
        if (firstUser) ReleaseSharedResources();
        return false;
    }

    // 枠線用: 同じ位置の頂点の法線を平均した法線(骨の影響が違っても、元の位置が同じなら同じ法線にする)
    {
        std::vector<XMFLOAT3> positions(verts.size()), normals(verts.size());
        for (size_t i = 0; i < verts.size(); i++) { positions[i] = verts[i].Position; normals[i] = verts[i].Normal; }
        std::vector<XMFLOAT3> smooth = Mesh::CreateSmoothNormals(positions, normals, nullptr);
        D3D11_BUFFER_DESC sbd = {};
        sbd.Usage = D3D11_USAGE_IMMUTABLE;
        sbd.ByteWidth = sizeof(XMFLOAT3) * (UINT)smooth.size();
        sbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sdata = {};
        sdata.pSysMem = smooth.data();
        if (FAILED(Graphics::device->CreateBuffer(&sbd, &sdata, &smoothNormalBuffer))) {
            OutputDebugStringA("★ SkinMesh 枠線用の法線バッファ作成に失敗\n");
            vertexBuffer->Release(); vertexBuffer = nullptr;
            if (firstUser) ReleaseSharedResources();
            return false;
        }
    }

    vertexCount = (UINT)verts.size();
    refCount++;
    return true;
}

bool SkinMesh::CreateSharedResources() {
    // 既に他のSkinMeshが作成済みなら、それをそのまま使い回す
    if (vertexShader) return true;
    if (!CreateShaders()) return false;
    if (!CreateStates()) return false;

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth = sizeof(MeshConstantBuffer);
    if (FAILED(Graphics::device->CreateBuffer(&cbd, nullptr, &constantBuffer))) return false;
    cbd.ByteWidth = sizeof(SkinConstantBuffer);
    if (FAILED(Graphics::device->CreateBuffer(&cbd, nullptr, &skinBuffer))) return false;
    cbd.ByteWidth = sizeof(MaterialConstantBuffer);
    if (FAILED(Graphics::device->CreateBuffer(&cbd, nullptr, &materialBuffer))) return false;
    cbd.ByteWidth = sizeof(MeshCameraBuffer);
    if (FAILED(Graphics::device->CreateBuffer(&cbd, nullptr, &cameraBuffer))) return false;
    return true;
}

void SkinMesh::ReleaseSharedResources() {
    if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
    if (skinBuffer) { skinBuffer->Release(); skinBuffer = nullptr; }
    if (materialBuffer) { materialBuffer->Release(); materialBuffer = nullptr; }
    if (cameraBuffer) { cameraBuffer->Release(); cameraBuffer = nullptr; }
    if (blendState) { blendState->Release(); blendState = nullptr; }
    if (depthStencilState) { depthStencilState->Release(); depthStencilState = nullptr; }
    if (depthStencilStateNoWrite) { depthStencilStateNoWrite->Release(); depthStencilStateNoWrite = nullptr; }
    if (outlineMarkState) { outlineMarkState->Release(); outlineMarkState = nullptr; }
    if (outlineRingState) { outlineRingState->Release(); outlineRingState = nullptr; }
    if (outlineMarkAllState) { outlineMarkAllState->Release(); outlineMarkAllState = nullptr; }
    if (outlineRingSeeThroughState) { outlineRingSeeThroughState->Release(); outlineRingSeeThroughState = nullptr; }
    if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
    if (whiteTexture) { whiteTexture->Release(); whiteTexture = nullptr; }
    if (sampler) { sampler->Release(); sampler = nullptr; }
    if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
    if (outlineInputLayout) { outlineInputLayout->Release(); outlineInputLayout = nullptr; }
    if (outlineVertexShader) { outlineVertexShader->Release(); outlineVertexShader = nullptr; }
    if (outlinePixelShader) { outlinePixelShader->Release(); outlinePixelShader = nullptr; }
    if (pixelShader) { pixelShader->Release(); pixelShader = nullptr; }
    if (shadowVertexShader) { shadowVertexShader->Release(); shadowVertexShader = nullptr; }
    if (vertexShader) { vertexShader->Release(); vertexShader = nullptr; }
}

bool SkinMesh::CreateShaders() {
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* err = nullptr;

    // Shader3D.hlsl と同じく、実行時にコンパイルする(エラーは出力ウィンドウに出る)。
    // Shader3DCommon.hlsli を #include しているので、D3D_COMPILE_STANDARD_FILE_INCLUDE を渡す
    HRESULT hr = D3DCompileFromFile(L"Shader3DSkin.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, &vsBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3DSkin.hlsl が見つかりません\n"); }
        return false;
    }

    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3DSkin.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, &psBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3DSkin PS コンパイル失敗\n"); }
        vsBlob->Release();
        return false;
    }

    // 影の深度パス用の頂点シェーダー(同じファイルの VSShadow。入力は VS と同じなので、同じ入力レイアウトで使える)
    ID3DBlob* shadowBlob = nullptr;
    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3DSkin.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSShadow", "vs_5_0", 0, 0, &shadowBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3DSkin VSShadow コンパイル失敗\n"); }
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    // 枠線用の頂点シェーダー・ピクセルシェーダー(同じファイルの VSOutline / PSOutline)
    ID3DBlob* outlineVsBlob = nullptr;
    ID3DBlob* outlinePsBlob = nullptr;
    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3DSkin.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSOutline", "vs_5_0", 0, 0, &outlineVsBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3DSkin VSOutline コンパイル失敗\n"); }
        shadowBlob->Release(); vsBlob->Release(); psBlob->Release();
        return false;
    }
    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3DSkin.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSOutline", "ps_5_0", 0, 0, &outlinePsBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3DSkin PSOutline コンパイル失敗\n"); }
        outlineVsBlob->Release(); shadowBlob->Release(); vsBlob->Release(); psBlob->Release();
        return false;
    }

    bool ok = SUCCEEDED(Graphics::device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader))
           && SUCCEEDED(Graphics::device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader))
           && SUCCEEDED(Graphics::device->CreateVertexShader(shadowBlob->GetBufferPointer(), shadowBlob->GetBufferSize(), nullptr, &shadowVertexShader))
           && SUCCEEDED(Graphics::device->CreateVertexShader(outlineVsBlob->GetBufferPointer(), outlineVsBlob->GetBufferSize(), nullptr, &outlineVertexShader))
           && SUCCEEDED(Graphics::device->CreatePixelShader(outlinePsBlob->GetBufferPointer(), outlinePsBlob->GetBufferSize(), nullptr, &outlinePixelShader));
    shadowBlob->Release();
    if (ok) {
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        ok = SUCCEEDED(Graphics::device->CreateInputLayout(layout, 6, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout));
        if (!ok) OutputDebugStringA("★ SkinMesh CreateInputLayout 失敗\n");
    }
    if (ok) {
        // 枠線用: slot 0 = 位置・法線・骨(色と画像は使わない)、slot 1 = 平均した法線
        D3D11_INPUT_ELEMENT_DESC outlineLayout[] = {
            { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",       1, DXGI_FORMAT_R32G32B32_FLOAT,    1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        ok = SUCCEEDED(Graphics::device->CreateInputLayout(outlineLayout, 5, outlineVsBlob->GetBufferPointer(), outlineVsBlob->GetBufferSize(), &outlineInputLayout));
        if (!ok) OutputDebugStringA("★ SkinMesh 枠線の CreateInputLayout 失敗\n");
    }
    outlineVsBlob->Release();
    outlinePsBlob->Release();
    if (!ok) OutputDebugStringA("★ SkinMesh シェーダーの作成に失敗\n");

    vsBlob->Release();
    psBlob->Release();
    return ok;
}

bool SkinMesh::CreateStates() {
    // Mesh と同じ設定(両面描画・深度テストあり・アルファブレンド)
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    if (FAILED(Graphics::device->CreateSamplerState(&sd, &sampler))) return false;

    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(Graphics::device->CreateBlendState(&bd, &blendState))) return false;

    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    if (FAILED(Graphics::device->CreateDepthStencilState(&dsd, &depthStencilState))) return false;

    // material.depthWrite = false 用: テストはするが書き込みはしない(Mesh と同じ)
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    if (FAILED(Graphics::device->CreateDepthStencilState(&dsd, &depthStencilStateNoWrite))) return false;

    // 枠線用(1): 本物と同じ深度(LESS_EQUAL)を通った画素のステンシルを、参照値に書き換える。深度は書かない
    D3D11_DEPTH_STENCIL_DESC mark = {};
    mark.DepthEnable = TRUE;
    mark.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    mark.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    mark.StencilEnable = TRUE;
    mark.StencilReadMask = 0xFF;
    mark.StencilWriteMask = 0xFF;
    mark.FrontFace = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_ALWAYS };
    mark.BackFace = mark.FrontFace;
    if (FAILED(Graphics::device->CreateDepthStencilState(&mark, &outlineMarkState))) return false;

    // 枠線用(2): 普通の深度テスト(書き込みあり)。ステンシルが参照値(1)の画素=本物の内側には描かない
    D3D11_DEPTH_STENCIL_DESC ring = {};
    ring.DepthEnable = TRUE;
    ring.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    ring.DepthFunc = D3D11_COMPARISON_LESS;
    ring.StencilEnable = TRUE;
    ring.StencilReadMask = 0xFF;
    ring.StencilWriteMask = 0;
    ring.FrontFace = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_NOT_EQUAL };
    ring.BackFace = ring.FrontFace;
    if (FAILED(Graphics::device->CreateDepthStencilState(&ring, &outlineRingState))) return false;

    // 枠線(壁越し)用(1): 深度テストなしで、本物のシルエット全体のステンシルを参照値に書き換える
    D3D11_DEPTH_STENCIL_DESC markAll = mark;
    markAll.DepthEnable = FALSE;
    if (FAILED(Graphics::device->CreateDepthStencilState(&markAll, &outlineMarkAllState))) return false;

    // 枠線(壁越し)用(2): 深度テストも深度の書き込みもなし。ステンシルが参照値(1)の画素(シルエットの内側)には描かない
    D3D11_DEPTH_STENCIL_DESC ringSeeThrough = ring;
    ringSeeThrough.DepthEnable = FALSE;
    ringSeeThrough.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    if (FAILED(Graphics::device->CreateDepthStencilState(&ringSeeThrough, &outlineRingSeeThroughState))) return false;

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    if (FAILED(Graphics::device->CreateRasterizerState(&rd, &rasterizerState))) return false;

    const UINT32 white = 0xFFFFFFFF;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1; td.Height = 1; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &white;
    data.SysMemPitch = sizeof(white);
    ID3D11Texture2D* texture = nullptr;
    if (FAILED(Graphics::device->CreateTexture2D(&td, &data, &texture))) return false;
    HRESULT hr = Graphics::device->CreateShaderResourceView(texture, nullptr, &whiteTexture);
    texture->Release();
    return SUCCEEDED(hr);
}

void SkinMesh::Draw(const XMMATRIX& world, const XMMATRIX* boneMatrices, int boneCount,
    const Camera& camera, XMFLOAT4 color, const Light& light) {
    // 色だけを変えた、既定のマテリアルで描く
    Material material;
    material.color = color;
    Draw(world, boneMatrices, boneCount, camera, material, light);
}

void SkinMesh::Draw(const XMMATRIX& world, const XMMATRIX* boneMatrices, int boneCount,
    const Camera& camera, const Material& material, const Light& light) {
    assert(context && vertexBuffer && "SkinMesh が初期化されていません");
    if (boneCount > maxBones) boneCount = maxBones;

    // b0: Shader3D.hlsl と同じ(転置して送り、シェーダー側は mul(vector, matrix))
    MeshConstantBuffer cb;
    cb.world = XMMatrixTranspose(world);
    cb.view = XMMatrixTranspose(camera.GetViewMatrix());
    cb.proj = XMMatrixTranspose(camera.GetProjectionMatrix(aspectRatio));
    cb.lightDir = { light.direction.x, light.direction.y, light.direction.z, 0.f };
    cb.lightColor = { light.color.x, light.color.y, light.color.z, 0.f };
    cb.ambient = { light.ambient.x, light.ambient.y, light.ambient.z, 0.f };

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    // b1: 骨の行列(使わない分は単位行列)
    SkinConstantBuffer sb;
    for (int i = 0; i < maxBones; i++) {
        sb.bones[i] = (i < boneCount) ? XMMatrixTranspose(boneMatrices[i]) : XMMatrixIdentity();
    }
    context->Map(skinBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &sb, sizeof(sb));
    context->Unmap(skinBuffer, 0);

    // b2: マテリアル(色・光沢・自発光・光を使うか)
    MaterialConstantBuffer mb = MakeMaterialConstantBuffer(material);
    mb.outlineParams.y = Graphics::width;
    mb.outlineParams.z = Graphics::height;
    context->Map(materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &mb, sizeof(mb));
    context->Unmap(materialBuffer, 0);

    // b3: カメラの位置
    MeshCameraBuffer eye = { { camera.position.x, camera.position.y, camera.position.z, 1.f } };
    context->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &eye, sizeof(eye));
    context->Unmap(cameraBuffer, 0);

    float blendFactor[4] = {};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(material.depthWrite ? depthStencilState : depthStencilStateNoWrite, 0);
    context->RSSetState(rasterizerState);

    UINT stride = sizeof(SkinVertex), offset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11Buffer* vsBuffers[2] = { constantBuffer, skinBuffer };
    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 2, vsBuffers);

    context->PSSetShader(pixelShader, nullptr, 0);
    ID3D11Buffer* psBuffers[4] = { constantBuffer, nullptr, materialBuffer, cameraBuffer };
    context->PSSetConstantBuffers(0, 4, psBuffers);
    Lighting::Bind();   // b4: 点光源・スポットライト
    Shadow::Bind();     // b5 / t1 / s1: 影
    ID3D11ShaderResourceView* view = srv ? srv : whiteTexture;
    context->PSSetShaderResources(0, 1, &view);
    context->PSSetSamplers(0, 1, &sampler);

    RenderStats::AddDraw((int)(vertexCount / 3));
    context->Draw(vertexCount, 0);

    // 次に2D(Sprite)や普通のMeshが描画される時のために、ラスタライザ状態を既定に戻しておく
    context->RSSetState(nullptr);
}

void SkinMesh::DrawOutline(const XMMATRIX& world, const XMMATRIX* boneMatrices, int boneCount,
    const Camera& camera, XMFLOAT4 color, float widthPixels, bool seeThrough) {
    assert(context && vertexBuffer && "SkinMesh が初期化されていません");
    if (widthPixels <= 0.f || color.w <= 0.f) return;
    if (!smoothNormalBuffer) return;   // 枠線用の法線が無い時は何もしない(防御)
    if (boneCount > maxBones) boneCount = maxBones;

    // b0: ワールド・ビュー・射影(光は使わない)
    MeshConstantBuffer cb = {};
    cb.world = XMMatrixTranspose(world);
    cb.view = XMMatrixTranspose(camera.GetViewMatrix());
    cb.proj = XMMatrixTranspose(camera.GetProjectionMatrix(aspectRatio));
    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    // b1: 骨の行列(Draw と同じ。使わない分は単位行列)
    SkinConstantBuffer sb;
    for (int i = 0; i < maxBones; i++) {
        sb.bones[i] = (i < boneCount) ? XMMatrixTranspose(boneMatrices[i]) : XMMatrixIdentity();
    }
    context->Map(skinBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &sb, sizeof(sb));
    context->Unmap(skinBuffer, 0);

    // b2: 枠線の色・太さ・画面の大きさ
    Material outlineMaterial;
    outlineMaterial.outlineColor = color;
    outlineMaterial.outlineWidth = widthPixels;
    MaterialConstantBuffer mb = MakeMaterialConstantBuffer(outlineMaterial);
    mb.outlineParams.y = Graphics::width;
    mb.outlineParams.z = Graphics::height;
    context->Map(materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &mb, sizeof(mb));
    context->Unmap(materialBuffer, 0);

    // b3: カメラの位置(裏向きの面かどうかの判定に使う)
    MeshCameraBuffer eye = { { camera.position.x, camera.position.y, camera.position.z, 1.f } };
    context->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &eye, sizeof(eye));
    context->Unmap(cameraBuffer, 0);

    float blendFactor[4] = {};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
    context->RSSetState(rasterizerState);

    // (1) 本物の見えている画素(深度が本物と同じ画素)のステンシルを1にする。色も深度も書かない
    //     (枠線を本物の内側に描かないための印。枠線の深度を手前へずらしても、内側に漏れない)
    UINT markStride = sizeof(SkinVertex), markOffset = 0;
    ID3D11Buffer* markVsBuffers[2] = { constantBuffer, skinBuffer };
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &markStride, &markOffset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(shadowVertexShader, nullptr, 0);   // 深度だけの頂点シェーダー(骨で曲げた本物の位置と同じ計算)
    context->VSSetConstantBuffers(0, 2, markVsBuffers);
    context->PSSetShader(nullptr, nullptr, 0);
    context->OMSetDepthStencilState(seeThrough ? outlineMarkAllState : outlineMarkState, 1);
    RenderStats::AddOutlineDraw((int)(vertexCount / 3));
    context->Draw(vertexCount, 0);

    // (2) 枠線: ステンシルが1の画素には描かない
    context->OMSetDepthStencilState(seeThrough ? outlineRingSeeThroughState : outlineRingState, 1);

    ID3D11Buffer* buffers[2] = { vertexBuffer, smoothNormalBuffer };
    UINT strides[2] = { sizeof(SkinVertex), sizeof(XMFLOAT3) };
    UINT offsets[2] = { 0, 0 };
    context->IASetInputLayout(outlineInputLayout);
    context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11Buffer* vsBuffers[3] = { constantBuffer, skinBuffer, materialBuffer };
    context->VSSetShader(outlineVertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 3, vsBuffers);
    context->PSSetShader(outlinePixelShader, nullptr, 0);
    ID3D11Buffer* psBuffers[4] = { constantBuffer, nullptr, materialBuffer, cameraBuffer };
    context->PSSetConstantBuffers(0, 4, psBuffers);

    RenderStats::AddOutlineDraw((int)(vertexCount / 3));
    context->Draw(vertexCount, 0);

    // (3) 印を消す(ステンシルを0に戻す。他の描画に影響しないように)
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &markStride, &markOffset);
    context->VSSetShader(shadowVertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 2, markVsBuffers);
    context->PSSetShader(nullptr, nullptr, 0);
    context->OMSetDepthStencilState(seeThrough ? outlineMarkAllState : outlineMarkState, 0);
    RenderStats::AddOutlineDraw((int)(vertexCount / 3));
    context->Draw(vertexCount, 0);

    // 深度ステンシルを普通の状態に戻し、slot 1 の頂点バッファを外し(次の普通の描画は slot 0 だけ使う)、ラスタライザ状態を既定に戻す
    ID3D11Buffer* noBuffers[2] = { nullptr, nullptr };
    UINT noStrides[2] = { 0, 0 };
    context->IASetVertexBuffers(0, 2, noBuffers, noStrides, offsets);
    context->OMSetDepthStencilState(depthStencilState, 0);
    context->RSSetState(nullptr);
}

void SkinMesh::DrawDepth(const XMMATRIX& world, const XMMATRIX* boneMatrices, int boneCount,
    const XMMATRIX& lightView, const XMMATRIX& lightProj) {
    assert(context && vertexBuffer && "SkinMesh が初期化されていません");
    if (boneCount > maxBones) boneCount = maxBones;

    // b0 の view / proj に、光から見た行列を入れる(Shader3DSkin.hlsl の VSShadow がそのまま使う)
    MeshConstantBuffer cb = {};
    cb.world = XMMatrixTranspose(world);
    cb.view = XMMatrixTranspose(lightView);
    cb.proj = XMMatrixTranspose(lightProj);
    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    // b1: 骨の行列(Draw と同じ。使わない分は単位行列)
    SkinConstantBuffer sb;
    for (int i = 0; i < maxBones; i++) {
        sb.bones[i] = (i < boneCount) ? XMMatrixTranspose(boneMatrices[i]) : XMMatrixIdentity();
    }
    context->Map(skinBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &sb, sizeof(sb));
    context->Unmap(skinBuffer, 0);

    UINT stride = sizeof(SkinVertex), offset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11Buffer* vsBuffers[2] = { constantBuffer, skinBuffer };
    context->VSSetShader(shadowVertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 2, vsBuffers);
    context->PSSetShader(nullptr, nullptr, 0);   // 色は描かない(深度だけ)

    RenderStats::AddShadowDraw((int)(vertexCount / 3));
    context->Draw(vertexCount, 0);
}

void SkinMesh::Uninit() {
    if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
    if (smoothNormalBuffer) { smoothNormalBuffer->Release(); smoothNormalBuffer = nullptr; }
    vertexCount = 0;

    // 共有リソースは、最後の1体が使い終わった時にだけ解放する
    refCount--;
    if (refCount <= 0) {
        ReleaseSharedResources();
        refCount = 0;
    }
}
