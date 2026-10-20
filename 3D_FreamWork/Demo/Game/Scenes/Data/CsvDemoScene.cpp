#include "CsvDemoScene.h"
#include "../../Objects/Common/DemoBackground.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Data/CsvDemo.h"

void CsvDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoBackground(*this);
    CreateDemoGuide(*this, { L"表(CSV)を読むデモ(Csv)",
        L"CSVを読んで表にします。実行中にファイルを保存し直すと、0.5秒ごとの確認で見つけて自動で読み直します(ホットリロード)",
        L"操作: メモ帳などで画面のファイルを編集して保存 / R 今すぐ読み直す / Esc: 戻る",
        L"使っているAPI: Csv::Load, IsFileUpdated, Reload, GetString, GetInt, GetFloat, FindRow, HasColumn" });
    CreateCsvDemo(*this);
}

void CsvDemoScene::Uninit()
{
    Scene::Uninit();
}