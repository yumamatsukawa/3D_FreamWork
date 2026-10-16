#include "Wall.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"

GameObject* CreateWall(Scene& scene, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 size) {
    // タグを"Wall"にしておくと、VisionComponentの視線がさえぎられる(付け忘れると素通しになる)
    GameObject* wall = scene.CreateObject("Wall", "Wall");
    wall->transform.position = position;
    wall->transform.scale    = size;

    // 見た目(3Dメッシュの立方体)
    auto* meshRenderer = wall->AddComponent<MeshRenderer>(Mesh::CreateCube());
    meshRenderer->SetTexture(L"Assets/debug_box.png");

    // 物理演算(PhysX): 動かない壁(大きさはtransform.scaleと同じ)
    wall->AddComponent<BoxRigidbodyComponent>(BodyType::Static);

    return wall;
}