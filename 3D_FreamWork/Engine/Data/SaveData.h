#pragma once
#include <string>

// ハイスコア・音量などを保存して、次に起動した時も残るようにする(Unityの PlayerPrefs と同じ使い方)。
// Set/Get で読み書きして、Save でファイルに書く。起動時にEngineが自動で Load(0) し、
// 終了時(GameEngine::Uninit)に、書き換えていれば自動で Save する(ゲーム側で Save を呼び忘れても消えない)。
// ただし起動時の Load(0) が読めなかった(false)時は、そのファイルを上書きしないように、終了時の自動保存はしない
//
// 保存先: %LOCALAPPDATA%\3D_FreamWork\save{slot}.txt(例: save0.txt)。Sample2DとSample3Dで同じファイルを使う
// 決まり:
// - キーに使える文字は、英字・数字・_ だけ(PascalCaseの英語にする: "HighScore", "MusicVolume")。
//   それ以外の文字を含むキー・空のキーは、Setしても保存されず、Getはいつも defaultValue を返す(出力ウィンドウに知らせる)
// - 値は型を覚えずに文字で持ち、Get の時に変換する。同じキーに別の型で Set すると、後に書いた値で上書きされる
// - GetInt は、整数として読めない値("1.5"、"abc"、intに入らない大きな数)なら defaultValue を返す(小数を切り捨てたりしない)
// - GetFloat は、数として読めない値なら defaultValue を返す(整数の "100" は 100.0 として読める)
// - GetString は、どの型で書いた値も文字のまま返す(SetInt(…, 5) なら L"5")
namespace SaveData {
    void  SetInt(const std::string& key, int value);                          // 整数を書く
    int   GetInt(const std::string& key, int defaultValue = 0);               // 整数を読む(無い・読めなければdefaultValue)
    void  SetFloat(const std::string& key, float value);                      // 小数を書く(無限大・NaNは書かない)
    float GetFloat(const std::string& key, float defaultValue = 0.f);         // 小数を読む(無い・読めなければdefaultValue)
    void  SetString(const std::string& key, const std::wstring& value);       // 文字を書く(日本語・改行・= もそのまま使える)
    std::wstring GetString(const std::string& key, const std::wstring& defaultValue = L""); // 文字を読む(無ければdefaultValue)
    bool  HasKey(const std::string& key);                                     // そのキーがあるか
    void  DeleteKey(const std::string& key);                                  // そのキーを消す(無ければ何もしない)

    // ファイルに書き出す(成功したらtrue)。一時ファイル(save0.txt.tmp)に書いてから本物と差し替えるので、
    // 失敗しても・保存中に落ちても、前のセーブデータは壊れない。失敗したら出力ウィンドウに知らせる。
    // 中身が16MBより大きい時は書かずにfalse(Loadで読めなくなるため)。
    // ※Load が false だった(ファイルはあるのに読めなかった)後に Save すると、読めなかったファイルを今の値で上書きする
    bool  Save(int slot = 0);
    // ファイルから読み込む(今の値はすべて捨てて、ファイルの中身に置き換わる)。起動時にEngineが自動で Load(0) する。
    // ファイルが無いのは正常(初めての起動)で、空の状態にして true。読めなかった時(ほかのアプリが開いている・16MBより大きい)は
    // 今の値をそのまま残して false。
    // 壊れた行は飛ばす(出力ウィンドウに行番号を知らせる)
    bool  Load(int slot = 0);

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    void Init();     // 起動時: Load(0)
    void Uninit();   // 終了時: 書き換えていれば、最後に Load / Save に成功したスロットへ Save する(1回も成功していなければ保存しない)
}
