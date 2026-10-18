#include "Cube.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render3D/MeshRenderer.h"
#include "../Components/SpinComponent.h"
#include "../../../Engine/Physics/RigidbodyComponent.h"

GameObject* CreateCube(Scene& scene) {
    GameObject* cube = scene.CreateObject("Cube");
    cube->transform.position = {   0.0f,   0.0f, 100.0f };
    cube->transform.scale    = { 100.0f, 100.0f, 100.0f };

    // 見た目(3Dメッシュ)
    auto* meshRenderer = cube->AddComponent<MeshRenderer>(Mesh::LoadOBJ(L"Assets/test_cube.obj"));
    meshRenderer->SetTexture(L"Assets/debug_box.png");

    cube->AddComponent<BoxRigidbodyComponent>(BodyType::Static);

    // 動作確認用に回転させ続ける
    //cube->AddComponent<SpinComponent>();

    return cube;
}
