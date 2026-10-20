#include "PoolDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/PoolDemo.h"

void PoolDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Poolデモ(ObjectPool)",
        L"弾を出しては消す時、毎回作らずに使い回します。何回出しても、作った数は増え続けません。",
        L"操作: クリック・SPACE(押し続けると連射) 弾を出す。弾はマウスの方へ飛びます",
        L"使っているAPI: ObjectPool(Prewarm, Rent, Return, GetPoolSize, GetObjects)" });
    CreatePoolDemo(*this);
}

void PoolDemoScene::Uninit()
{
    Scene::Uninit();
}
