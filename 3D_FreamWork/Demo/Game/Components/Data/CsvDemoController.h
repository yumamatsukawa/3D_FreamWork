#pragma once
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Core/Timer.h"
#include "../../../../Engine/Data/Csv.h"
#include <string>

// Csvデモの操作役。デモ専用の表(Assets/Demo/DemoTable.csv)を読んで画面に表を出す。
// 0.5秒ごとに IsFileUpdated で調べ、ファイルが保存し直されていたら Reload する(ホットリロード)。
// 表を読むだけで、ファイルには何も書かない(サンプルの Enemies.csv / Waves.csv にも触らない)
class CsvDemoController : public Component {
private:
    Csv table;                              // 読み込んだ表
    Timer checkTimer{ 0.5f, true };         // ファイルが変わったかを調べる間隔(0.5秒ごとに繰り返す)
    std::wstring fullPath;                  // 読んでいるファイルの場所(画面に出して、編集する場所を教える)
    std::wstring message;                   // 最後の読み込みの結果
    bool messageError = false;              // messageが失敗の知らせか
    int reloadCount = 0;                    // Reload が成功した回数
    int checkCount = 0;                     // IsFileUpdated で調べた回数
    float sinceReload = 0.f;                // 最後に読み込んでからの秒数
    float flashTimer = 0.f;                 // 表の枠を光らせる残り時間(読み直した合図)

    static constexpr float checkInterval = 0.5f;   // 調べる間隔(秒)
    static constexpr float flashSeconds = 1.f;     // 読み直した合図を出す時間
    static constexpr int maxRows = 8;              // 表に出す行の数の上限

    void ReloadNow();   // 今すぐ読み直す(成功したら表が入れ替わる)

public:
    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
