#include "NetworkInputDemo3DScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Network/NetworkLobby.h"
#include "../../Objects/Network/NetworkArena.h"
#include "../../Objects/Network/NetworkWorld.h"
#include "../../Objects/Network/NetworkInputDemo.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Network/Network.h"

void NetworkInputDemo3DScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"入力の同期のデモ(NetworkInput)",
        L"自分の番号の箱を、キーで動かします。入力はホストに送られ、ホストが動かした結果が全員の画面に出ます。",
        L"操作: Host / Join のあと WASD か矢印で移動、SPACE でジャンプ。いない人の箱は暗い / Tab: 2D版へ",
        L"使っているAPI: Network::SetLocalInput, GetInput, NetworkInput, HasAuthority, RigidbodyComponent::SetVelocity" });
    CreateDemoStage3D(*this, { 0.0f, 350.0f, 0.0f }, 0.0f, -25.0f, 1150.0f);
    CreateNetworkArena(*this, DemoMode::Mode3D);
    CreateNetworkWorld(*this, [this] { CreateNetworkInputDemo(*this, DemoMode::Mode3D); });   // 役割(ホスト/クライアント)に合わせて作り直される
    CreateNetworkLobby(*this, { true, true, true, false });
    Network::NotifySceneReady(*this, 42);   // このシーンの準備ができた(ホストは、これを待って今の状態を送る)
}

void NetworkInputDemo3DScene::Uninit()
{
    // this を使うラムダを登録したので、シーンが消える前に登録を全部消す。つないだままにしない
    Network::Disconnect();
    Network::ClearHandlers();
    EventBus::Get().Clear();
    Scene::Uninit();
}
