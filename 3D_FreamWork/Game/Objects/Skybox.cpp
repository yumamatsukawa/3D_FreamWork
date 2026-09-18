#include "Skybox.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/MeshRenderer.h"
#include "../Components/SkyboxFollowComponent.h"
#include <climits>

GameObject* CreateSkybox(Scene& scene) {
    GameObject* skybox = scene.CreateObject("Skybox");

    // Cameraのfar(既定10000)より内側に収まる大きさにする(遠方クリップされないように)
    float size = 8000.0f;
    skybox->transform.scale = { size, size, size };

    // 必ず一番最初に描画し(drawPriority最小)、深度も書き込まない(depthWrite=false)。
    // これにより、深度バッファの精度や他の3Dオブジェクトとの競合に一切依存しない、
    // 「常に一番奥にある背景」として描画される(Z-fightingが原理的に起こらない)
    skybox->SetDrawPriority(INT_MIN);

    auto* meshRenderer = skybox->AddComponent<MeshRenderer>(Mesh::CreateSphere());
    meshRenderer->SetTexture(L"Assets/skybox.png");
    meshRenderer->SetUnlit(true);
    meshRenderer->SetDepthWrite(false);

    // カメラに位置だけ追従させる(回転はしない)
    skybox->AddComponent<SkyboxFollowComponent>();

    return skybox;
}
