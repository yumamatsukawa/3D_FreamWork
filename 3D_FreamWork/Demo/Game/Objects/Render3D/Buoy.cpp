#include "Buoy.h"
#include "../../Components/Render3D/FloatOnWaterController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

GameObject* CreateBuoy(Scene& scene, WaterRenderer* water, DirectX::XMFLOAT3 position, float size) {
    GameObject* buoy = scene.CreateObject("Buoy");
    buoy->transform.position = position;
    buoy->transform.scale = { size, size, size };

    // 見た目(赤い立方体)
    auto* meshRenderer = buoy->AddComponent<MeshRenderer>(Mesh::CreateCube());
    meshRenderer->SetColor(0.9f, 0.2f, 0.2f, 1.0f);

    // 動き: 波の高さに合わせて上下する(箱の高さの4分の1だけ、波の面から中心を上げる)
    buoy->AddComponent<FloatOnWaterController>(water, size * 0.25f);
    return buoy;
}
