#include "SaveDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Data/SaveDemo.h"

void SaveDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"セーブデータのデモ(Save)",
        L"整数・小数・文字(日本語)を書いて、ファイルに保存します。アプリを閉じて開き直しても、同じ値が読み込まれます。",
        L"操作: Z 整数 / X 小数 / C 名前 / S 保存 / L 読込 / D 消す / 1 2 3 スロット / Esc: 戻る",
        L"使っているAPI: SaveData::SetInt, GetInt, SetFloat, GetFloat, SetString, GetString, HasKey, DeleteKey, Save, Load" });
    CreateSaveDemo(*this);
}

void SaveDemoScene::Uninit()
{
    Scene::Uninit();
}