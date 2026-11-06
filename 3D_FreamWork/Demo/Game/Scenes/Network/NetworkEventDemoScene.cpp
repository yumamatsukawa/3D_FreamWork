#include "NetworkEventDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Network/NetworkLobby.h"
#include "../../Objects/Network/NetworkEventDemo.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Network/Network.h"

void NetworkEventDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"出来事のデモ(NetworkEvent)",
        L"スイッチA・Bを両方押すと扉が開きます(誰が押してもよい)。扉が開いた後に参加した人にも、開いた扉が見えます。",
        L"操作: Press Switch A / B(別々のPCで押してもよい)、Reset Door で戻す。扉を開けたまま、もう1台を Join してみよう",
        L"使っているAPI: Network::SendEvent, Subscribe, SetSharedValue, GetSharedValue, EventBus(DoorOpened)" });
    CreateNetworkLobby(*this, { true, true, true, false });
    CreateNetworkEventDemo(*this);
}

void NetworkEventDemoScene::Uninit()
{
    Network::Disconnect();
    Network::ClearHandlers();
    EventBus::Get().Clear();
    Scene::Uninit();
}