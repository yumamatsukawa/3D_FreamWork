#include "TimeDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/TimeDemo.h"

void TimeDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Timeデモ(時間の倍率・ポーズ・Timer)",
        L"倍率を変えると dt で動く物だけが遅くなります。Timer も同じ時間で進みます。",
        L"操作: SPACE スロー / P ポーズ / 1 Timer(2秒・1回) / 2 Timer(1秒ごと) / 3 Timer(5秒)",
        L"使っているAPI: Time::GetDeltaTime, GetUnscaledDeltaTime, SetTimeScale, IsPaused, Timer" });
    CreateTimeDemo(*this);
}

void TimeDemoScene::Uninit()
{
    Scene::Uninit();
}
