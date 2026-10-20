#include "DemoLights.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Animation/TweenComponent.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Render3D/LightComponent.h"

namespace {
    // 光を出す場所の目印の玉を付ける(自分は光の影響を受けず、いつも同じ明るさ)
    void AddMarker(GameObject* object, float size, DirectX::XMFLOAT3 color) {
        object->transform.scale = { size, size, size };
        auto* renderer = object->AddComponent<MeshRenderer>(Mesh::CreateSphere());
        renderer->SetColor(color.x, color.y, color.z, 1.0f);
        renderer->SetUnlit(true);
    }
}

GameObject* CreateDemoPointLight(Scene& scene, DirectX::XMFLOAT3 center) {
    GameObject* lamp = scene.CreateObject("DemoPointLight");
    lamp->transform.position = { center.x - 450.f, center.y, center.z };

    // 見た目(光る小さな玉)と、周りを照らす光(点光源。オレンジ色)
    AddMarker(lamp, 30.f, { 1.0f, 0.75f, 0.4f });
    auto* light = lamp->AddComponent<LightComponent>(LightType::Point);
    light->SetColor(1.0f, 0.6f, 0.25f);
    light->SetIntensity(1.8f);
    light->SetRange(700.f);

    // 動き: 左右に往復する(Tweenで動かす。行って戻るのを繰り返す)
    lamp->AddComponent<TweenComponent>()->MoveTo({ center.x + 450.f, center.y, center.z }, 4.f, Ease::InOutQuad).SetLoop(true, true);
    return lamp;
}

GameObject* CreateDemoSpotLight(Scene& scene, DirectX::XMFLOAT3 center, bool castShadow) {
    GameObject* lamp = scene.CreateObject("DemoSpotLight");
    lamp->transform.position = { center.x - 400.f, center.y, center.z };

    // 見た目(光る小さな玉)と、円すい形に照らす光(スポットライト。向きは下の斜め前)
    AddMarker(lamp, 30.f, { 1.0f, 1.0f, 1.0f });
    auto* light = lamp->AddComponent<LightComponent>(LightType::Spot);
    light->SetColor(1.0f, 1.0f, 0.9f);
    light->SetIntensity(2.5f);
    light->SetRange(1400.f);
    light->SetDirection({ 0.f, -1.f, 0.5f });
    light->SetSpotAngle(15.f, 30.f);
    light->SetCastShadow(castShadow);

    // 動き: 左右に往復する(向きは変えないので、照らす場所が左右に動く)
    lamp->AddComponent<TweenComponent>()->MoveTo({ center.x + 400.f, center.y, center.z }, 6.f, Ease::InOutQuad).SetLoop(true, true);
    return lamp;
}
