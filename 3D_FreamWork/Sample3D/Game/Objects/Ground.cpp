#include "Ground.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../Components/GroundController.h"
#include <climits>

GameObject* CreateGround(Scene& scene) {
    // 地面の上面の高さと、当たり判定の大きさ(見た目のタイルとは別に、見えない大きな板を1枚置く)
    const float topY = -50.0f;
    const DirectX::XMFLOAT3 colliderSize = { 100000.0f, 10.0f, 100000.0f };

    GameObject* ground = scene.CreateObject("Ground");
    ground->transform.position = { 0.0f, topY - colliderSize.y * 0.5f, 0.0f };

    // スカイボックス(INT_MIN)のすぐ後、他のオブジェクトより先に描く
    // (半透明の部分がある敵の画像より後に描くと、画像の透明な部分の奥に地面が描かれないことがあるため)
    ground->SetDrawPriority(INT_MIN + 1);

    // 見た目: カメラの周りにタイル(1枚1000四方、画像はbg_tile.png)を敷き詰める
    ground->AddComponent<GroundController>(L"Assets/bg_tile.png", 1000.0f, topY);

    // 物理演算(PhysX): 動かない床
    ground->AddComponent<BoxRigidbodyComponent>(BodyType::Static, colliderSize);

    return ground;
}
