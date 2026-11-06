#include "NetworkSpawnManager.h"
#include "NetworkBulletController.h"
#include "../Common/DemoTextStyle.h"
#include "../../Objects/Network/NetworkBullet.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Network/Network.h"
#include "../../../../Engine/UI/Text.h"

DirectX::XMFLOAT3 NetworkSpawnManager::GetMuzzle(int player) const {
    // 4人が横に並んで、下から真上に撃つ
    const float x = -300.f + 200.f * (float)player;
    return is3D ? DirectX::XMFLOAT3{ x, 0.f, 0.f } : DirectX::XMFLOAT3{ x, -280.f, 0.f };
}

GameObject* NetworkSpawnManager::CreateBullet(Scene& scene, int owner) {
    // 借りた弾は前に使った時の値が残っているので、見た目(持ち主の色)と動きを毎回付け直す
    GameObject* bullet = pool.Rent(scene, [this](GameObject* object) { SetupNetworkBullet(object, is3D); });
    ApplyNetworkBulletLook(bullet, is3D, owner);
    bullet->GetComponent<NetworkBulletController>()->Launch();
    return bullet;
}

void NetworkSpawnManager::Fire() {
    // 撃つ頼みはホストへ(ホスト自身が撃つ時も同じ道を通る)
    Network::SendEvent("FireRequest", NetworkTarget::Host);
}

void NetworkSpawnManager::Init() {
    // プレハブの登録: ホストもクライアントも同じ内容で(クライアントはSpawnを受けた時にこれで弾を作る)
    Network::RegisterPrefab(bulletPrefab, [this](Scene& scene, const NetworkSpawnInfo& info) {
        return CreateBullet(scene, info.ownerPlayerIndex);
    });
    // 消す時は Destroy ではなくプールに返す
    Network::SetDespawnHandler(bulletPrefab, [this](GameObject* bullet) { pool.Return(bullet); });

    // 撃つ頼みを受けて、ホストだけが弾を作る(作った弾は Network が全員に知らせる)
    Network::Subscribe("FireRequest", [this](NetworkReader&, int sender) {
        if (!Network::IsHost()) return;
        Network::Spawn(*GetOwner()->GetScene(), bulletPrefab, sender, GetMuzzle(sender));
    });
}

void NetworkSpawnManager::Update(float dt) {
    // 役割(ホスト/クライアント)が変わったら、出ている弾を全部片付ける(弾を消すのはホストだけなので、残ると消えなくなる)
    if (Network::IsClient() != wasClient) {
        wasClient = Network::IsClient();
        for (GameObject* bullet : pool.GetObjects()) if (bullet->GetIsActive()) pool.Return(bullet);
    }

    // SPACEキーで撃つ
    if (!Time::IsPaused() && Input::GetKeyDown(KEY_SPACE)) Fire();
}

void NetworkSpawnManager::Draw() {
    // 今この画面に出ている弾の数(プールの中で、アクティブな物)
    int active = 0;
    for (const GameObject* object : pool.GetObjects()) if (object->GetIsActive()) active++;

    Text::Draw(L"この画面に出ている弾: " + std::to_wstring(active) + L"   プールの大きさ: " + std::to_wstring(pool.GetPoolSize()),
        150.f, -130.f, MakeDemoTextStyle(19.f, { 1.f, 0.9f, 0.4f, 1.f }));
    Text::Draw(L"弾を出すのはホストだけ。撃つ頼み(FireRequest)は", 150.f, -160.f, MakeDemoTextStyle(17.f));
    Text::Draw(L"誰でも送れる。自分の番号の色の弾が出る", 150.f, -186.f, MakeDemoTextStyle(17.f));
}