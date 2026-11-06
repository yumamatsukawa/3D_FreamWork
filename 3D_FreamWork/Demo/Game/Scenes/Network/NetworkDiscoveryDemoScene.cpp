#include "NetworkDiscoveryDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Network/NetworkLobby.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Network/Network.h"

void NetworkDiscoveryDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"ホスト探索のデモ(NetworkDiscovery)",
        L"Host で部屋を作ると、同じLANのPCの Find Hosts に一覧で出ます。出たボタンを押すと、そのホストに参加します。",
        L"操作: 1台で Host、もう1台で Find Hosts → 右に出るホストのボタンを押す / Disconnect(すべてマウス)",
        L"使っているAPI: Network::StartHost, StartSearch, StopSearch, GetFoundHosts, Join, SetGameId" });
    CreateNetworkLobby(*this, { false, false, true, true });
}

void NetworkDiscoveryDemoScene::Uninit()
{
    Network::Disconnect();
    Network::ClearHandlers();
    EventBus::Get().Clear();
    Scene::Uninit();
}