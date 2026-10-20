#include "Sprite.h"
#include "../ThirdParty/WICTextureLoader11.h"
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
    if (!CreateDepthStencilState()) { OutputDebugStringA("★ CreateDepthStencilState 失敗\n"); return false; }
    if (!CreateRasterizerState()) { OutputDebugStringA("★ CreateRasterizerState 失敗\n"); return false; }
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
    if (FAILED(hr)) return false;

    // 加算合成: 描く色をそのまま背景に足す(重なるほど明るくなる)
    D3D11_BLEND_DESC additive = bd;
    additive.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    additive.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    hr = Graphics::device->CreateBlendState(&additive, &blendStateAdditive);
    if (FAILED(hr)) return false;

    // ふわっとした合成: 描く色(RGB)を「濃さ」として使う。
    //   結果 = blendFactor(色) × 描く色 + 背景 × (1 - 描く色)
    // Draw側で、描く色=画像の明るさ×濃さ、blendFactor=粒の色 にしておくと、普通のアルファ合成と同じ見た目になる
    D3D11_BLEND_DESC soft = bd;
    soft.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
    soft.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
    hr = Graphics::device->CreateBlendState(&soft, &blendStateSoft);
    return SUCCEEDED(hr);
}

bool Sprite::CreateDepthStencilState() {
    // UIモード: 深度テスト・深度書き込みを両方オフにする(今まで通り描画順で重なる、常に手前)
    D3D11_DEPTH_STENCIL_DESC dsdUI = {};
    dsdUI.DepthEnable = FALSE;
    dsdUI.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsdUI.DepthFunc = D3D11_COMPARISON_ALWAYS;
    HRESULT hr = Graphics::device->CreateDepthStencilState(&dsdUI, &depthStencilStateUI);
    if (FAILED(hr)) return false;

    // Worldモード: 3Dメッシュ(Mesh)と同じ深度テストを行い、奥行きで正しく前後する
    D3D11_DEPTH_STENCIL_DESC dsdWorld = {};
    dsdWorld.DepthEnable = TRUE;
    dsdWorld.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    // LESS_EQUALにしておく: 同じ深度に重なる2つのWorldモード描画があった時、
    // 単純なLESSだと「後から描く側」が同値深度で負けて見えなくなってしまうため
    dsdWorld.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    hr = Graphics::device->CreateDepthStencilState(&dsdWorld, &depthStencilStateWorld);
    if (FAILED(hr)) return false;

    // Worldモード(深度を書かない): 手前の物には隠れるが、自分は深度を書かない(半透明の粒が後ろの粒を隠さないように)
    D3D11_DEPTH_STENCIL_DESC dsdNoWrite = dsdWorld;
    dsdNoWrite.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    hr = Graphics::device->CreateDepthStencilState(&dsdNoWrite, &depthStencilStateWorldNoWrite);
    return SUCCEEDED(hr);
}

bool Sprite::CreateRasterizerState() {
    // Worldモードのスプライトは、固定の向きのままカメラが裏に回り込んだり、
    // ビルボードが真横に近い角度から見られたりすることがあるので、
    // カリングしない(裏面も描画する。Mesh::CreateRasterizerStateと同じ理由)
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    HRESULT hr = Graphics::device->CreateRasterizerState(&rd, &rasterizerStateWorld);
    return SUCCEEDED(hr);
}

// ---------------------------------------------------------------------------
// BuildWorldMatrix
//   Transform → ワールド行列（平行移動・回転・スケール）を組み立てる
//   最終的に NDC 変換まで含めた行列を返す
// ---------------------------------------------------------------------------
XMMATRIX Sprite::BuildWorldMatrix(const Transform& transform, SpriteMode mode,
    bool lockX, bool lockY, bool lockZ) const {
    // Worldモードは、3Dメッシュ(Mesh)と全く同じcamera3DのView/Projection行列を使う
    // 「常にカメラの方を向く板(ビルボード)」として描画する(CameraControllerでカメラを
    // 動かしても、2Dオブジェクトが3Dオブジェクトと同じように正しく追従して見える)
    // (UI・World2Dは、下の2D用の計算を使う)
    if (mode == SpriteMode::World3D && camera3D) {
        return BuildBillboardMatrix(transform, lockX, lockY, lockZ);
    }

    // ★ UIモード、とWorld2Dモードは、zで大きさを変えない(遠近なし)
    float w = (transform.scale.x > 0.f) ? transform.scale.x : (float)texWidth;
    float h = (transform.scale.y > 0.f) ? transform.scale.y : (float)texHeight;

    // --- 各変換行列を生成 ---
    // ① スケール行列: ローカル座標 [-0.5, 0.5] → ピクセルサイズに拡大
    XMMATRIX S = XMMatrixScaling(w, h, 1.f);

    // ② 回転行列（Z 軸、度 → ラジアン）
    float rad = transform.rotate.z * (XM_PI / 180.f);
    XMMATRIX R = XMMatrixRotationZ(rad);

    // ③ 平行移動行列: ワールド座標 → カメラからの相対座標 → NDC
    //    カメラのposition(x,y)ぶんだけ引くことで、カメラを動かすと
    //    画面全体がスクロールしているように見える。
    float camX = camera ? camera->position.x : 0.f;
    float camY = camera ? camera->position.y : 0.f;
    float worldX = transform.position.x - camX;
    float worldY = transform.position.y - camY;

    //    NDC_x = pixel_x / (resW / 2)   NDC_y = pixel_y / (resH / 2)
    //    ★ position.yはY+が上方向(標準的な数学と同じ、3D側と統一した慣習)なので、
    //      NDC(こちらもY+が上方向)への変換に符号反転は不要
    float ndcX = worldX / (resW * 0.5f);
    float ndcY = worldY / (resH * 0.5f);

    // ★ World2Dモードの時だけ、深度バッファに書き込むためのZ値(0〜1)を計算する。
    //   3D用カメラ(camera3D)からの距離を、Meshと同じ透視投影の式でNDC深度に変換する(zが小さいほど手前)。
    //   見た目の大きさは変えず、前後の判定にだけ使う。
    //   UIモードの時は深度テスト自体をしないので、0のままでよい
    float ndcZ = 0.f;
    if (mode == SpriteMode::World2D && camera3D) {
        float viewZ = transform.position.z - camera3D->position.z;
        if (viewZ > 0.f) {
            float nearZ = camera3D->nearZ;
            float farZ = camera3D->farZ;
            ndcZ = (farZ / (farZ - nearZ)) * (1.0f - nearZ / viewZ);
        }
    }
    XMMATRIX T = XMMatrixTranslation(ndcX, ndcY, ndcZ);

    // ④ NDC スケール行列: ローカル [-0.5, 0.5] を NDC スケールに変換する追加係数
    //    （スケール行列でピクセルに拡大した後、さらに NDC へ縮小）
    XMMATRIX NDC = XMMatrixScaling(2.f / resW, 2.f / resH, 1.f);

    // 合成順: Scale → Rotate → NDC_Scale → Translate
    //   ※ HLSL で row_major を使う場合は転置が必要。column_major なら不要。
    //     ここでは XMMatrixTranspose して転送し、シェーダー側は column_major 想定。
    return XMMatrixTranspose(S * R * NDC * T);
}

// ---------------------------------------------------------------------------
// BuildBillboardMatrix
//   Worldモード用。ローカルの板(quad)を、camera3Dの方を向くように向きを合わせ、
//   実際のView/Projection行列で変換する(Mesh::Drawと同じ考え方)。
//   lockX/lockY/lockZ: trueにした軸はカメラに合わせて回転させず固定する
// ---------------------------------------------------------------------------
XMMATRIX Sprite::BuildBillboardMatrix(const Transform& transform, bool lockX, bool lockY, bool lockZ) const {
    float w = (transform.scale.x > 0.f) ? transform.scale.x : (float)texWidth;
    float h = (transform.scale.y > 0.f) ? transform.scale.y : (float)texHeight;

    // ① スケール行列: ローカル座標[-0.5, 0.5] → ワールド単位の大きさへ(Meshと同じ単位系)
    XMMATRIX S = XMMatrixScaling(w, h, 1.f);

    // ② 板の中での見た目の回転(プレイヤーの向きなど)。ビルボード化の前にかけておくことで、
    //    画面上での回転として機能する
    float rad = transform.rotate.z * (XM_PI / 180.f);
    XMMATRIX R = XMMatrixRotationZ(rad);

    // ③ ビルボード回転: カメラの視線方向をyaw(Y軸)・pitch(X軸)に分解し、
    //    ロックした軸は0(回転しない)にしてから組み直す。
    //    このエンジンのカメラはロール(Z軸回転)を持たないため、lockZは今のところ効果が無い
    //    (将来カメラがロールを持つようになった時のために引数だけ用意してある)
    XMVECTOR eye = XMLoadFloat3(&camera3D->position);
    XMVECTOR at = XMLoadFloat3(&camera3D->target);
    XMVECTOR forwardVec = XMVector3Normalize(at - eye);
    XMFLOAT3 forward;
    XMStoreFloat3(&forward, forwardVec);

    float yaw = lockY ? 0.f : atan2f(forward.x, forward.z);
    float pitch = lockX ? 0.f : atan2f(-forward.y, sqrtf(forward.x * forward.x + forward.z * forward.z));
    float roll = 0.f;
    (void)lockZ; // ロール自体が無いので、ロックしてもしなくても結果は変わらない

    XMMATRIX billboard = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

    // ④ ワールド位置への移動。2Dも3DもY+が上方向で統一されているので、符号反転は不要
    XMMATRIX T = XMMatrixTranslation(transform.position.x, transform.position.y, transform.position.z);

    XMMATRIX view = camera3D->GetViewMatrix();
    float aspectRatio = (resH > 0.f) ? (resW / resH) : 1.f;
    XMMATRIX proj = camera3D->GetProjectionMatrix(aspectRatio);

    return XMMatrixTranspose(S * R * billboard * T * view * proj);
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------
void Sprite::Draw(Transform transform, XMFLOAT4 color, const SpriteSheet& sheet, bool worldSpace,
    bool lockX, bool lockY, bool lockZ, BlendMode blend, bool depthWrite) {
    Draw(transform, color, sheet, worldSpace ? SpriteMode::World3D : SpriteMode::UI, lockX, lockY, lockZ, blend, depthWrite);
}

void Sprite::Draw(Transform transform, XMFLOAT4 color, const SpriteSheet& sheet, SpriteMode mode,
    bool lockX, bool lockY, bool lockZ, BlendMode blend, bool depthWrite) {
    const bool worldSpace = (mode != SpriteMode::UI);   // 深度を使うか(World2D・World3D)
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

    // ★ コマの境界を半テクセルだけ内側に絞る。バイリニアフィルタリングは
    //   サンプリング位置がコマの境界ギリギリだと隣のコマ(や余白の背景色)を
    //   巻き込んでにじんでしまうため(特に斜めから見て縮小される時に目立つ)
    if (texWidth > 0 && texHeight > 0) {
        float insetU = 0.5f / (float)texWidth;
        float insetV = 0.5f / (float)texHeight;
        u0 += insetU; u1 -= insetU;
        v0 += insetV; v1 -= insetV;
    }

    // 重ね方に合わせて、頂点の色とブレンドの係数を決める
    // (Additive/Softはアルファを1のままにして、色の明るさで濃さを表す。理由はBlendModeのコメント参照)
    XMFLOAT4 vertexColor = color;
    float blendFactor[4] = {};
    ID3D11BlendState* currentBlendState = blendState;
    if (blend == BlendMode::Additive) {
        vertexColor = { color.x * color.w, color.y * color.w, color.z * color.w, 1.f };
        currentBlendState = blendStateAdditive;
    }
    else if (blend == BlendMode::Soft) {
        vertexColor = { color.w, color.w, color.w, 1.f };
        blendFactor[0] = color.x; blendFactor[1] = color.y; blendFactor[2] = color.z; blendFactor[3] = 1.f;
        currentBlendState = blendStateSoft;
    }

    // ローカル頂点（単位クワッド）
    const Vertex vertices[4] = {
        { {  0.5f, -0.5f }, vertexColor, { u1, v1 } },
        { { -0.5f, -0.5f }, vertexColor, { u0, v1 } },
        { {  0.5f,  0.5f }, vertexColor, { u1, v0 } },
        { { -0.5f,  0.5f }, vertexColor, { u0, v0 } },
    };

    D3D11_MAPPED_SUBRESOURCE msr;
    context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, vertices, sizeof(vertices));
    context->Unmap(vertexBuffer, 0);

    // ------------------------------------------------------------------
    // 2. 定数バッファ（ワールド行列）更新
    // ------------------------------------------------------------------
    ConstantBuffer cb;
    cb.world = BuildWorldMatrix(transform, mode, lockX, lockY, lockZ);

    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, &cb, sizeof(cb));
    context->Unmap(constantBuffer, 0);

    // ------------------------------------------------------------------
    // 3. パイプライン設定 & 描画
    // ------------------------------------------------------------------
    context->OMSetBlendState(currentBlendState, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(
        worldSpace ? (depthWrite ? depthStencilStateWorld : depthStencilStateWorldNoWrite) : depthStencilStateUI, 0);
    // Worldモードはカリング無効(裏面も描画)、UIモードは既定(裏面カリングあり)のまま
    context->RSSetState(worldSpace ? rasterizerStateWorld : nullptr);

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
    if (blendStateAdditive) { blendStateAdditive->Release(); blendStateAdditive = nullptr; }
    if (blendStateSoft) { blendStateSoft->Release(); blendStateSoft = nullptr; }
    if (depthStencilStateWorldNoWrite) { depthStencilStateWorldNoWrite->Release(); depthStencilStateWorldNoWrite = nullptr; }
    if (depthStencilStateUI) { depthStencilStateUI->Release(); depthStencilStateUI = nullptr; }
    if (depthStencilStateWorld) { depthStencilStateWorld->Release(); depthStencilStateWorld = nullptr; }
    if (rasterizerStateWorld) { rasterizerStateWorld->Release(); rasterizerStateWorld = nullptr; }
    if (sampler) { sampler->Release();         sampler = nullptr; }
    if (inputLayout) { inputLayout->Release();     inputLayout = nullptr; }
    if (pixelShader) { pixelShader->Release();     pixelShader = nullptr; }
    if (vertexShader) { vertexShader->Release();    vertexShader = nullptr; }
    if (vertexBuffer) { vertexBuffer->Release();    vertexBuffer = nullptr; }
}
