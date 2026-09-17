#include "Sprite.h"
#include "WICTextureLoader11.h"
#include "Graphics.h"
#include <cassert>

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

    hr = Graphics::device->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) {
        OutputDebugStringA("★ CreateVertexShader 失敗\n");
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    hr = Graphics::device->CreatePixelShader(psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) {
        OutputDebugStringA("★ CreatePixelShader 失敗\n");
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,      0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = Graphics::device->CreateInputLayout(layout, 3,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    if (FAILED(hr)) {
        OutputDebugStringA("★ CreateInputLayout 失敗\n");
        vsBlob->Release(); psBlob->Release();
        return false;
    }

    vsBlob->Release();
    psBlob->Release();
    return true;
}

bool Sprite::CreateBuffers() {
    // ---- 頂点バッファ ----
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(Vertex) * 4;   // スプライト1枚は常に4頂点
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = Graphics::device->CreateBuffer(&bd, nullptr, &vertexBuffer);
    if (FAILED(hr)) return false;

    // ---- 定数バッファ（ワールド行列）----
    // cbuffer は 16 バイト境界が必要。XMMATRIX は 64 バイトなのでそのまま OK
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(ConstantBuffer);   // = sizeof(XMMATRIX) = 64
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
XMMATRIX Sprite::BuildWorldMatrix(const Transform& transform) const {
    float w = (transform.scale.x > 0.f) ? transform.scale.x : (float)texWidth;
    float h = (transform.scale.y > 0.f) ? transform.scale.y : (float)texHeight;

    // ★ カメラとのZ距離による遠近スケール。遠いほど小さく、カメラ中心寄りに見える。
    //   focalLengthの距離にいる時が等倍(depthScale=1)。カメラが無ければ効果なし。
    float depthScale = 1.f;
    if (camera) {
        float depth = transform.position.z - camera->position.z;
        if (depth > 0.f) depthScale = camera->focalLength / depth;
    }
    w *= depthScale;
    h *= depthScale;

    // --- 各変換行列を生成 ---
    // ① スケール行列: ローカル座標 [-0.5, 0.5] → ピクセルサイズに拡大
    XMMATRIX S = XMMatrixScaling(w, h, 1.f);

    // ② 回転行列（Z 軸、度 → ラジアン）
    float rad = transform.rotate.z * (XM_PI / 180.f);
    XMMATRIX R = XMMatrixRotationZ(rad);

    // ③ 平行移動行列: ワールド座標 → カメラからの相対座標 → NDC
    //    カメラのposition(x,y)ぶんだけ引くことで、カメラを動かすと
    //    画面全体がスクロールしているように見える。
    //    さらにdepthScaleを掛けることで、奥にあるものほど中心に寄って見える(視差)
    float camX = camera ? camera->position.x : 0.f;
    float camY = camera ? camera->position.y : 0.f;
    float worldX = (transform.position.x - camX) * depthScale;
    float worldY = (transform.position.y - camY) * depthScale;

    //    NDC_x = pixel_x / (resW / 2)   NDC_y = -pixel_y / (resH / 2)
    float ndcX = worldX / (resW * 0.5f);
    float ndcY = -worldY / (resH * 0.5f);
    XMMATRIX T = XMMatrixTranslation(ndcX, ndcY, 0.f);

    // ④ NDC スケール行列: ローカル [-0.5, 0.5] を NDC スケールに変換する追加係数
    //    （スケール行列でピクセルに拡大した後、さらに NDC へ縮小）
    XMMATRIX NDC = XMMatrixScaling(2.f / resW, 2.f / resH, 1.f);

    // 合成順: Scale → Rotate → NDC_Scale → Translate
    //   ※ HLSL で row_major を使う場合は転置が必要。column_major なら不要。
    //     ここでは XMMatrixTranspose して転送し、シェーダー側は column_major 想定。
    return XMMatrixTranspose(S * R * NDC * T);
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

    // ローカル頂点（単位クワッド）
    const Vertex vertices[4] = {
        { {  0.5f, -0.5f }, color, { u1, v1 } },
        { { -0.5f, -0.5f }, color, { u0, v1 } },
        { {  0.5f,  0.5f }, color, { u1, v0 } },
        { { -0.5f,  0.5f }, color, { u0, v0 } },
    };

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, vertices, sizeof(vertices));
    context->Unmap(vertexBuffer, 0);

    // ------------------------------------------------------------------
    // 2. 定数バッファ（ワールド行列）更新
    // ------------------------------------------------------------------
    ConstantBuffer cb;
    cb.world = BuildWorldMatrix(transform);

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
