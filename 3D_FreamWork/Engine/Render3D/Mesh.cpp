#include "Mesh.h"
#include "../Render/Graphics.h"
#include "Lighting.h"
#include "Shadow.h"
#include "RenderStats.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

ID3D11DeviceContext* Mesh::context = nullptr;
ID3D11Buffer* Mesh::constantBuffer = nullptr;
ID3D11Buffer* Mesh::materialBuffer = nullptr;
ID3D11Buffer* Mesh::cameraBuffer = nullptr;
ID3D11VertexShader* Mesh::vertexShader = nullptr;
ID3D11PixelShader* Mesh::pixelShader = nullptr;
ID3D11VertexShader* Mesh::shadowVertexShader = nullptr;
ID3D11VertexShader* Mesh::outlineVertexShader = nullptr;
ID3D11PixelShader* Mesh::outlinePixelShader = nullptr;
ID3D11InputLayout* Mesh::inputLayout = nullptr;
ID3D11InputLayout* Mesh::outlineInputLayout = nullptr;
ID3D11SamplerState* Mesh::sampler = nullptr;
ID3D11BlendState* Mesh::blendState = nullptr;
ID3D11DepthStencilState* Mesh::depthStencilState = nullptr;
ID3D11DepthStencilState* Mesh::depthStencilStateNoWrite = nullptr;
ID3D11DepthStencilState* Mesh::depthStencilStateNone = nullptr;
ID3D11DepthStencilState* Mesh::outlineMarkState = nullptr;
ID3D11DepthStencilState* Mesh::outlineRingState = nullptr;
ID3D11DepthStencilState* Mesh::outlineMarkAllState = nullptr;
ID3D11DepthStencilState* Mesh::outlineRingSeeThroughState = nullptr;
ID3D11RasterizerState* Mesh::rasterizerState = nullptr;
ID3D11ShaderResourceView* Mesh::whiteTexture = nullptr;
float Mesh::aspectRatio = 1.f;
int Mesh::refCount = 0;

// 1回の描画命令で描く三角形の数(線のメッシュは0。RenderStats の数え用)
static int CountTriangles(D3D11_PRIMITIVE_TOPOLOGY topology, UINT vertexCount, UINT indexCount, bool indexed) {
    if (topology != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) return 0;
    return (int)((indexed ? indexCount : vertexCount) / 3);
}

bool Mesh::Init(const std::vector<MeshVertex>& verts, D3D11_PRIMITIVE_TOPOLOGY topo) {
    return InitInternal(verts, nullptr, topo);
}

bool Mesh::InitIndexed(const std::vector<MeshVertex>& verts, const std::vector<UINT>& indices, D3D11_PRIMITIVE_TOPOLOGY topo) {
    return InitInternal(verts, &indices, topo);
}

bool Mesh::InitInternal(const std::vector<MeshVertex>& verts, const std::vector<UINT>* indices, D3D11_PRIMITIVE_TOPOLOGY topo) {
    context = Graphics::context;
    topology = topo;
    aspectRatio = (Graphics::height > 0.f) ? (Graphics::width / Graphics::height) : 1.f;

    if (!CreateSharedResources()) { OutputDebugStringA("★ Mesh CreateSharedResources 失敗\n"); return false; }
    bool alreadyCounted = (vertexBuffer != nullptr);   // 同じMeshでInitを2回呼んだ時は、数え直さない
    if (!CreateBuffers(verts, indices)) { OutputDebugStringA("★ Mesh CreateVertexBuffer 失敗\n"); return false; }

    if (!alreadyCounted) refCount++;
    return true;
}

bool Mesh::CreateSharedResources() {
    // 既に他のMeshが作成済みなら、それをそのまま使い回す
    if (vertexShader) return true;

    if (!CreateShaders()) { OutputDebugStringA("★ Mesh CreateShaders 失敗\n"); return false; }
    if (!CreateConstantBuffer()) { OutputDebugStringA("★ Mesh CreateConstantBuffer 失敗\n"); return false; }
    if (!CreateSampler()) { OutputDebugStringA("★ Mesh CreateSampler 失敗\n"); return false; }
    if (!CreateBlendState()) { OutputDebugStringA("★ Mesh CreateBlendState 失敗\n"); return false; }
    if (!CreateDepthStencilState()) { OutputDebugStringA("★ Mesh CreateDepthStencilState 失敗\n"); return false; }
    if (!CreateRasterizerState()) { OutputDebugStringA("★ Mesh CreateRasterizerState 失敗\n"); return false; }
    if (!CreateWhiteTexture()) { OutputDebugStringA("★ Mesh CreateWhiteTexture 失敗\n"); return false; }
    return true;
}

void Mesh::ReleaseSharedResources() {
    if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
    if (materialBuffer) { materialBuffer->Release(); materialBuffer = nullptr; }
    if (cameraBuffer) { cameraBuffer->Release(); cameraBuffer = nullptr; }
    if (blendState) { blendState->Release(); blendState = nullptr; }
    if (depthStencilState) { depthStencilState->Release(); depthStencilState = nullptr; }
    if (depthStencilStateNoWrite) { depthStencilStateNoWrite->Release(); depthStencilStateNoWrite = nullptr; }
    if (depthStencilStateNone) { depthStencilStateNone->Release(); depthStencilStateNone = nullptr; }
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

bool Mesh::CreateShaders() {
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* err = nullptr;

    // Shader3DCommon.hlsli を #include しているので、D3D_COMPILE_STANDARD_FILE_INCLUDE を渡す
    // (.hlsli はShader3D.hlslと同じ場所に必要。Engine.vcxprojのCopyRuntimeFilesでコピーしている)
    HRESULT hr = D3DCompileFromFile(L"Shader3D.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VS", "vs_5_0", 0, 0, &vsBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3D.hlsl が見つかりません\n"); }
        return false;
    }

    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3D.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS", "ps_5_0", 0, 0, &psBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3D PS コンパイル失敗\n"); }
        vsBlob->Release();
        return false;
    }

    hr = Graphics::device->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) {
        OutputDebugStringA("★ Mesh CreateVertexShader 失敗\n");
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    hr = Graphics::device->CreatePixelShader(psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) {
        OutputDebugStringA("★ Mesh CreatePixelShader 失敗\n");
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    // 影の深度パス用の頂点シェーダー(同じファイルの VSShadow。入力は VS と同じなので、同じ入力レイアウトで使える)
    ID3DBlob* shadowBlob = nullptr;
    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3D.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSShadow", "vs_5_0", 0, 0, &shadowBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3D VSShadow コンパイル失敗\n"); }
        vsBlob->Release(); psBlob->Release();
        return false;
    }
    hr = Graphics::device->CreateVertexShader(shadowBlob->GetBufferPointer(),
        shadowBlob->GetBufferSize(), nullptr, &shadowVertexShader);
    shadowBlob->Release();
    if (FAILED(hr)) {
        OutputDebugStringA("★ Mesh CreateVertexShader(影) 失敗\n");
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    // 枠線用の頂点シェーダー・ピクセルシェーダー(同じファイルの VSOutline / PSOutline)
    ID3DBlob* outlineVsBlob = nullptr;
    ID3DBlob* outlinePsBlob = nullptr;
    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3D.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSOutline", "vs_5_0", 0, 0, &outlineVsBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3D VSOutline コンパイル失敗\n"); }
        vsBlob->Release(); psBlob->Release();
        return false;
    }
    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3D.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSOutline", "ps_5_0", 0, 0, &outlinePsBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3D PSOutline コンパイル失敗\n"); }
        outlineVsBlob->Release();
        vsBlob->Release(); psBlob->Release();
        return false;
    }
    bool outlineOk =
        SUCCEEDED(Graphics::device->CreateVertexShader(outlineVsBlob->GetBufferPointer(), outlineVsBlob->GetBufferSize(), nullptr, &outlineVertexShader)) &&
        SUCCEEDED(Graphics::device->CreatePixelShader(outlinePsBlob->GetBufferPointer(), outlinePsBlob->GetBufferSize(), nullptr, &outlinePixelShader));
    if (outlineOk) {
        // slot 0 = 普通の頂点(位置・法線だけ使う)、slot 1 = 平均した法線
        D3D11_INPUT_ELEMENT_DESC outlineLayout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   1, DXGI_FORMAT_R32G32B32_FLOAT, 1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        outlineOk = SUCCEEDED(Graphics::device->CreateInputLayout(outlineLayout, 3,
            outlineVsBlob->GetBufferPointer(), outlineVsBlob->GetBufferSize(), &outlineInputLayout));
    }
    outlineVsBlob->Release();
    outlinePsBlob->Release();
    if (!outlineOk) {
        OutputDebugStringA("★ Mesh 枠線のシェーダー作成に失敗\n");
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = Graphics::device->CreateInputLayout(layout, 4,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    if (FAILED(hr)) {
        OutputDebugStringA("★ Mesh CreateInputLayout 失敗\n");
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    vsBlob->Release();
    psBlob->Release();
    return true;
}

bool Mesh::CreateBuffers(const std::vector<MeshVertex>& verts, const std::vector<UINT>* indices) {
    if (verts.empty()) return false;
    if (indices && indices->empty()) return false;

    // 頂点バッファは変更不可(IMMUTABLE)。作る時に1回だけ送り、描く度のコピーはしない
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(MeshVertex) * (UINT)verts.size();
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = verts.data();
    ID3D11Buffer* newVertexBuffer = nullptr;
    if (FAILED(Graphics::device->CreateBuffer(&bd, &data, &newVertexBuffer))) return false;

    ID3D11Buffer* newIndexBuffer = nullptr;
    if (indices) {
        D3D11_BUFFER_DESC ibd = {};
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.ByteWidth = sizeof(UINT) * (UINT)indices->size();
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA idata = {};
        idata.pSysMem = indices->data();
        if (FAILED(Graphics::device->CreateBuffer(&ibd, &idata, &newIndexBuffer))) {
            newVertexBuffer->Release();
            return false;
        }
    }

    // 枠線用: 三角形のメッシュだけ、同じ位置の法線を平均した法線のバッファを作る(線のメッシュは枠線を描かない)
    ID3D11Buffer* newSmoothBuffer = nullptr;
    if (topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
        std::vector<XMFLOAT3> positions(verts.size()), normals(verts.size());
        for (size_t i = 0; i < verts.size(); i++) { positions[i] = verts[i].Position; normals[i] = verts[i].Normal; }
        std::vector<XMFLOAT3> smooth = CreateSmoothNormals(positions, normals, indices);
        D3D11_BUFFER_DESC sbd = {};
        sbd.Usage = D3D11_USAGE_IMMUTABLE;
        sbd.ByteWidth = sizeof(XMFLOAT3) * (UINT)smooth.size();
        sbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sdata = {};
        sdata.pSysMem = smooth.data();
        if (FAILED(Graphics::device->CreateBuffer(&sbd, &sdata, &newSmoothBuffer))) {
            newVertexBuffer->Release();
            if (newIndexBuffer) newIndexBuffer->Release();
            return false;
        }
    }

    // 作り直された時(同じMeshでInitを2回呼んだ時)に前のバッファが残らないよう、先に手放す
    if (vertexBuffer) vertexBuffer->Release();
    if (indexBuffer) indexBuffer->Release();
    if (smoothNormalBuffer) smoothNormalBuffer->Release();
    vertexBuffer = newVertexBuffer;
    indexBuffer = newIndexBuffer;
    smoothNormalBuffer = newSmoothBuffer;    vertexCount = (UINT)verts.size();
    indexCount = indices ? (UINT)indices->size() : 0;
    return true;
}

bool Mesh::CreateConstantBuffer() {
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(MeshConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(Graphics::device->CreateBuffer(&cbd, nullptr, &constantBuffer))) return false;

    cbd.ByteWidth = sizeof(MaterialConstantBuffer);
    if (FAILED(Graphics::device->CreateBuffer(&cbd, nullptr, &materialBuffer))) return false;

    cbd.ByteWidth = sizeof(MeshCameraBuffer);
    return SUCCEEDED(Graphics::device->CreateBuffer(&cbd, nullptr, &cameraBuffer));
}

bool Mesh::CreateSampler() {
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    HRESULT hr = Graphics::device->CreateSamplerState(&sd, &sampler);
    return SUCCEEDED(hr);
}

bool Mesh::CreateBlendState() {
    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    HRESULT hr = Graphics::device->CreateBlendState(&bd, &blendState);
    return SUCCEEDED(hr);
}

bool Mesh::CreateDepthStencilState() {
    // 3Dは深度テストを有効にして、前後関係を正しく描画する(2D/Spriteとの違い)
    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    HRESULT hr = Graphics::device->CreateDepthStencilState(&dsd, &depthStencilState);
    if (FAILED(hr)) return false;

    // スカイボックス用: テストはするが書き込みはしない。
    // drawPriorityで必ず最初に描画される(クリア直後、深度バッファは全ピクセル遠方値)前提で、
    // 「書き込まない」ことで自分より後ろに何も無くても他の描画を一切邪魔しない、
    // Z-fightingが原理的に起こらない背景になる
    D3D11_DEPTH_STENCIL_DESC dsdNoWrite = {};
    dsdNoWrite.DepthEnable = TRUE;
    dsdNoWrite.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsdNoWrite.DepthFunc = D3D11_COMPARISON_LESS;
    hr = Graphics::device->CreateDepthStencilState(&dsdNoWrite, &depthStencilStateNoWrite);
    if (FAILED(hr)) return false;

    // 2Dの線用: 深度テストも書き込みもしない(描いた順に、上に重なる)
    D3D11_DEPTH_STENCIL_DESC dsdNone = {};
    dsdNone.DepthEnable = FALSE;
    dsdNone.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsdNone.DepthFunc = D3D11_COMPARISON_ALWAYS;
    hr = Graphics::device->CreateDepthStencilState(&dsdNone, &depthStencilStateNone);
    if (FAILED(hr)) return false;

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
    hr = Graphics::device->CreateDepthStencilState(&mark, &outlineMarkState);
    if (FAILED(hr)) return false;

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
    hr = Graphics::device->CreateDepthStencilState(&ring, &outlineRingState);
    if (FAILED(hr)) return false;
    // 枠線(壁越し)用(1): 深度テストなしで、本物のシルエット全体のステンシルを参照値に書き換える
    D3D11_DEPTH_STENCIL_DESC markAll = mark;
    markAll.DepthEnable = FALSE;
    if (FAILED(Graphics::device->CreateDepthStencilState(&markAll, &outlineMarkAllState))) return false;

    // 枠線(壁越し)用(2): 深度テストも深度の書き込みもなし。ステンシルが参照値(1)の画素(シルエットの内側)には描かない
    D3D11_DEPTH_STENCIL_DESC ringSeeThrough = ring;
    ringSeeThrough.DepthEnable = FALSE;
    ringSeeThrough.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    return SUCCEEDED(Graphics::device->CreateDepthStencilState(&ringSeeThrough, &outlineRingSeeThroughState));
}

bool Mesh::CreateRasterizerState() {
    // カリングはしない(両面とも描画する)。
    // 不透明な部分は深度テストだけで正しい前後関係になるし、
    // 透明な部分はシェーダー側のclip()で描画自体をスキップするので、
    // 結果として「透明な部分だけ裏面が透けて見える」形になる
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    HRESULT hr = Graphics::device->CreateRasterizerState(&rd, &rasterizerState);
    return SUCCEEDED(hr);
}

bool Mesh::CreateWhiteTexture() {
    const UINT32 white = 0xFFFFFFFF;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &white;
    data.SysMemPitch = sizeof(white);

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = Graphics::device->CreateTexture2D(&td, &data, &texture);
    if (FAILED(hr)) return false;
    hr = Graphics::device->CreateShaderResourceView(texture, nullptr, &whiteTexture);
    texture->Release();  // SRVが参照を持っているので、こちらの参照は手放してよい
    return SUCCEEDED(hr);
}

void Mesh::Draw(const Transform& transform, const Camera& camera, XMFLOAT4 color, const Light& light, bool writeDepth) {
    Draw(transform.GetWorldMatrix(), camera, color, light, writeDepth);
}

void Mesh::Draw(const XMMATRIX& world, const Camera& camera, XMFLOAT4 color, const Light& light, bool writeDepth) {
    // 色と深度の書き込みだけを変えた、既定のマテリアルで描く
    Material material;
    material.color = color;
    material.depthWrite = writeDepth;
    Draw(world, camera, material, light);
}

void Mesh::Draw(const Transform& transform, const Camera& camera, const Material& material, const Light& light) {
    Draw(transform.GetWorldMatrix(), camera, material, light);
}

void Mesh::DrawLines2D(const Transform& transform, const Camera& camera2D, XMFLOAT4 color, const Light& light) {
    DrawLines2D(transform.GetWorldMatrix(), camera2D, color, light);
}

void Mesh::DrawLines2D(const XMMATRIX& world, const Camera& camera2D, XMFLOAT4 color, const Light& light) {
    // 2Dの正射影: view = 2Dカメラの x,y を引く平行移動、proj = 画面の半分の大きさ(ピクセル)で割る(zは0に潰す)。Sprite::BuildWorldMatrix の2Dの式と同じ画面位置
    XMMATRIX view = XMMatrixTranslation(-camera2D.position.x, -camera2D.position.y, 0.f);
    XMMATRIX proj = XMMatrixScaling(2.f / Graphics::width, 2.f / Graphics::height, 0.f);
    Material material;
    material.color = color;
    material.lit = false;
    DrawWithMatrices(world, view, proj, camera2D.position, material, light, depthStencilStateNone);
}

void Mesh::Draw(const XMMATRIX& world, const Camera& camera, const Material& material, const Light& light) {
    DrawWithMatrices(world, camera.GetViewMatrix(), camera.GetProjectionMatrix(aspectRatio), camera.position, material,
        light, material.depthWrite ? depthStencilState : depthStencilStateNoWrite);
}

void Mesh::DrawWithMatrices(const XMMATRIX& world, const XMMATRIX& viewMatrix, const XMMATRIX& projMatrix, XMFLOAT3 eyePosition,
    const Material& material, const Light& light, ID3D11DepthStencilState* depthState) {
    assert(context && "context が nullptr");
    assert(vertexBuffer && "vertexBuffer が nullptr");

    // ★ World/View/Projectionをそれぞれ個別に転置して送る。
    //   シェーダー側はmul(vector, matrix)の順で使う(2DのShader.hlslと同じ考え方)
    MeshConstantBuffer cb;
    cb.world = XMMatrixTranspose(world);
    cb.view = XMMatrixTranspose(viewMatrix);
    cb.proj = XMMatrixTranspose(projMatrix);
    cb.lightDir = { light.direction.x, light.direction.y, light.direction.z, 0.f };
    cb.lightColor = { light.color.x, light.color.y, light.color.z, 0.f };
    cb.ambient = { light.ambient.x, light.ambient.y, light.ambient.z, 0.f };

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    // b2: マテリアル(色・光沢・自発光・光を使うか)
    MaterialConstantBuffer mb = MakeMaterialConstantBuffer(material);
    mb.outlineParams.y = Graphics::width;
    mb.outlineParams.z = Graphics::height;
    context->Map(materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &mb, sizeof(mb));
    context->Unmap(materialBuffer, 0);

    // b3: カメラの位置
    MeshCameraBuffer eye = { { eyePosition.x, eyePosition.y, eyePosition.z, 1.f } };
    context->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &eye, sizeof(eye));
    context->Unmap(cameraBuffer, 0);

    float blendFactor[4] = {};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthState, 0);
    context->RSSetState(rasterizerState);

    UINT stride = sizeof(MeshVertex), offset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);   // nullptrなら何も設定しない扱い
    context->IASetPrimitiveTopology(topology);

    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);

    context->PSSetShader(pixelShader, nullptr, 0);
    ID3D11Buffer* psBuffers[4] = { constantBuffer, nullptr, materialBuffer, cameraBuffer };
    context->PSSetConstantBuffers(0, 4, psBuffers);
    Lighting::Bind();   // b4: 点光源・スポットライト
    Shadow::Bind();     // b5 / t1 / s1: 影
    ID3D11ShaderResourceView* view = srv ? srv : whiteTexture;
    context->PSSetShaderResources(0, 1, &view);
    context->PSSetSamplers(0, 1, &sampler);

    RenderStats::AddDraw(CountTriangles(topology, vertexCount, indexCount, indexBuffer != nullptr));
    if (indexBuffer) context->DrawIndexed(indexCount, 0, 0);
    else context->Draw(vertexCount, 0);

    // 次に2D(Sprite)が描画される時のために、ラスタライザ状態を既定に戻しておく
    context->RSSetState(nullptr);
}

void Mesh::DrawOutline(const XMMATRIX& world, const Camera& camera, XMFLOAT4 color, float widthPixels, bool seeThrough) {
    assert(context && "context が nullptr");
    assert(vertexBuffer && "vertexBuffer が nullptr");
    if (widthPixels <= 0.f || color.w <= 0.f) return;
    if (!smoothNormalBuffer) return;   // 線のメッシュには枠線を付けない

    // b0: ワールド・ビュー・射影(光は使わない)
    MeshConstantBuffer cb = {};
    cb.world = XMMatrixTranspose(world);
    cb.view = XMMatrixTranspose(camera.GetViewMatrix());
    cb.proj = XMMatrixTranspose(camera.GetProjectionMatrix(aspectRatio));
    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

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
    UINT markStride = sizeof(MeshVertex), markOffset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &markStride, &markOffset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(topology);
    context->VSSetShader(shadowVertexShader, nullptr, 0);   // 深度だけの頂点シェーダー(本物の位置と同じ計算)
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetShader(nullptr, nullptr, 0);
    context->OMSetDepthStencilState(seeThrough ? outlineMarkAllState : outlineMarkState, 1);
    int outlineTriangles = CountTriangles(topology, vertexCount, indexCount, indexBuffer != nullptr);
    RenderStats::AddOutlineDraw(outlineTriangles);
    if (indexBuffer) context->DrawIndexed(indexCount, 0, 0);
    else context->Draw(vertexCount, 0);

    // (2) 枠線: ステンシルが1の画素には描かない
    context->OMSetDepthStencilState(seeThrough ? outlineRingSeeThroughState : outlineRingState, 1);

    ID3D11Buffer* buffers[2] = { vertexBuffer, smoothNormalBuffer };
    UINT strides[2] = { sizeof(MeshVertex), sizeof(XMFLOAT3) };
    UINT offsets[2] = { 0, 0 };
    context->IASetInputLayout(outlineInputLayout);
    context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(topology);

    ID3D11Buffer* vsBuffers[3] = { constantBuffer, nullptr, materialBuffer };
    context->VSSetShader(outlineVertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 3, vsBuffers);
    context->PSSetShader(outlinePixelShader, nullptr, 0);
    ID3D11Buffer* psBuffers[4] = { constantBuffer, nullptr, materialBuffer, cameraBuffer };
    context->PSSetConstantBuffers(0, 4, psBuffers);

    RenderStats::AddOutlineDraw(outlineTriangles);
    if (indexBuffer) context->DrawIndexed(indexCount, 0, 0);
    else context->Draw(vertexCount, 0);

    // (3) 印を消す(ステンシルを0に戻す。他の描画に影響しないように)
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &markStride, &markOffset);
    context->VSSetShader(shadowVertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetShader(nullptr, nullptr, 0);
    context->OMSetDepthStencilState(seeThrough ? outlineMarkAllState : outlineMarkState, 0);
    RenderStats::AddOutlineDraw(outlineTriangles);
    if (indexBuffer) context->DrawIndexed(indexCount, 0, 0);
    else context->Draw(vertexCount, 0);

    // 深度ステンシルを普通の状態に戻し、slot 1 の頂点バッファを外し(次の普通の描画は slot 0 だけ使う)、ラスタライザ状態を既定に戻す
    ID3D11Buffer* noBuffers[2] = { nullptr, nullptr };
    UINT noStrides[2] = { 0, 0 };
    context->IASetVertexBuffers(0, 2, noBuffers, noStrides, offsets);
    context->OMSetDepthStencilState(depthStencilState, 0);
    context->RSSetState(nullptr);
}

void Mesh::DrawDepth(const XMMATRIX& world, const XMMATRIX& lightView, const XMMATRIX& lightProj) {
    assert(context && "context が nullptr");
    assert(vertexBuffer && "vertexBuffer が nullptr");
    if (topology != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) return;   // 線は影にならない

    // b0 の view / proj に、光から見た行列を入れる(Shader3D.hlsl の VSShadow がそのまま使う)
    MeshConstantBuffer cb = {};
    cb.world = XMMatrixTranspose(world);
    cb.view = XMMatrixTranspose(lightView);
    cb.proj = XMMatrixTranspose(lightProj);
    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    UINT stride = sizeof(MeshVertex), offset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(topology);

    context->VSSetShader(shadowVertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetShader(nullptr, nullptr, 0);   // 色は描かない(深度だけ)

    RenderStats::AddShadowDraw(CountTriangles(topology, vertexCount, indexCount, indexBuffer != nullptr));
    if (indexBuffer) context->DrawIndexed(indexCount, 0, 0);
    else context->Draw(vertexCount, 0);
}

void Mesh::Uninit() {
    if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
    if (indexBuffer) { indexBuffer->Release(); indexBuffer = nullptr; }
    if (smoothNormalBuffer) { smoothNormalBuffer->Release(); smoothNormalBuffer = nullptr; }
    vertexCount = 0;
    indexCount = 0;

    // 共有リソースは、最後の1体が使い終わった時にだけ解放する
    refCount--;
    if (refCount <= 0) {
        ReleaseSharedResources();
        refCount = 0;
    }
}

std::vector<MeshVertex> Mesh::CreateCube() {
    XMFLOAT4 c = { 1.f, 1.f, 1.f, 1.f };
    XMFLOAT3 nFront{ 0.f, 0.f,-1.f }, nBack{ 0.f, 0.f, 1.f };
    XMFLOAT3 nLeft{ -1.f, 0.f, 0.f }, nRight{ 1.f, 0.f, 0.f };
    XMFLOAT3 nTop{ 0.f, 1.f, 0.f }, nBottom{ 0.f,-1.f, 0.f };
    return {
        // front (z = -0.5)
        {{-0.5f,-0.5f,-0.5f}, nFront, c, {0.f,1.f}}, {{ 0.5f,-0.5f,-0.5f}, nFront, c, {1.f,1.f}}, {{ 0.5f, 0.5f,-0.5f}, nFront, c, {1.f,0.f}},
        {{-0.5f,-0.5f,-0.5f}, nFront, c, {0.f,1.f}}, {{ 0.5f, 0.5f,-0.5f}, nFront, c, {1.f,0.f}}, {{-0.5f, 0.5f,-0.5f}, nFront, c, {0.f,0.f}},
        // back (z = 0.5)
        {{ 0.5f,-0.5f, 0.5f}, nBack, c, {0.f,1.f}}, {{-0.5f,-0.5f, 0.5f}, nBack, c, {1.f,1.f}}, {{-0.5f, 0.5f, 0.5f}, nBack, c, {1.f,0.f}},
        {{ 0.5f,-0.5f, 0.5f}, nBack, c, {0.f,1.f}}, {{-0.5f, 0.5f, 0.5f}, nBack, c, {1.f,0.f}}, {{ 0.5f, 0.5f, 0.5f}, nBack, c, {0.f,0.f}},
        // left (x = -0.5)
        {{-0.5f,-0.5f, 0.5f}, nLeft, c, {0.f,1.f}}, {{-0.5f,-0.5f,-0.5f}, nLeft, c, {1.f,1.f}}, {{-0.5f, 0.5f,-0.5f}, nLeft, c, {1.f,0.f}},
        {{-0.5f,-0.5f, 0.5f}, nLeft, c, {0.f,1.f}}, {{-0.5f, 0.5f,-0.5f}, nLeft, c, {1.f,0.f}}, {{-0.5f, 0.5f, 0.5f}, nLeft, c, {0.f,0.f}},
        // right (x = 0.5)
        {{ 0.5f,-0.5f,-0.5f}, nRight, c, {0.f,1.f}}, {{ 0.5f,-0.5f, 0.5f}, nRight, c, {1.f,1.f}}, {{ 0.5f, 0.5f, 0.5f}, nRight, c, {1.f,0.f}},
        {{ 0.5f,-0.5f,-0.5f}, nRight, c, {0.f,1.f}}, {{ 0.5f, 0.5f, 0.5f}, nRight, c, {1.f,0.f}}, {{ 0.5f, 0.5f,-0.5f}, nRight, c, {0.f,0.f}},
        // top (y = 0.5)
        {{-0.5f, 0.5f,-0.5f}, nTop, c, {0.f,1.f}}, {{ 0.5f, 0.5f,-0.5f}, nTop, c, {1.f,1.f}}, {{ 0.5f, 0.5f, 0.5f}, nTop, c, {1.f,0.f}},
        {{-0.5f, 0.5f,-0.5f}, nTop, c, {0.f,1.f}}, {{ 0.5f, 0.5f, 0.5f}, nTop, c, {1.f,0.f}}, {{-0.5f, 0.5f, 0.5f}, nTop, c, {0.f,0.f}},
        // bottom (y = -0.5)
        {{-0.5f,-0.5f, 0.5f}, nBottom, c, {0.f,1.f}}, {{ 0.5f,-0.5f, 0.5f}, nBottom, c, {1.f,1.f}}, {{ 0.5f,-0.5f,-0.5f}, nBottom, c, {1.f,0.f}},
        {{-0.5f,-0.5f, 0.5f}, nBottom, c, {0.f,1.f}}, {{ 0.5f,-0.5f,-0.5f}, nBottom, c, {1.f,0.f}}, {{-0.5f,-0.5f,-0.5f}, nBottom, c, {0.f,0.f}},
    };
}

std::vector<XMFLOAT3> Mesh::CreateSmoothNormals(const std::vector<XMFLOAT3>& positions,
    const std::vector<XMFLOAT3>& normals, const std::vector<UINT>* indices) {
    // 位置を細かい格子(1/10000)に丸めて「同じ位置」を見つける
    struct Key {
        long long x, y, z;
        bool operator==(const Key& o) const { return x == o.x && y == o.y && z == o.z; }
    };
    struct KeyHash {
        size_t operator()(const Key& k) const {
            return (size_t)(k.x * 73856093LL) ^ (size_t)(k.y * 19349663LL) ^ (size_t)(k.z * 83492791LL);
        }
    };
    auto MakeKey = [](const XMFLOAT3& p) {
        return Key{ (long long)floorf(p.x * 10000.f + 0.5f), (long long)floorf(p.y * 10000.f + 0.5f), (long long)floorf(p.z * 10000.f + 0.5f) };
    };

    std::unordered_map<Key, XMFLOAT3, KeyHash> sums;
    size_t indexCount = indices ? indices->size() : positions.size();
    auto VertexAt = [&](size_t i) { return indices ? (size_t)(*indices)[i] : i; };
    for (size_t t = 0; t + 2 < indexCount; t += 3) {
        size_t v[3] = { VertexAt(t), VertexAt(t + 1), VertexAt(t + 2) };
        if (v[0] >= positions.size() || v[1] >= positions.size() || v[2] >= positions.size()) continue;
        for (int k = 0; k < 3; k++) {
            // 三角形のこの角の大きさ(ラジアン)を重みにする(細かく割った面に引っ張られないように)
            XMVECTOR p = XMLoadFloat3(&positions[v[k]]);
            XMVECTOR e1 = XMLoadFloat3(&positions[v[(k + 1) % 3]]) - p;
            XMVECTOR e2 = XMLoadFloat3(&positions[v[(k + 2) % 3]]) - p;
            float l1 = XMVectorGetX(XMVector3Length(e1)), l2 = XMVectorGetX(XMVector3Length(e2));
            if (l1 < 1e-8f || l2 < 1e-8f) continue;   // つぶれた三角形
            float cosine = XMVectorGetX(XMVector3Dot(e1, e2)) / (l1 * l2);
            float weight = acosf((std::max)(-1.f, (std::min)(1.f, cosine)));
            XMFLOAT3& sum = sums[MakeKey(positions[v[k]])];
            sum.x += normals[v[k]].x * weight;
            sum.y += normals[v[k]].y * weight;
            sum.z += normals[v[k]].z * weight;
        }
    }

    std::vector<XMFLOAT3> result(positions.size());
    for (size_t i = 0; i < positions.size(); i++) {
        result[i] = normals[i];   // どの三角形にも使われない頂点・合計が0になる頂点は、元の法線のまま
        auto it = sums.find(MakeKey(positions[i]));
        if (it == sums.end()) continue;
        XMVECTOR sum = XMLoadFloat3(&it->second);
        if (XMVectorGetX(XMVector3Length(sum)) < 1e-6f) continue;
        XMStoreFloat3(&result[i], XMVector3Normalize(sum));
    }
    return result;
}

std::vector<MeshVertex> Mesh::CreateSphere(int rings, int segments) {
    XMFLOAT4 c = { 1.f, 1.f, 1.f, 1.f };
    const float radius = 0.5f;
    std::vector<MeshVertex> verts;

    // 経度(theta: 0〜PI、上から下)と緯度(phi: 0〜2PI、ぐるっと1周)で
    // 四角形の区画に分割し、それぞれを三角形2枚で埋めていく
    auto MakeVertex = [&](float theta, float phi) {
        float x = sinf(theta) * cosf(phi);
        float y = cosf(theta);
        float z = sinf(theta) * sinf(phi);
        float u = phi / (2.0f * XM_PI);
        float v = theta / XM_PI;
        return MeshVertex{ { x * radius, y * radius, z * radius }, { x, y, z }, c, { u, v } };
    };

    for (int lat = 0; lat < rings; lat++) {
        float theta0 = XM_PI * (float)lat / rings;
        float theta1 = XM_PI * (float)(lat + 1) / rings;

        for (int lon = 0; lon < segments; lon++) {
            float phi0 = 2.0f * XM_PI * (float)lon / segments;
            float phi1 = 2.0f * XM_PI * (float)(lon + 1) / segments;

            MeshVertex v00 = MakeVertex(theta0, phi0);
            MeshVertex v01 = MakeVertex(theta0, phi1);
            MeshVertex v10 = MakeVertex(theta1, phi0);
            MeshVertex v11 = MakeVertex(theta1, phi1);

            verts.push_back(v00); verts.push_back(v11); verts.push_back(v01);
            verts.push_back(v00); verts.push_back(v10); verts.push_back(v11);
        }
    }

    return verts;
}

std::vector<MeshVertex> Mesh::CreateCapsule(float radius, float height, int rings, int segments) {
    XMFLOAT4 c = { 1.f, 1.f, 1.f, 1.f };
    if (rings < 1) rings = 1;
    if (segments < 3) segments = 3;
    // 円柱部分の長さの半分(全体の高さから両端の半球を引く。負なら球)
    float halfCylinder = height * 0.5f - radius;
    if (halfCylinder < 0.f) halfCylinder = 0.f;

    // 上の半球(rings+1段)と下の半球(rings+1段)を、緯度の段として並べる。
    // 上の最後の段と下の最初の段の間が円柱部分になる
    struct Row { float theta; float offsetY; };
    std::vector<Row> rows;
    for (int i = 0; i <= rings; i++) rows.push_back({ XM_PIDIV2 * (float)i / rings, halfCylinder });
    for (int i = 0; i <= rings; i++) rows.push_back({ XM_PIDIV2 + XM_PIDIV2 * (float)i / rings, -halfCylinder });
    const float rowCount = (float)(rows.size() - 1);

    auto MakeVertex = [&](int row, float phi) {
        float theta = rows[row].theta;
        float x = sinf(theta) * cosf(phi);
        float y = cosf(theta);
        float z = sinf(theta) * sinf(phi);
        float u = phi / XM_2PI;
        float v = (float)row / rowCount;
        return MeshVertex{ { x * radius, y * radius + rows[row].offsetY, z * radius }, { x, y, z }, c, { u, v } };
    };

    std::vector<MeshVertex> verts;
    for (size_t row = 0; row + 1 < rows.size(); row++) {
        for (int lon = 0; lon < segments; lon++) {
            float phi0 = XM_2PI * (float)lon / segments;
            float phi1 = XM_2PI * (float)(lon + 1) / segments;
            MeshVertex v00 = MakeVertex((int)row, phi0);
            MeshVertex v01 = MakeVertex((int)row, phi1);
            MeshVertex v10 = MakeVertex((int)row + 1, phi0);
            MeshVertex v11 = MakeVertex((int)row + 1, phi1);
            // CreateSphereと同じ巻き方
            verts.push_back(v00); verts.push_back(v11); verts.push_back(v01);
            verts.push_back(v00); verts.push_back(v10); verts.push_back(v11);
        }
    }
    return verts;
}

// ─── 輪郭(線)データ ─────────────────────────────
namespace {
    // 線で使う頂点。光源の影響を受けない描き方(全方向から光)で使う前提なので法線は何でもよいが、
    // 0ベクトルだとシェーダーの正規化でNaNになるため、適当な向きを入れておく
    MeshVertex LineVertex(float x, float y, float z) {
        return MeshVertex{ { x, y, z }, { 0.f, 0.f, -1.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f } };
    }

    // 中心(0,0,0)・直径1の円を、segments本の線分で追加する。plane: 0=XY, 1=XZ, 2=YZ
    void AppendCircle(std::vector<MeshVertex>& out, int segments, int plane) {
        auto point = [&](int i) {
            float a = XM_2PI * (float)i / (float)segments;
            float c = cosf(a) * 0.5f, s = sinf(a) * 0.5f;
            if (plane == 0) return LineVertex(c, s, 0.f);
            if (plane == 1) return LineVertex(c, 0.f, s);
            return LineVertex(0.f, c, s);
        };
        for (int i = 0; i < segments; i++) {
            out.push_back(point(i));
            out.push_back(point(i + 1));
        }
    }
}

std::vector<MeshVertex> Mesh::CreateCubeOutline() {
    const float h = 0.5f;
    XMFLOAT3 c[8] = {
        { -h, -h, -h }, {  h, -h, -h }, {  h,  h, -h }, { -h,  h, -h },  // 手前(z=-0.5)
        { -h, -h,  h }, {  h, -h,  h }, {  h,  h,  h }, { -h,  h,  h },  // 奥(z=0.5)
    };
    const int edges[12][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },  // 手前の四角
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },  // 奥の四角
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },  // 手前と奥をつなぐ辺
    };
    std::vector<MeshVertex> verts;
    for (auto& e : edges) {
        verts.push_back(LineVertex(c[e[0]].x, c[e[0]].y, c[e[0]].z));
        verts.push_back(LineVertex(c[e[1]].x, c[e[1]].y, c[e[1]].z));
    }
    return verts;
}

std::vector<MeshVertex> Mesh::CreateSquareOutline() {
    const float h = 0.5f;
    XMFLOAT2 c[4] = { { -h, -h }, { h, -h }, { h, h }, { -h, h } };
    std::vector<MeshVertex> verts;
    for (int i = 0; i < 4; i++) {
        verts.push_back(LineVertex(c[i].x, c[i].y, 0.f));
        verts.push_back(LineVertex(c[(i + 1) % 4].x, c[(i + 1) % 4].y, 0.f));
    }
    return verts;
}

std::vector<MeshVertex> Mesh::CreateCircleOutline(int segments) {
    std::vector<MeshVertex> verts;
    AppendCircle(verts, segments, 0);
    return verts;
}

std::vector<MeshVertex> Mesh::CreateSphereOutline(int segments) {
    std::vector<MeshVertex> verts;
    AppendCircle(verts, segments, 0);
    AppendCircle(verts, segments, 1);
    AppendCircle(verts, segments, 2);
    return verts;
}

std::vector<MeshVertex> Mesh::CreateCapsuleOutline(float radius, float halfHeight,
    CapsuleAxis axis, int segments, bool crossSectionOnly) {
    std::vector<MeshVertex> verts;
    if (segments < 4) segments = 4;

    // 断面だけ(2D用)で、長い向きがZの時は、XY平面で見ると円になる
    if (crossSectionOnly && axis == CapsuleAxis::Z) {
        for (int i = 0; i < segments; i++) {
            float a0 = XM_2PI * (float)i / segments, a1 = XM_2PI * (float)(i + 1) / segments;
            verts.push_back(LineVertex(cosf(a0) * radius, sinf(a0) * radius, 0.f));
            verts.push_back(LineVertex(cosf(a1) * radius, sinf(a1) * radius, 0.f));
        }
        return verts;
    }

    // まずY向きのカプセルとして線を作り、axisに合わせて座標を入れ替える
    auto Add = [&](float x, float y, float z) {
        if (axis == CapsuleAxis::X) verts.push_back(LineVertex(y, x, z));        // Y→X
        else if (axis == CapsuleAxis::Z) verts.push_back(LineVertex(x, z, y));   // Y→Z
        else verts.push_back(LineVertex(x, y, z));
    };
    auto AddLine = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
        Add(x0, y0, z0);
        Add(x1, y1, z1);
    };

    // 両端の半円(XY平面と、3Dの時はZY平面)
    for (int i = 0; i < segments / 2; i++) {
        float a0 = XM_PI * (float)i / (segments / 2), a1 = XM_PI * (float)(i + 1) / (segments / 2);
        float c0 = cosf(a0) * radius, s0 = sinf(a0) * radius;
        float c1 = cosf(a1) * radius, s1 = sinf(a1) * radius;
        AddLine(c0, halfHeight + s0, 0.f, c1, halfHeight + s1, 0.f);     // 上(XY)
        AddLine(c0, -halfHeight - s0, 0.f, c1, -halfHeight - s1, 0.f);   // 下(XY)
        if (!crossSectionOnly) {
            AddLine(0.f, halfHeight + s0, c0, 0.f, halfHeight + s1, c1);     // 上(ZY)
            AddLine(0.f, -halfHeight - s0, c0, 0.f, -halfHeight - s1, c1);   // 下(ZY)
        }
    }
    // 側面の線
    AddLine(radius, -halfHeight, 0.f, radius, halfHeight, 0.f);
    AddLine(-radius, -halfHeight, 0.f, -radius, halfHeight, 0.f);
    if (!crossSectionOnly) {
        AddLine(0.f, -halfHeight, radius, 0.f, halfHeight, radius);
        AddLine(0.f, -halfHeight, -radius, 0.f, halfHeight, -radius);
        // 円柱の上下の輪(水平な円)
        for (int i = 0; i < segments; i++) {
            float a0 = XM_2PI * (float)i / segments, a1 = XM_2PI * (float)(i + 1) / segments;
            AddLine(cosf(a0) * radius, halfHeight, sinf(a0) * radius, cosf(a1) * radius, halfHeight, sinf(a1) * radius);
            AddLine(cosf(a0) * radius, -halfHeight, sinf(a0) * radius, cosf(a1) * radius, -halfHeight, sinf(a1) * radius);
        }
    }
    return verts;
}

// "1", "1/2", "1/2/3", "1//3" のいずれの形式からも、頂点・UV・法線インデックスを取り出す。
// 見つからない場合は0を返す(0はOBJでは使われない値なので「無い」の意味で使う)
static void ParseFaceVertex(const std::string& token, int& posIndex, int& uvIndex, int& normalIndex) {
    posIndex = 0;
    uvIndex = 0;
    normalIndex = 0;

    size_t slash1 = token.find('/');
    if (slash1 == std::string::npos) {
        posIndex = std::stoi(token);
        return;
    }

    posIndex = std::stoi(token.substr(0, slash1));

    size_t slash2 = token.find('/', slash1 + 1);
    if (slash2 == std::string::npos) {
        std::string uvPart = token.substr(slash1 + 1);
        if (!uvPart.empty()) uvIndex = std::stoi(uvPart);
        return;
    }

    std::string uvPart = token.substr(slash1 + 1, slash2 - slash1 - 1);
    if (!uvPart.empty()) uvIndex = std::stoi(uvPart);

    std::string nPart = token.substr(slash2 + 1);
    if (!nPart.empty()) normalIndex = std::stoi(nPart);
}

std::vector<MeshVertex> Mesh::LoadOBJ(const std::wstring& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        OutputDebugStringA("★ Mesh::LoadOBJ: ファイルが開けません\n");
        return {};
    }

    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT2> texcoords;
    std::vector<XMFLOAT3> normals;
    std::vector<MeshVertex> result;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "v") {
            XMFLOAT3 p;
            iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (tag == "vt") {
            XMFLOAT2 uv;
            iss >> uv.x >> uv.y;
            uv.y = 1.0f - uv.y;  // OBJのvtは下から上、このエンジンは上から下が基準なので反転する
            texcoords.push_back(uv);
        }
        else if (tag == "vn") {
            XMFLOAT3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (tag == "f") {
            std::vector<int> posIdx, uvIdx, normalIdx;
            std::string token;
            while (iss >> token) {
                int p = 0, t = 0, n = 0;
                ParseFaceVertex(token, p, t, n);

                // 負の値は「末尾からの相対インデックス」を意味する(OBJの仕様)
                if (p < 0) p = (int)positions.size() + p + 1;
                if (t < 0) t = (int)texcoords.size() + t + 1;
                if (n < 0) n = (int)normals.size() + n + 1;

                posIdx.push_back(p);
                uvIdx.push_back(t);
                normalIdx.push_back(n);
            }

            // 3頂点なら三角形そのまま、4頂点以上(四角形など)は扇状に三角形分割する
            for (int i = 1; i + 1 < (int)posIdx.size(); i++) {
                int triangle[3] = { 0, i, i + 1 };
                MeshVertex tri[3]{};

                for (int k = 0; k < 3; k++) {
                    int idx = triangle[k];
                    MeshVertex v{};
                    v.Color = { 1.f, 1.f, 1.f, 1.f };

                    int pI = posIdx[idx];
                    if (pI >= 1 && pI <= (int)positions.size()) v.Position = positions[pI - 1];

                    int tI = uvIdx[idx];
                    if (tI >= 1 && tI <= (int)texcoords.size()) v.TexCoord = texcoords[tI - 1];

                    int nI = normalIdx[idx];
                    if (nI >= 1 && nI <= (int)normals.size()) v.Normal = normals[nI - 1];

                    tri[k] = v;
                }

                // ファイルにvnが無かった場合は、2辺の外積からこの三角形の面法線を求めて代用する
                bool hasNormals = !normals.empty();
                if (!hasNormals) {
                    XMVECTOR p0 = XMLoadFloat3(&tri[0].Position);
                    XMVECTOR p1 = XMLoadFloat3(&tri[1].Position);
                    XMVECTOR p2 = XMLoadFloat3(&tri[2].Position);
                    XMVECTOR faceNormal = XMVector3Normalize(XMVector3Cross(p1 - p0, p2 - p0));
                    XMFLOAT3 n;
                    XMStoreFloat3(&n, faceNormal);
                    tri[0].Normal = tri[1].Normal = tri[2].Normal = n;
                }

                result.push_back(tri[0]);
                result.push_back(tri[1]);
                result.push_back(tri[2]);
            }
        }
    }

    if (result.empty()) {
        OutputDebugStringA("★ Mesh::LoadOBJ: 頂点を読み込めませんでした(面が無いか、形式が非対応の可能性)\n");
    }

    return result;
}
