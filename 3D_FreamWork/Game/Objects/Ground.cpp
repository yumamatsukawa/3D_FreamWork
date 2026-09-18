#include "Ground.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/MeshRenderer.h"
#include "../Components/SpinComponent.h"
#include "../Components/PlayerController.h"

GameObject* CreateGround(Scene& scene) {
    GameObject* cube = scene.CreateObject("Ground");
    cube->transform.position = {    0.0f, -50.0f ,  100.0f };
    cube->transform.scale =    { 1000.0f,   0.01f, 1000.0f };

    // 見た目(3Dメッシュ)
    auto* meshRenderer = cube->AddComponent<MeshRenderer>(Mesh::CreateCube());
    meshRenderer->SetTexture(L"Assets/Ground.png");

    return cube;
}
