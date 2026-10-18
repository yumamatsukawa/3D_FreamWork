#include "Lighting.h"
#include "LightComponent.h"
#include "../Core/GameObject.h"
#include "../Render/Graphics.h"
#include "../Render/Image.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace Lighting {
    namespace {
        std::vector<LightComponent*> registered;     // 登録された光(LightComponentのInitで追加、Uninitで削除)
        ID3D11Buffer* lightBuffer = nullptr;         // b4: LightConstantBuffer
        int activeCount = 0;                         // 今フレームに使っている光の数
        const LightComponent* shadowSpotLight = nullptr;   // 今フレームの、影を落とすスポットライト
        bool overflowWarned = false;                 // 数が超えた警告を出したか(超えている間は1回だけ出す)
    }

    void Register(LightComponent* light) {
        if (!light) return;
        if (std::find(registered.begin(), registered.end(), light) != registered.end()) return;   // 二重登録しない
        registered.push_back(light);
    }

    void Unregister(LightComponent* light) {
        registered.erase(std::remove(registered.begin(), registered.end(), light), registered.end());
    }

    LightConstantBuffer Build(const LightComponent* const* lights, int lightCount,
        float cameraX, float cameraY, float cameraZ, bool* overflowed, const LightComponent** shadowLight) {
        // アクティブで有効な光だけを集め、カメラからの距離(の2乗)を測る
        struct Entry { const LightComponent* light; float distanceSq; };
        std::vector<Entry> entries;
        for (int i = 0; i < lightCount; i++) {
            const LightComponent* light = lights[i];
            if (!light || !light->IsEnabled()) continue;
            if (!light->GetOwner() || !light->GetOwner()->GetIsActive()) continue;   // SetActive(false)の光(ObjectPoolで休んでいる物)は使わない
            XMFLOAT3 p = light->GetWorldPosition();
            float dx = p.x - cameraX, dy = p.y - cameraY, dz = p.z - cameraZ;
            entries.push_back({ light, dx * dx + dy * dy + dz * dz });
        }

        // 多すぎる時は、カメラに近い順に maxLights 個だけ使う(同じ距離なら登録した順のまま)
        bool tooMany = (int)entries.size() > maxLights;
        if (overflowed) *overflowed = tooMany;
        if (tooMany) {
            std::stable_sort(entries.begin(), entries.end(),
                [](const Entry& a, const Entry& b) { return a.distanceSq < b.distanceSq; });
            entries.resize(maxLights);
        }

        // 影を落とすスポットライト: 使われる光の中で、カメラに一番近い castShadow の1つ(同時に影を作れるのは1つだけ)
        int shadowIndex = -1;
        for (int i = 0; i < (int)entries.size(); i++) {
            const LightComponent* light = entries[i].light;
            if (light->GetLightType() != LightType::Spot || !light->IsCastingShadow()) continue;
            if (shadowIndex < 0 || entries[i].distanceSq < entries[shadowIndex].distanceSq) shadowIndex = i;
        }
        if (shadowLight) *shadowLight = (shadowIndex >= 0) ? entries[shadowIndex].light : nullptr;

        LightConstantBuffer cb = {};
        cb.counts[0] = (int)entries.size();
        for (int i = 0; i < (int)entries.size(); i++) {
            const LightComponent* light = entries[i].light;
            GpuLight& gpu = cb.lights[i];

            XMFLOAT3 position = light->GetWorldPosition();
            gpu.positionRange[0] = position.x;
            gpu.positionRange[1] = position.y;
            gpu.positionRange[2] = position.z;
            gpu.positionRange[3] = light->GetRange();

            XMFLOAT3 color = light->GetColor();
            float intensity = light->GetIntensity();
            gpu.colorType[0] = color.x * intensity;
            gpu.colorType[1] = color.y * intensity;
            gpu.colorType[2] = color.z * intensity;
            gpu.colorType[3] = (light->GetLightType() == LightType::Spot) ? 2.f : 1.f;

            if (light->GetLightType() == LightType::Spot) {
                XMFLOAT3 direction = light->GetWorldDirection();
                gpu.directionOuter[0] = direction.x;
                gpu.directionOuter[1] = direction.y;
                gpu.directionOuter[2] = direction.z;
                gpu.directionOuter[3] = cosf(XMConvertToRadians(light->GetOuterAngle()));
                gpu.innerPad[0] = cosf(XMConvertToRadians(light->GetInnerAngle()));
                gpu.innerPad[1] = (i == shadowIndex) ? 1.f : 0.f;   // この光の影を引く
            }
        }
        return cb;
    }

    void Update() {
        // 3Dカメラ(画面の揺れが入った、描画に使う位置)に近い順で選ぶ
        const Camera& camera = Image::GetCamera3D();
        bool overflowed = false;
        LightConstantBuffer cb = Build(registered.empty() ? nullptr : registered.data(), (int)registered.size(),
            camera.position.x, camera.position.y, camera.position.z, &overflowed, &shadowSpotLight);
        activeCount = cb.counts[0];

        // 数が超えた時は、開発者が気付けるように出力ウィンドウへ1回だけ知らせる
        if (overflowed && !overflowWarned) {
            OutputDebugStringA("★ Lighting: 光の数が上限(8個)を超えました。カメラに近い8個だけが光ります\n");
        }
        overflowWarned = overflowed;

        // 光が1つも無く、バッファも作っていないなら何もしない(nullのまま。シェーダーは0として読む)
        if (!lightBuffer) {
            if (activeCount == 0 || !Graphics::device) return;
            D3D11_BUFFER_DESC bd = {};
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.ByteWidth = sizeof(LightConstantBuffer);
            bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(Graphics::device->CreateBuffer(&bd, nullptr, &lightBuffer))) {
                OutputDebugStringA("★ Lighting: 定数バッファの作成に失敗\n");
                return;
            }
        }

        D3D11_MAPPED_SUBRESOURCE msr;
        if (SUCCEEDED(Graphics::context->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr))) {
            memcpy(msr.pData, &cb, sizeof(cb));
            Graphics::context->Unmap(lightBuffer, 0);
        }
    }

    void Bind() {
        // バッファがまだ無い間はnullptrをセットする(シェーダーは0として読むので、光は0個の扱い)
        Graphics::context->PSSetConstantBuffers(4, 1, &lightBuffer);
    }

    void Shutdown() {
        if (lightBuffer) { lightBuffer->Release(); lightBuffer = nullptr; }
        registered.clear();
        activeCount = 0;
        shadowSpotLight = nullptr;
        overflowWarned = false;
    }

    int GetActiveCount() {
        return activeCount;
    }

    const LightComponent* GetShadowSpotLight() {
        return shadowSpotLight;
    }
}
