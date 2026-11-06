#include "NetworkTransformDemo3DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Network/NetworkLobby.h"
#include "../../Objects/Network/NetworkArena.h"
#include "../../Objects/Network/NetworkWorld.h"
#include "../../Objects/Network/NetworkTransformDemo.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Network/Network.h"

void NetworkTransformDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"位置の同期のデモ(NetworkTransform)",
        L"ホストが動かす箱と球を、クライアントは少し遅れて滑らかに表示します(クライアントの球は物理では動きません)。",
        L"操作: Host で部屋を作り、もう1台が Join する(マウス)。右ドラッグ 視点 / Q・E ズーム / Tab: 2D版へ",
        L"使っているAPI: NetworkIdentityComponent, NetworkTransformComponent, Network::IsHost, HasAuthority" });
    CreateDemoStage3D(*this, { 0.0f, 350.0f, 0.0f }, 0.0f, -25.0f, 1150.0f);
    CreateNetworkArena(*this, DemoMode::Mode3D);
    CreateNetworkWorld(*this, [this] { CreateNetworkTransformDemo(*this, DemoMode::Mode3D); });   // 役割(ホスト/クライアント)に合わせて作り直される
    CreateNetworkLobby(*this, { true, true, true, false });
    Network::NotifySceneReady(*this, 32);   // このシーンの準備ができた(ホストは、これを待って今の状態を送る)
}

void NetworkTransformDemo3DScene::Uninit()
{
    // this を使うラムダを登録したので、シーンが消える前に登録を全部消す。つないだままにしない
    Network::Disconnect();
    Network::ClearHandlers();
    EventBus::Get().Clear();
    Scene::Uninit();
}
