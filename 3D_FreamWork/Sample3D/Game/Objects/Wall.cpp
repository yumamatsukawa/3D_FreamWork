#include "Wall.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render3D/MeshRenderer.h"
#include "../../../Engine/Physics/RigidbodyComponent.h"

GameObject* CreateWall(Scene& scene, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 size) {
    // タグを"Wall"にしておくと、VisionComponentの視線がさえぎられる(付け忘れると素通しになる)
    GameObject* wall = scene.CreateObject("Wall", "Wall");
    wall->transform.position = position;
    wall->transform.scale    = size;

    // 見た目(3Dメッシュの立方体)
    auto* meshRenderer = wall->AddComponent<MeshRenderer>(Mesh::CreateCube());
    meshRenderer->SetTexture(L"Assets/debug_box.png");
    meshRenderer->GetMaterial().castShadow = true;     // 影を落とす(地面や、懐中電灯の光をさえぎる)
    meshRenderer->GetMaterial().receiveShadow = true;

    // 物理演算(PhysX): 動かない壁(大きさはtransform.scaleと同じ)
    wall->AddComponent<BoxRigidbodyComponent>(BodyType::Static);

    return wall;
}