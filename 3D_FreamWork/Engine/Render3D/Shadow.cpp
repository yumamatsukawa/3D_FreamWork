#include "Shadow.h"
#include "../Core/Component.h"
#include "../Core/GameObject.h"
#include "../Render/Graphics.h"
#include "../Render/Image.h"
#include "Lighting.h"
#include "LightComponent.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace Shadow {
    namespace {
        Settings settings;

        struct Entry { Component* component; Caster* caster; };
        std::vector<Entry> casters;   // 登録された影を落とす物(Rendererの Init で追加、Uninit で削除)

        // 影の深度テクスチャ(R32_TYPELESS)。書く時は D32_FLOAT の DSV、読む時は R32_FLOAT の SRV。平行光源用とスポットライト用の2枚
        struct DepthMap {
            ID3D11Texture2D* texture = nullptr;
            ID3D11DepthStencilView* view = nullptr;
            ID3D11ShaderResourceView* srv = nullptr;
            int resolution = 0;
        };
        DepthMap sunMap;    // t1
        DepthMap spotMap;   // t2

        ID3D11SamplerState* compareSampler = nullptr;       // s1: 比較サンプラー(PCFの2x2補間はGPUがやる)
        ID3D11RasterizerState* depthRasterizer = nullptr;   // 影のパス用: 両面描画・深度バイアス
        ID3D11DepthStencilState* depthState = nullptr;      // 影のパス用: 深度テスト・書き込みあり
        ID3D11Buffer* shadowBuffer = nullptr;               // b5: ShadowConstantBuffer

        bool sunActive = false;    // 今フレームに平行光源の影のパスを実行したか(Bind が見る)
        bool spotActive = false;   // 今フレームにスポットライトの影のパスを実行したか
        int drawnCount = 0;

        void ReleaseMap(DepthMap& map) {
            if (map.srv) { map.srv->Release(); map.srv = nullptr; }
            if (map.view) { map.view->Release(); map.view = nullptr; }
            if (map.texture) { map.texture->Release(); map.texture = nullptr; }
            map.resolution = 0;
        }

        // 設定を使える範囲に収める
        Settings Sanitized() {
            Settings s = settings;
            s.resolution = (std::min)((std::max)(s.resolution, 256), 8192);
            s.distance = (std::max)(s.distance, 1.f);
            s.depthBias = (std::max)(s.depthBias, 0.f);
            s.normalBias = (std::max)(s.normalBias, 0.f);
            s.strength = (std::min)((std::max)(s.strength, 0.f), 1.f);
            s.pcfRadius = (std::min)((std::max)(s.pcfRadius, 0), 4);
            s.spotResolution = (std::min)((std::max)(s.spotResolution, 256), 8192);
            s.spotDepthBias = (std::max)(s.spotDepthBias, 0.f);
            s.spotStrength = (std::min)((std::max)(s.spotStrength, 0.f), 1.f);
            return s;
        }

        // 比較サンプラー(s1)を用意する。影を使わないフレームも、描く物は s1 を読むシェーダーなので、必ずセットしておく
        bool EnsureSampler() {
            if (compareSampler) return true;
            if (!Graphics::device) return false;
            // 「ピクセルの深度 <= 影の深度」なら1(明るい)。範囲の外は1(影なし)
            D3D11_SAMPLER_DESC sd = {};
            sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
            sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 1.f;
            sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
            sd.MaxLOD = D3D11_FLOAT32_MAX;
            return SUCCEEDED(Graphics::device->CreateSamplerState(&sd, &compareSampler));
        }

        // 影の深度テクスチャ(解像度が変わっていたら作り直す)
        bool EnsureMap(DepthMap& map, int resolution) {
            if (map.texture && map.resolution == resolution) return true;
            ReleaseMap(map);

            D3D11_TEXTURE2D_DESC td = {};
            td.Width = resolution;
            td.Height = resolution;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = DXGI_FORMAT_R32_TYPELESS;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            if (FAILED(Graphics::device->CreateTexture2D(&td, nullptr, &map.texture))) {
                OutputDebugStringA("★ Shadow: 影のテクスチャの作成に失敗\n");
                return false;
            }

            D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};
            dsvd.Format = DXGI_FORMAT_D32_FLOAT;
            dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
            srvd.Format = DXGI_FORMAT_R32_FLOAT;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;
            if (FAILED(Graphics::device->CreateDepthStencilView(map.texture, &dsvd, &map.view)) ||
                FAILED(Graphics::device->CreateShaderResourceView(map.texture, &srvd, &map.srv))) {
                OutputDebugStringA("★ Shadow: 影のテクスチャのビューの作成に失敗\n");
                ReleaseMap(map);
                return false;
            }
            map.resolution = resolution;
            return true;
        }

        // ステート・バッファを用意する
        bool EnsureStates() {
            if (!Graphics::device) return false;
            if (!EnsureSampler()) return false;
            if (!depthRasterizer) {
                // 両面を描く(薄い板や、裏側から見える面も影になるように)。傾いた面のアクネを減らすため、傾きに応じた深度バイアスを付ける
                D3D11_RASTERIZER_DESC rd = {};
                rd.FillMode = D3D11_FILL_SOLID;
                rd.CullMode = D3D11_CULL_NONE;
                rd.DepthClipEnable = TRUE;
                rd.SlopeScaledDepthBias = 1.5f;
                if (FAILED(Graphics::device->CreateRasterizerState(&rd, &depthRasterizer))) return false;
            }
            if (!depthState) {
                D3D11_DEPTH_STENCIL_DESC dsd = {};
                dsd.DepthEnable = TRUE;
                dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                dsd.DepthFunc = D3D11_COMPARISON_LESS;
                if (FAILED(Graphics::device->CreateDepthStencilState(&dsd, &depthState))) return false;
            }
            if (!shadowBuffer) {
                D3D11_BUFFER_DESC bd = {};
                bd.Usage = D3D11_USAGE_DYNAMIC;
                bd.ByteWidth = sizeof(ShadowConstantBuffer);
                bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                if (FAILED(Graphics::device->CreateBuffer(&bd, nullptr, &shadowBuffer))) return false;
            }
            return true;
        }

        // 1枚の深度テクスチャに、影を落とす物を全部描く(アクティブで有効な物だけ)。描いた数を返す
        int DrawCasters(const DepthMap& map, const LightView& lightView) {
            ID3D11DeviceContext* context = Graphics::context;
            context->OMSetRenderTargets(0, nullptr, map.view);   // 色の描画先は無し。深度だけを書く
            context->ClearDepthStencilView(map.view, D3D11_CLEAR_DEPTH, 1.f, 0);
            D3D11_VIEWPORT viewport = {};
            viewport.Width = (float)map.resolution;
            viewport.Height = (float)map.resolution;
            viewport.MaxDepth = 1.f;
            context->RSSetViewports(1, &viewport);
            context->RSSetState(depthRasterizer);
            context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(depthState, 0);

            // 描く途中で登録が増減しても壊れないよう、コピーを回す
            int drawn = 0;
            std::vector<Entry> snapshot = casters;
            for (const Entry& entry : snapshot) {
                if (!entry.component->IsEnabled()) continue;
                GameObject* owner = entry.component->GetOwner();
                if (!owner || !owner->GetIsActive()) continue;
                entry.caster->DrawShadow(lightView);
                drawn++;
            }
            return drawn;
        }
    }

    void SetSettings(const Settings& value) {
        settings = value;
    }

    Settings& GetSettings() {
        return settings;
    }

    void Register(Component* component, Caster* caster) {
        if (!component || !caster) return;
        for (const Entry& entry : casters) {
            if (entry.caster == caster) return;   // 二重登録しない
        }
        casters.push_back({ component, caster });
    }

    void Unregister(Caster* caster) {
        casters.erase(std::remove_if(casters.begin(), casters.end(),
            [caster](const Entry& entry) { return entry.caster == caster; }), casters.end());
    }

    LightView BuildSunView(const Camera& camera, const Light& light, float distance, int resolution) {
        // 光の向き(長さ0なら真下)と、光から見た「上」(真下に近い向きの時は別の軸にする)
        XMVECTOR dir = XMLoadFloat3(&light.direction);
        if (XMVectorGetX(XMVector3LengthSq(dir)) < 1e-8f) dir = XMVectorSet(0.f, -1.f, 0.f, 0.f);
        dir = XMVector3Normalize(dir);
        XMVECTOR up = (fabsf(XMVectorGetY(dir)) > 0.99f) ? XMVectorSet(0.f, 0.f, 1.f, 0.f) : XMVectorSet(0.f, 1.f, 0.f, 0.f);

        // 影の範囲の中心: カメラの前方、distance の半分の所
        XMVECTOR eye = XMLoadFloat3(&camera.position);
        XMVECTOR forward = XMLoadFloat3(&camera.target) - eye;
        if (XMVectorGetX(XMVector3LengthSq(forward)) < 1e-8f) forward = XMVectorSet(0.f, 0.f, 1.f, 0.f);
        forward = XMVector3Normalize(forward);
        float half = distance * 0.5f;
        XMVECTOR center = eye + forward * half;

        // 光の向きだけを向いた行列(原点が基準)で中心を測り、縦横をテクセルの大きさの整数倍に丸める。
        // 丸めないと、カメラが動く度に影の縁がテクセル1つ分ずつずれてちらつく
        XMMATRIX rotation = XMMatrixLookToLH(XMVectorZero(), dir, up);
        XMVECTOR centerInLight = XMVector3TransformCoord(center, rotation);
        float texel = distance / (float)resolution;
        float snappedX = floorf(XMVectorGetX(centerInLight) / texel + 0.5f) * texel;
        float snappedY = floorf(XMVectorGetY(centerInLight) / texel + 0.5f) * texel;

        // 中心が、箱の真ん中(縦横は丸めた位置、奥行きは distance)に来るようにずらす。
        // 奥行きは 0〜2×distance を使う(中心より光に近い側にある、高い所の物も影を落とせるように)
        LightView result;
        result.view = rotation * XMMatrixTranslation(-snappedX, -snappedY, distance - XMVectorGetZ(centerInLight));
        result.proj = XMMatrixOrthographicLH(distance, distance, 0.f, distance * 2.f);
        return result;
    }

    LightView BuildSpotView(XMFLOAT3 position, XMFLOAT3 direction, float outerAngleDegrees, float range) {
        XMVECTOR dir = XMLoadFloat3(&direction);
        if (XMVectorGetX(XMVector3LengthSq(dir)) < 1e-8f) dir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
        dir = XMVector3Normalize(dir);
        XMVECTOR up = (fabsf(XMVectorGetY(dir)) > 0.99f) ? XMVectorSet(0.f, 0.f, 1.f, 0.f) : XMVectorSet(0.f, 1.f, 0.f, 0.f);

        // 視野 = 円すいの外側の角度の2倍(正方形の画面に、円すいがちょうど収まる)。広すぎると深度の精度が足りないので120度まで
        float halfAngle = (std::min)((std::max)(outerAngleDegrees, 1.f), 60.f);
        float nearZ = (std::max)(range * 0.01f, 1.f);   // 近すぎると深度の精度が落ちるので、届く距離の1%(最低1)
        float farZ = (std::max)(range, nearZ + 1.f);

        LightView result;
        result.view = XMMatrixLookToLH(XMLoadFloat3(&position), dir, up);
        result.proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(halfAngle * 2.f), 1.f, nearZ, farZ);
        return result;
    }

    void Render() {
        sunActive = false;
        spotActive = false;
        drawnCount = 0;
        if (!settings.enabled) {
            // 無効の間は、影のテクスチャを持たない(メモリを返す。もう一度有効にした時に作り直す)
            ReleaseMap(sunMap);
            ReleaseMap(spotMap);
            return;
        }
        if (!Graphics::device || !Graphics::context) return;

        Settings s = Sanitized();
        if (!EnsureStates() || !EnsureMap(sunMap, s.resolution)) return;
        ID3D11DeviceContext* context = Graphics::context;

        // 光から見た行列と、本番のシェーダーに渡す内容(平行光源)
        LightView sunView = BuildSunView(Image::GetCamera3D(), Image::GetLight(), s.distance, s.resolution);
        ShadowConstantBuffer cb = {};
        cb.viewProj = XMMatrixTranspose(sunView.view * sunView.proj);
        cb.params = { 1.f, s.depthBias, s.normalBias, s.strength };
        cb.params2 = { 1.f / (float)s.resolution, (float)s.pcfRadius, 0.f, 0.f };

        // 影を落とすスポットライト(Lighting::Update が選んだ1つ)があれば、その光から見た行列も作る
        const LightComponent* spotLight = Lighting::GetShadowSpotLight();
        bool wantSpot = spotLight && spotLight->GetOwner() && spotLight->GetOwner()->GetIsActive() && EnsureMap(spotMap, s.spotResolution);
        if (!wantSpot) ReleaseMap(spotMap);   // 影を作るスポットが無い間は、スポット用のテクスチャを持たない
        LightView spotView = {};
        cb.spotViewProj = XMMatrixIdentity();   // スポットの影が無い時は使われない
        if (wantSpot) {
            spotView = BuildSpotView(spotLight->GetWorldPosition(), spotLight->GetWorldDirection(), spotLight->GetOuterAngle(), spotLight->GetRange());
            float spotNear = (std::max)(spotLight->GetRange() * 0.01f, 1.f);   // BuildSpotView と同じ値
            float spotFar = (std::max)(spotLight->GetRange(), spotNear + 1.f);
            cb.spotViewProj = XMMatrixTranspose(spotView.view * spotView.proj);
            cb.spotParams = { 1.f, s.spotDepthBias, spotNear, spotFar };
            cb.spotParams2 = { 1.f / (float)s.spotResolution, (float)s.pcfRadius, s.normalBias, s.spotStrength };
        }
        D3D11_MAPPED_SUBRESOURCE msr;
        if (FAILED(context->Map(shadowBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) return;
        memcpy(msr.pData, &cb, sizeof(cb));
        context->Unmap(shadowBuffer, 0);

        // 今の描画先・ビューポート・ステートを覚えておく(パスの後に必ず元へ戻す)
        ID3D11RenderTargetView* oldTarget = nullptr;
        ID3D11DepthStencilView* oldDepth = nullptr;
        context->OMGetRenderTargets(1, &oldTarget, &oldDepth);
        UINT viewportCount = 1;
        D3D11_VIEWPORT oldViewport = {};
        context->RSGetViewports(&viewportCount, &oldViewport);
        ID3D11RasterizerState* oldRasterizer = nullptr;
        context->RSGetState(&oldRasterizer);
        ID3D11BlendState* oldBlend = nullptr;
        float oldBlendFactor[4] = {};
        UINT oldSampleMask = 0xFFFFFFFF;
        context->OMGetBlendState(&oldBlend, oldBlendFactor, &oldSampleMask);
        ID3D11DepthStencilState* oldDepthState = nullptr;
        UINT oldStencilRef = 0;
        context->OMGetDepthStencilState(&oldDepthState, &oldStencilRef);
        ID3D11PixelShader* oldPixelShader = nullptr;
        context->PSGetShader(&oldPixelShader, nullptr, nullptr);

        // 影の深度テクスチャを「読む」バインド(t1・t2)を外してから「書く」(同じ物を同時に読み書きしない)
        ID3D11ShaderResourceView* nullSrvs[2] = { nullptr, nullptr };
        context->PSSetShaderResources(1, 2, nullSrvs);

        // 影のパス: 平行光源 → (あれば)スポットライト
        drawnCount += DrawCasters(sunMap, sunView);
        if (wantSpot) drawnCount += DrawCasters(spotMap, spotView);

        // 元に戻す(描画先・ビューポート・ステート)。戻し忘れると画面が真っ黒になる
        context->OMSetRenderTargets(oldTarget ? 1 : 0, &oldTarget, oldDepth);
        context->RSSetViewports(1, &oldViewport);
        context->RSSetState(oldRasterizer);
        context->OMSetBlendState(oldBlend, oldBlendFactor, oldSampleMask);
        context->OMSetDepthStencilState(oldDepthState, oldStencilRef);
        context->PSSetShader(oldPixelShader, nullptr, 0);   // 影のパスでは外している(Mesh::DrawDepth)ので戻す
        if (oldPixelShader) oldPixelShader->Release();
        if (oldTarget) oldTarget->Release();
        if (oldDepth) oldDepth->Release();
        if (oldRasterizer) oldRasterizer->Release();
        if (oldBlend) oldBlend->Release();
        if (oldDepthState) oldDepthState->Release();

        sunActive = true;
        spotActive = wantSpot;
    }

    void Bind() {
        ID3D11DeviceContext* context = Graphics::context;
        if (!context) return;
        // 影を使わないフレームは nullptr をセットする(シェーダーは0として読むので、影は無し=今までと同じ見た目)
        ID3D11Buffer* buffer = sunActive ? shadowBuffer : nullptr;
        ID3D11ShaderResourceView* srvs[2] = { sunActive ? sunMap.srv : nullptr, spotActive ? spotMap.srv : nullptr };
        EnsureSampler();   // 比較サンプラーは影を使わない間もセットしておく(s1を読むシェーダーに、比較用でないサンプラーを渡すとデバッグレイヤーが警告する)
        ID3D11SamplerState* sampler = compareSampler;
        context->PSSetConstantBuffers(5, 1, &buffer);
        context->PSSetShaderResources(1, 2, srvs);
        context->PSSetSamplers(1, 1, &sampler);
    }

    void Shutdown() {
        ReleaseMap(sunMap);
        ReleaseMap(spotMap);
        if (compareSampler) { compareSampler->Release(); compareSampler = nullptr; }
        if (depthRasterizer) { depthRasterizer->Release(); depthRasterizer = nullptr; }
        if (depthState) { depthState->Release(); depthState = nullptr; }
        if (shadowBuffer) { shadowBuffer->Release(); shadowBuffer = nullptr; }
        casters.clear();
        settings = Settings();
        sunActive = false;
        spotActive = false;
        drawnCount = 0;
    }

    int GetCasterCount() {
        return (int)casters.size();
    }

    int GetDrawnCount() {
        return drawnCount;
    }

    bool IsSpotShadowActive() {
        return spotActive;
    }
}
