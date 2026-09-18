#include "Skybox.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/MeshRenderer.h"
#include "../Components/SkyboxFollowComponent.h"

GameObject* CreateSkybox(Scene& scene) {
    GameObject* skybox = scene.CreateObject("Skybox");

    // Cameraのfar(既定10000)より内側に収まる大きさにする(遠方クリップされないように)
    float size = 8000.0f;
    skybox->transform.scale = { size, size, size };

    auto* meshRenderer = skybox->AddComponent<MeshRenderer>(Mesh::CreateSphere());
    meshRenderer->SetTexture(L"Assets/skybox.png");

    // カメラに位置だけ追従させる(回転はしない)
    skybox->AddComponent<SkyboxFollowComponent>();

    return skybox;
}
