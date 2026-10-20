#include "ResourcesDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/ResourcesDemo.h"

void ResourcesDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Resourcesデモ(画像・音・モデルの共有)",
        L"同じ画像を何枚置いても、読み込みは1回です。使う人がいなくなっても、UnloadUnused まで残ります。",
        L"操作: Add 10 同じ画像を増やす / Remove 全部消す / Unload 使われていない物を解放 / Sound・Model 2回借りる / Release 返す",
        L"使っているAPI: Resources::LoadTexture, ReleaseTexture, LoadSound, ReleaseSound, LoadModel, UnloadUnused" });
    CreateResourcesDemo(*this);
}

void ResourcesDemoScene::Uninit()
{
    Scene::Uninit();
}
