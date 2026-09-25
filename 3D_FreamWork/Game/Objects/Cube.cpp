#include "Cube.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/MeshRenderer.h"
#include "../Components/SpinComponent.h"
#include "../Components/PlayerController.h"

GameObject* CreateCube(Scene& scene) {
    GameObject* cube = scene.CreateObject("Cube");
    cube->transform.position = {   0.0f,   0.0f, 100.0f };
    cube->transform.scale    = { 100.0f, 100.0f, 100.0f };

    // 見た目(3Dメッシュ)
    auto* meshRenderer = cube->AddComponent<MeshRenderer>(Mesh::LoadOBJ(L"Assets/test_cube.obj"));
    meshRenderer->SetTexture(L"Assets/debug_box.png");

    // 動作確認用に回転させ続ける
    //cube->AddComponent<SpinComponent>();

    return cube;
}
