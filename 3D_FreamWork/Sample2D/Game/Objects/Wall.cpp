#include "Wall.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"

GameObject* CreateWall(Scene& scene, DirectX::XMFLOAT2 position, DirectX::XMFLOAT2 size) {
    // タグを"Wall"にしておくと、VisionComponentの視線がさえぎられる(付け忘れると素通しになる)
    GameObject* wall = scene.CreateObject("Wall", "Wall");
    wall->transform.position = { position.x, position.y, 0.0f };
    wall->transform.scale    = { size.x, size.y, 1.0f };

    // 見た目(画像を引き伸ばして、灰色にする)
    auto* sprite = wall->AddComponent<SpriteRenderer>(L"Assets/bg_tile.png");
    sprite->SetColor(0.5f, 0.5f, 0.5f, 1.0f);

    // 物理演算(PhysX): 動かない壁(奥行きは、Zを固定した2Dの物と重なる厚みを持たせる)
    wall->AddComponent<BoxRigidbodyComponent>(BodyType::Static, DirectX::XMFLOAT3{ size.x, size.y, 100.0f });

    return wall;
}