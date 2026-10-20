#include "TextDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/UI/TextDemo.h"

void TextDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"Textデモ(文字の見た目)",
        L"大きさ・色・縁取り・太字・寄せ方・フォントを並べます。日本語は Text::LoadFont で読んだフォントで出ます。",
        L"操作: A〜Z・SPACE 文字を入力 / BackSpace 消す / 1〜4 入力した文字のスタイル",
        L"使っているAPI: Text::Draw, Text::LoadFont, Text::Measure, TextStyle, TextAlign, TextRenderer(SetText, SetOffset)" });
    CreateTextDemo(*this);
}

void TextDemoScene::Uninit()
{
    Scene::Uninit();
}
