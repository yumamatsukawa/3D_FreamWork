#include "EventBusDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/EventBusDemo.h"
#include "../../../../Engine/Core/EventBus.h"

void EventBusDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"EventBusデモ(イベントの送受信)",
        L"送る側・受け取る側・記録係は、お互いを知りません。イベント名だけでつながっています。",
        L"操作: ボタン(または 1・2 キー)でイベントを送る。右の記録係が、流れたイベントを順に書きます",
        L"使っているAPI: EventBus::Get().Subscribe, Publish, SubscribeObject, PublishObject, Clear" });
    CreateEventBusDemo(*this);
}

void EventBusDemoScene::Uninit()
{
    // this を使うラムダを登録したので、シーンが消える前に登録を全部消す(消えたComponentが呼ばれないように)
    EventBus::Get().Clear();
    Scene::Uninit();
}
