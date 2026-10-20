#include "InputDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Core/InputDemo.h"

void InputDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Inputデモ(キーボード・マウス・ゲームパッド)",
        L"キー・マウス・パッド(XInput)の今の状態を表示します。つながっていないパッドは「未接続」と出ます。",
        L"操作: 何かキーを押す / マウスを動かす・クリック / パッドをつなぐ / F1 カーソル固定",
        L"使っているAPI: Input::GetKeyDown / Press / Up, GetMousePosition, GetMouseDelta, GetMouseWorldPosition, GetPad○○" });
    CreateInputDemo(*this);
}

void InputDemoScene::Uninit()
{
    Scene::Uninit();
}
