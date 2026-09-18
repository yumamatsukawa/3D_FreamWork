#include "Mesh.h"
#include "Graphics.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

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

void Mesh::Draw(const Transform& transform, const Camera& camera, XMFLOAT4 color, const Light& light) {
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
    cb.lightDir = { light.direction.x, light.direction.y, light.direction.z, 0.f };
    cb.lightColor = { light.color.x, light.color.y, light.color.z, 0.f };
    cb.ambient = { light.ambient.x, light.ambient.y, light.ambient.z, 0.f };

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
    context->PSSetConstantBuffers(0, 1, &constantBuffer);
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
