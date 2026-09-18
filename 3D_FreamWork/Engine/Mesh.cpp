#include "Mesh.h"
#include "Graphics.h"
#include <cassert>

bool Mesh::Init(const std::vector<MeshVertex>& verts) {
    context = Graphics::context;
    vertices = verts;
    aspectRatio = (Graphics::height > 0.f) ? (Graphics::width / Graphics::height) : 1.f;

    if (!CreateShaders()) { OutputDebugStringA("★ Mesh CreateShaders 失敗\n"); return false; }
    if (!CreateBuffers()) { OutputDebugStringA("★ Mesh CreateBuffers 失敗\n"); return false; }
    if (!CreateSampler()) { OutputDebugStringA("★ Mesh CreateSampler 失敗\n"); return false; }
    if (!CreateBlendState()) { OutputDebugStringA("★ Mesh CreateBlendState 失敗\n"); return false; }
    if (!CreateDepthStencilState()) { OutputDebugStringA("★ Mesh CreateDepthStencilState 失敗\n"); return false; }
    if (!CreateRasterizerState()) { OutputDebugStringA("★ Mesh CreateRasterizerState 失敗\n"); return false; }
    return true;
}

bool Mesh::CreateShaders() {
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* err = nullptr;

    HRESULT hr = D3DCompileFromFile(L"Shader3D.hlsl", nullptr, nullptr,
        "VS", "vs_5_0", 0, 0, &vsBlob, &err);
    if (FAILED(hr)) {
        if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
        else { OutputDebugStringA("★ Shader3D.hlsl が見つかりません\n"); }
        return false;
    }

    err = nullptr;
    hr = D3DCompileFromFile(L"Shader3D.hlsl", nullptr, nullptr,
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

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = Graphics::device->CreateInputLayout(layout, 3,
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

bool Mesh::CreateBuffers() {
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(MeshVertex) * (UINT)vertices.size();
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = Graphics::device->CreateBuffer(&bd, nullptr, &vertexBuffer);
    if (FAILED(hr)) return false;

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(MeshConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = Graphics::device->CreateBuffer(&cbd, nullptr, &constantBuffer);
    return SUCCEEDED(hr);
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
    return SUCCEEDED(hr);
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

void Mesh::Draw(const Transform& transform, const Camera& camera, XMFLOAT4 color) {
    assert(context && "context が nullptr");
    assert(vertexBuffer && "vertexBuffer が nullptr");

    // 頂点の色を指定色で上書きしてから送る(SpriteRenderer::SetColorと同じ使い勝手にするため)
    std::vector<MeshVertex> coloredVerts = vertices;
    for (auto& v : coloredVerts) v.Color = color;

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, coloredVerts.data(), sizeof(MeshVertex) * coloredVerts.size());
    context->Unmap(vertexBuffer, 0);

    // ★ World/View/Projectionをそれぞれ個別に転置して送る。
    //   シェーダー側はmul(vector, matrix)の順で使う(2DのShader.hlslと同じ考え方)
    MeshConstantBuffer cb;
    cb.world = XMMatrixTranspose(transform.GetWorldMatrix());
    cb.view = XMMatrixTranspose(camera.GetViewMatrix());
    cb.proj = XMMatrixTranspose(camera.GetProjectionMatrix(aspectRatio));

    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    float blendFactor[4] = {};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilState, 0);
    context->RSSetState(rasterizerState);

    UINT stride = sizeof(MeshVertex), offset = 0;
    context->IASetInputLayout(inputLayout);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);

    context->PSSetShader(pixelShader, nullptr, 0);
    if (srv) context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);

    context->Draw((UINT)coloredVerts.size(), 0);

    // 次に2D(Sprite)が描画される時のために、ラスタライザ状態を既定に戻しておく
    context->RSSetState(nullptr);
}

void Mesh::Uninit() {
    if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
    if (blendState) { blendState->Release(); blendState = nullptr; }
    if (depthStencilState) { depthStencilState->Release(); depthStencilState = nullptr; }
    if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
    if (sampler) { sampler->Release(); sampler = nullptr; }
    if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
    if (pixelShader) { pixelShader->Release(); pixelShader = nullptr; }
    if (vertexShader) { vertexShader->Release(); vertexShader = nullptr; }
    if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
}

std::vector<MeshVertex> Mesh::CreateCube() {
    XMFLOAT4 c = { 1.f, 1.f, 1.f, 1.f };
    return {
        // front (z = -0.5)
        {{-0.5f,-0.5f,-0.5f}, c, {0.f,1.f}}, {{ 0.5f,-0.5f,-0.5f}, c, {1.f,1.f}}, {{ 0.5f, 0.5f,-0.5f}, c, {1.f,0.f}},
        {{-0.5f,-0.5f,-0.5f}, c, {0.f,1.f}}, {{ 0.5f, 0.5f,-0.5f}, c, {1.f,0.f}}, {{-0.5f, 0.5f,-0.5f}, c, {0.f,0.f}},
        // back (z = 0.5)
        {{ 0.5f,-0.5f, 0.5f}, c, {0.f,1.f}}, {{-0.5f,-0.5f, 0.5f}, c, {1.f,1.f}}, {{-0.5f, 0.5f, 0.5f}, c, {1.f,0.f}},
        {{ 0.5f,-0.5f, 0.5f}, c, {0.f,1.f}}, {{-0.5f, 0.5f, 0.5f}, c, {1.f,0.f}}, {{ 0.5f, 0.5f, 0.5f}, c, {0.f,0.f}},
        // left (x = -0.5)
        {{-0.5f,-0.5f, 0.5f}, c, {0.f,1.f}}, {{-0.5f,-0.5f,-0.5f}, c, {1.f,1.f}}, {{-0.5f, 0.5f,-0.5f}, c, {1.f,0.f}},
        {{-0.5f,-0.5f, 0.5f}, c, {0.f,1.f}}, {{-0.5f, 0.5f,-0.5f}, c, {1.f,0.f}}, {{-0.5f, 0.5f, 0.5f}, c, {0.f,0.f}},
        // right (x = 0.5)
        {{ 0.5f,-0.5f,-0.5f}, c, {0.f,1.f}}, {{ 0.5f,-0.5f, 0.5f}, c, {1.f,1.f}}, {{ 0.5f, 0.5f, 0.5f}, c, {1.f,0.f}},
        {{ 0.5f,-0.5f,-0.5f}, c, {0.f,1.f}}, {{ 0.5f, 0.5f, 0.5f}, c, {1.f,0.f}}, {{ 0.5f, 0.5f,-0.5f}, c, {0.f,0.f}},
        // top (y = 0.5)
        {{-0.5f, 0.5f,-0.5f}, c, {0.f,1.f}}, {{ 0.5f, 0.5f,-0.5f}, c, {1.f,1.f}}, {{ 0.5f, 0.5f, 0.5f}, c, {1.f,0.f}},
        {{-0.5f, 0.5f,-0.5f}, c, {0.f,1.f}}, {{ 0.5f, 0.5f, 0.5f}, c, {1.f,0.f}}, {{-0.5f, 0.5f, 0.5f}, c, {0.f,0.f}},
        // bottom (y = -0.5)
        {{-0.5f,-0.5f, 0.5f}, c, {0.f,1.f}}, {{ 0.5f,-0.5f, 0.5f}, c, {1.f,1.f}}, {{ 0.5f,-0.5f,-0.5f}, c, {1.f,0.f}},
        {{-0.5f,-0.5f, 0.5f}, c, {0.f,1.f}}, {{ 0.5f,-0.5f,-0.5f}, c, {1.f,0.f}}, {{-0.5f,-0.5f,-0.5f}, c, {0.f,0.f}},
    };
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
        return MeshVertex{ { x * radius, y * radius, z * radius }, c, { u, v } };
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
