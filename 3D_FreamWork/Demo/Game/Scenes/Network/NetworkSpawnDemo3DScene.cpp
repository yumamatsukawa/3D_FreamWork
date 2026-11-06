#include "NetworkSpawnDemo3DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Network/NetworkLobby.h"
#include "../../Objects/Network/NetworkSpawnDemo.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Network/Network.h"

void NetworkSpawnDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"スポーン・破棄のデモ(NetworkSpawn)",
        L"弾を撃つ頼みをホストが受け、弾を作って全員に知らせます。弾は一定時間で消え、ObjectPoolに返されます。",
        L"操作: Fire ボタンか SPACE で、自分の番号の位置から弾を撃つ。Host で部屋を作り、もう1台が Join する / Tab: 2D版へ",
        L"使っているAPI: Network::RegisterPrefab, Spawn, Despawn, SetDespawnHandler, ObjectPool" });
    CreateDemoStage3D(*this, { 0.0f, 350.0f, 0.0f }, 0.0f, -25.0f, 1150.0f);
    CreateNetworkSpawnDemo(*this, DemoMode::Mode3D);
    CreateNetworkLobby(*this, { true, true, true, false });
    Network::NotifySceneReady(*this, 62);   // このシーンの準備ができた(ホストは、これを待って今の状態を送る)
}

void NetworkSpawnDemo3DScene::Uninit()
{
    // this を使うラムダを登録したので、シーンが消える前に登録を全部消す。つないだままにしない
    Network::Disconnect();
    Network::ClearHandlers();
    EventBus::Get().Clear();
    Scene::Uninit();
}
