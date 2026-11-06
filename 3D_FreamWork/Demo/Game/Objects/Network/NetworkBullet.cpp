#include "NetworkBullet.h"
#include "../../Components/Network/NetworkBulletController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Network/Network.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

namespace {
    constexpr float bulletSize = 44.f;   // 弾の大きさ
}

void SetupNetworkBullet(GameObject* bullet, bool is3D) {
    bullet->SetTag("Bullet");
    bullet->transform.scale = { bulletSize, bulletSize, bulletSize };

    // 見た目: 3Dは球、2Dは丸い絵(box.png の1コマ)
    if (is3D) {
        bullet->AddComponent<MeshRenderer>(Mesh::CreateSphere());
    }
    else {
        bullet->transform.scale.z = 1.f;
        bullet->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetMode(SpriteMode::World2D);
    }

    // 同期の印と、位置の同期(向きは使わない)
    bullet->AddComponent<NetworkIdentityComponent>();
    NetworkTransformSettings settings;
    settings.syncRotation = false;
    settings.is2D = !is3D;
    bullet->AddComponent<NetworkTransformComponent>(settings);

    // 動き(ホストだけが動かす)
    bullet->AddComponent<NetworkBulletController>();
}

void ApplyNetworkBulletLook(GameObject* bullet, bool is3D, int owner) {
    // 持ち主の番号で色を変える(0=赤、1=緑、2=青、3=黄)
    static const float colors[4][3] = { { 1.f, 0.3f, 0.3f }, { 0.3f, 1.f, 0.4f }, { 0.4f, 0.5f, 1.f }, { 1.f, 0.9f, 0.3f } };
    const int index = (owner >= 0 && owner < 4) ? owner : 0;
    if (is3D) {
        bullet->GetComponent<MeshRenderer>()->SetColor(colors[index][0], colors[index][1], colors[index][2], 1.f);
    }
    else {
        // box.png の 3x3 の絵のうち、色の違う4つを使う
        static const int cells[4] = { 0, 1, 2, 3 };
        bullet->GetComponent<SpriteRenderer>()->SetSpriteIndex(cells[index]);
    }
}