#include "NetworkSpawnDemo2DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Network/NetworkLobby.h"
#include "../../Objects/Network/NetworkSpawnDemo.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Network/Network.h"

void NetworkSpawnDemo2DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"スポーン・破棄のデモ(NetworkSpawn)",
        L"弾を撃つ頼みをホストが受け、弾を作って全員に知らせます。弾は一定時間で消え、ObjectPoolに返されます。",
        L"操作: Fire ボタンか SPACE で、自分の番号の位置から弾を撃つ。Host で部屋を作り、もう1台が Join する / Tab: 3D版へ",
        L"使っているAPI: Network::RegisterPrefab, Spawn, Despawn, SetDespawnHandler, ObjectPool" });
    CreateNetworkSpawnDemo(*this, DemoMode::Mode2D);
    CreateNetworkLobby(*this, { true, true, true, false });
    Network::NotifySceneReady(*this, 61);   // このシーンの準備ができた(ホストは、これを待って今の状態を送る)
}

void NetworkSpawnDemo2DScene::Uninit()
{
    // this を使うラムダを登録したので、シーンが消える前に登録を全部消す。つないだままにしない
    Network::Disconnect();
    Network::ClearHandlers();
    EventBus::Get().Clear();
    Scene::Uninit();
}
