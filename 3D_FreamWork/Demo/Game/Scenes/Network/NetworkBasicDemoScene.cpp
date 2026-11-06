#include "NetworkBasicDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Network/NetworkLobby.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Network/Network.h"

void NetworkBasicDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"ネットワークの基本(NetworkBasic)",
        L"1台が Host で部屋を作り、もう1台が Join で参加します。つながったら Send で、文字が全員に届きます。",
        L"操作: Host / Join 127.0.0.1(同じPCの2つ目)/ Edit IP → Join IP(別のPC)/ Send / Disconnect(すべてマウス)",
        L"使っているAPI: Network::StartHost, Join, Disconnect, SendEvent, Subscribe, GetState, GetPing, GetStats" });
    CreateNetworkLobby(*this, { true, true, false, true });
}

void NetworkBasicDemoScene::Uninit()
{
    // this を使うラムダを登録したので、シーンが消える前に登録を全部消す。つないだままにしない
    Network::Disconnect();
    Network::ClearHandlers();
    EventBus::Get().Clear();
    Scene::Uninit();
}