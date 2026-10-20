#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

// Saveデモの操作役。整数・小数・文字(日本語)を SaveData に書いて、ファイルに保存・読み込みする。
// 保存したらデモを閉じて(アプリを終了して)開き直しても、同じ値が読み込まれる。
// サンプルのセーブ(スロット0。ハイスコアなど)を壊さないよう、このデモはスロット1〜3だけを使い、キーも "Demo_" で始まる物だけを書く
class SaveDemoController : public Component {
private:
    int slot = 1;                       // 今のスロット(1〜3)
    bool ready = false;                 // このスロットを読み込めたか(読めなかった間は書き換えない)
    bool dirty = false;                 // 読み込み・保存の後に値を書き換えたか(未保存の印)
    int saveCount = 0;                  // Save に成功した回数
    std::wstring savePath;              // このスロットの保存先
    std::wstring message;               // 最後の操作の結果
    bool messageError = false;          // messageが失敗の知らせか

    static constexpr int firstSlot = 1;   // 使えるスロット(0はサンプルが使うので使わない)
    static constexpr int lastSlot = 3;

    void UpdateSavePath();              // 保存先の場所を調べる

public:
    void Init() override;
    void Update(float dt) override;
    void Draw() override;

    void SelectSlot(int newSlot);       // 1〜3: スロットを切り替えて、そのファイルを読み込む
    void AddCount();                    // Z: 整数を 1 増やす(SetInt)
    void AddRatio();                    // X: 小数を 0.1 増やす(SetFloat。1を超えたら0へ)
    void NextName();                    // C: 文字(日本語)を次の名前に変える(SetString)
    void SaveNow();                     // S: ファイルに保存(Save)
    void LoadNow();                     // L: ファイルから読み込む(Load。保存していない変更は捨てられる)
    void DeleteKeys();                  // D: 3つのキーを消す(DeleteKey。ファイルは Save するまで変わらない)
};
