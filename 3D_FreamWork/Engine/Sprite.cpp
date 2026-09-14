#include "Sprite.h"
#include "WICTextureLoader11.h"
#include "Graphics.h"

bool Sprite::Init() {
    context = Graphics::context;
    resW = Graphics::width;
    resH = Graphics::height;

    if (!CreateShaders()) { OutputDebugStringA("★ CreateShaders 失敗\n");    return false; }
    if (!CreateBuffers()) { OutputDebugStringA("★ CreateBuffers 失敗\n");    return false; }
    if (!CreateSampler()) { OutputDebugStringA("★ CreateSampler 失敗\n");    return false; }
    if (!CreateBlendState()) { OutputDebugStringA("★ CreateBlendState 失敗\n"); return false; }
    return true;
}

bool Sprite::CreateShaders() {
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* err = nullptr;

    HRESULT hr = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr,
        "VS", "vs_5_0", 0, 0, &vsBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ shader.hlsl が見つかりません\n"); }
        return false;
    }

    err = nullptr;
    hr = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr,
        "PS", "ps_5_0", 0, 0, &psBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ PS コンパイル失敗（原因不明）\n"); }
        vsBlob->Release();
        return false;
    }

    Graphics::device->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), nullptr, &vertexShader);
    Graphics::device->CreatePixelShader(psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(), nullptr, &pixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,      0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    Graphics::device->CreateInputLayout(layout, 3,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

    vsBlob->Release();
    psBlob->Release();
    return true;
}

bool Sprite::CreateBuffers() {
    // ---- 頂点バッファ ----
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(Vertex) * 64;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = Graphics::device->CreateBuffer(&bd, nullptr, &vertexBuffer);
    if (FAILED(hr)) return false;

    // ---- 定数バッファ（ワールド行列）----
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = Graphics::device->CreateBuffer(&cbd, nullptr, &constantBuffer);
    return SUCCEEDED(hr);
}

bool Sprite::CreateSampler() {
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    HRESULT hr = Graphics::device->CreateSamplerState(&sd, &sampler);
    return SUCCEEDED(hr);
}

bool Sprite::CreateBlendState() {
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

// ---------------------------------------------------------------------------
// BuildWorldMatrix
//   Transform → ワールド行列（平行移動・回転・スケール）を組み立てる
//   最終的に NDC 変換まで含めた行列を返す
// ---------------------------------------------------------------------------
XMMATRIX Sprite::BuildWorldMatrix(const Transform& transform, float crushY) const {
    float w = (transform.scale.x > 0.f) ? transform.scale.x : (float)texWidth;
    float h = (transform.scale.y > 0.f) ? transform.scale.y : (float)texHeight;
    float rad = transform.rotate.z * (XM_PI / 180.f);
    float ndcX = transform.position.x / (resW * 0.5f);
    float ndcY = -transform.position.y / (resH * 0.5f);

    XMMATRIX S = XMMatrixScaling(w, h, 1.f);
    XMMATRIX NDC = XMMatrixScaling(2.f / resW, 2.f / resH, 1.f);
    XMMATRIX R = XMMatrixRotationZ(rad);
    XMMATRIX C = XMMatrixScaling(1.f, std::abs(cos(crushY * (XM_PI / 180.f))), 1.f);
    XMMATRIX T = XMMatrixTranslation(ndcX, ndcY, 0.f);

    return XMMatrixTranspose(S * R * NDC * C * T);
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------
void Sprite::Draw(Transform transform, XMFLOAT4 color, const SpriteSheet& sheet) {
    assert(context && "context が nullptr");
    assert(vertexBuffer && "vertexBuffer が nullptr");
    assert(constantBuffer && "constantBuffer が nullptr");
    assert(srv && "srv が nullptr（テクスチャ未ロード）");

    // ------------------------------------------------------------------
    // 1. 頂点バッファ書き込み
    //    ローカル座標は [-0.5, +0.5] の正規化済み単位クワッド。
    //    スケール・回転・移動はすべてシェーダーの行列に任せる。
    // ------------------------------------------------------------------
    // UV 計算
    float uvW = 1.f / sheet.cols;
    float uvH = 1.f / sheet.rows;
    int   col = sheet.index % sheet.cols;
    int   row = sheet.index / sheet.cols;
    float u0 = col * uvW, u1 = u0 + uvW;
    float v0 = row * uvH, v1 = v0 + uvH;

    // ローカル頂点
    const Vertex vertices[4] = {
        { { -0.5f,  0.5f }, color, { u0, v0 } },
        { {  0.5f,  0.5f }, color, { u1, v0 } },
        { { -0.5f, -0.5f }, color, { u0, v1 } },
        { {  0.5f, -0.5f }, color, { u1, v1 } },
    };

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, vertices, sizeof(vertices));
    context->Unmap(vertexBuffer, 0);

    // ------------------------------------------------------------------
    // 2. 定数バッファ（ワールド行列）更新
    // ------------------------------------------------------------------
    ConstantBuffer cb;
    cb.world = BuildWorldMatrix(transform, transform.rotate.y);

    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    // ------------------------------------------------------------------
    // 3. パイプライン設定 & 描画
    // ------------------------------------------------------------------
    float blendFactor[4] = {};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);

    UINT stride = sizeof(Vertex), offset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);   // b0 スロット

    context->PSSetShader(pixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);

    context->Draw(4, 0);
}

void Sprite::DrawUV(Transform transform, XMFLOAT4 color,
    XMFLOAT2 uvMin, XMFLOAT2 uvMax, const SpriteSheet& sheet)
{
    // UV 計算
    float uvW = 1.f / sheet.cols;
    float uvH = 1.f / sheet.rows;
    int   col = sheet.index % sheet.cols;
    int   row = sheet.index / sheet.cols;

    // シート内のこのコマの UV 開始位置
    float sheetU0 = col * uvW;
    float sheetV0 = row * uvH;

    // uvMin/uvMax をシート内のコマの範囲にマッピング
    float u0 = sheetU0 + uvMin.x * uvW;
    float u1 = sheetU0 + uvMax.x * uvW;
    float v0 = sheetV0 + uvMin.y * uvH;
    float v1 = sheetV0 + uvMax.y * uvH;

    const Vertex vertices[4] = {
        { { -0.5f,  0.5f }, color, { u0, v0 } },
        { {  0.5f,  0.5f }, color, { u1, v0 } },
        { { -0.5f, -0.5f }, color, { u0, v1 } },
        { {  0.5f, -0.5f }, color, { u1, v1 } },
    };

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, vertices, sizeof(vertices));
    context->Unmap(vertexBuffer, 0);

    ConstantBuffer cb;
    cb.world = BuildWorldMatrix(transform, transform.rotate.y);
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    float blendFactor[4] = {};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
    UINT stride = sizeof(Vertex), offset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetShader(pixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);
    context->Draw(4, 0);
}

// Sprite.cpp
void Sprite::DrawPolygon(Transform transform, XMFLOAT4 color,
    const std::vector<XMFLOAT2>& localPos,
    const std::vector<XMFLOAT2>& uvs,
    const SpriteSheet& sheet)
{
    if (localPos.size() < 3) return;
    if (localPos.size() != uvs.size()) {
        OutputDebugStringA("★ DrawPolygon: localPos と uvs のサイズが違う\n");
        return;
    }

    float uvW = 1.f / sheet.cols;
    float uvH = 1.f / sheet.rows;
    int   col = sheet.index % sheet.cols;
    int   row = sheet.index / sheet.cols;
    float sheetU0 = col * uvW;
    float sheetV0 = row * uvH;

    std::vector<Vertex> vertices;

    auto MakeVertex = [&](int idx)
        {
            float u = sheetU0 + uvs[idx].x * uvW;
            float v = sheetV0 + uvs[idx].y * uvH;

            return Vertex{
                localPos[idx],
                color,
                {u,v}
            };
        };

    // 扇形分割
    for (int i = 1; i < (int)localPos.size() - 1; i++)
    {
        vertices.push_back(MakeVertex(0));
        vertices.push_back(MakeVertex(i));
        vertices.push_back(MakeVertex(i + 1));
    }

    if (vertices.empty() || vertices.size() > 64) return;

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, vertices.data(), sizeof(Vertex) * vertices.size());
    context->Unmap(vertexBuffer, 0);

    ConstantBuffer cb;
    cb.world = BuildWorldMatrix(transform, transform.rotate.y);
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    float blendFactor[4] = {};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
    UINT stride = sizeof(Vertex), offset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    // ★ TriangleFan で描画
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetShader(pixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);

    context->Draw((UINT)vertices.size(), 0);
}

// ---------------------------------------------------------------------------
// Uninit
// ---------------------------------------------------------------------------
void Sprite::Uninit() {
    if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
    if (blendState) { blendState->Release();     blendState = nullptr; }
    if (sampler) { sampler->Release();         sampler = nullptr; }
    if (inputLayout) { inputLayout->Release();     inputLayout = nullptr; }
    if (pixelShader) { pixelShader->Release();     pixelShader = nullptr; }
    if (vertexShader) { vertexShader->Release();    vertexShader = nullptr; }
    if (vertexBuffer) { vertexBuffer->Release();    vertexBuffer = nullptr; }
}