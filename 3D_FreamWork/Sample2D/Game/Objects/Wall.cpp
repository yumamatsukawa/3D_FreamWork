#include "Wall.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render/SpriteRenderer.h"
#include "../../../Engine/Physics/RigidbodyComponent.h"

namespace {
    constexpr float wallZ = 20.0f;   // 前後はzで決まる(2Dモード。小さいほど手前)。背景(+100)の手前、見張り役(+10)・敵(+5)・Player(0)・弾(-5)の奥。壁を探す経路の格子(厚み10、z=0)にも、壁の厚み(100)が届く
}

GameObject* CreateWall(Scene& scene, DirectX::XMFLOAT2 position, DirectX::XMFLOAT2 size) {
    // タグを"Wall"にしておくと、VisionComponentの視線がさえぎられる(付け忘れると素通しになる)
    GameObject* wall = scene.CreateObject("Wall", "Wall");
    wall->transform.position = { position.x, position.y, wallZ };
    wall->transform.scale    = { size.x, size.y, 1.0f };

    // 見た目(画像を引き伸ばして、灰色にする)
    auto* sprite = wall->AddComponent<SpriteRenderer>(L"Assets/bg_tile.png");
    sprite->SetColor(0.5f, 0.5f, 0.5f, 1.0f);
    sprite->SetMode(SpriteMode::World2D);   // 2Dモード: zで大きさは変わらず、zが小さいほど手前

    // 物理演算(PhysX): 動かない壁(奥行きは、Zを固定した2Dの物と重なる厚みを持たせる)
    wall->AddComponent<BoxRigidbodyComponent>(BodyType::Static, DirectX::XMFLOAT3{ size.x, size.y, 100.0f });

    return wall;
}