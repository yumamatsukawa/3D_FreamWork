#pragma once
#include <string>
#include <memory>

// 表(CSVファイル)を読んで、列の名前で値を取り出す(敵の強さ・出現の順番などを、コードではなくExcelで調整するため)。
// 使い方: ComponentのInitで1回Loadして、必要な値を取り出す(毎フレームLoadしない。Loadは毎回ファイルを読み直す)
//
//   Csv table = Csv::Load(L"Assets/Data/Enemies.csv");       // 1行目は列の名前(Name,Speed)
//   int row = table.FindRow("Name", L"Slime");                // Nameが Slime の行を探す(無ければ-1)
//   float speed = table.GetFloat(row, "Speed", 60.f);         // 無い・読めなければ 60
//
// 決まり:
// - 列の名前で取り出すので、Excelで列の順番を入れ替えても壊れない。列の名前は半角英数字(Name, Hp, Speed)にする。
//   列の名前の比較は、大文字小文字を区別する("name" と "Name" は別)。列の名前の前後の空白は無視する。
//   同じ列の名前が2つあれば最初のものを使う
// - 行の番号は0から数える(1行目の列の名前は数えない。つまり表の2行目が row=0)
// - 列が無い・行が範囲外・値が空・数に読めない時は defaultValue(GetStringは空文字列)を返し、
//   出力ウィンドウに知らせる(列ごと・ファイルごとに1回だけ)
// - GetInt は全体が整数として読める時だけ("1.5" や "abc" は defaultValue。SaveDataと同じ)。
//   GetFloat は有限の数だけ。どちらも前後の空白は無視し、PCの地域設定に関係なく小数点は . で読む
// - 形式はRFC 4180に近い: "" で囲んだ値(中のカンマ・改行・"" で書いた " も使える)、改行はCRLF/LF/CRのどれでもよい、
//   最後の行に改行が無くてもよい、空行(全部の値が空の行)は飛ばす
//   行ごとに列の数が違っていてもよい(足りない分は空、多い分は無視。出力ウィンドウに知らせる)
// - 文字コードはUTF-8(BOMがあっても無くてもよい)。UTF-8として不正なバイトがあれば Shift-JIS として読み直し、
//   「Excelでは『CSV UTF-8(コンマ区切り)』で保存してください」と知らせる。UTF-16のファイルは読めない
// - 読めない時(ファイルが無い・16MBより大きい・中身が空で列の名前の行も無い)は IsValid() が false で、
//   Get 系はいつも defaultValue を返す(Loadした時に出力ウィンドウに知らせる)
// - Csvは値として持てる(コピーしても、表の中身は共有するので軽い)
//
// 実行中の反映(ホットリロード): 使う側が0.5秒ごとなどに IsFileUpdated() を調べ、trueなら Reload() する
//
//   if (checkTimer.Update(dt) && table.IsFileUpdated()) {
//       if (table.Reload()) { /* 値を取り出し直す */ }
//   }
//
// - IsFileUpdated は更新日時とファイルの大きさを見るだけで軽いが、毎フレームは呼ばない
// - Reload が失敗した時(保存の途中で読めない・壊れている)は、今の中身をそのまま残す。
//   読み込み済みの日時は変えないので、次の IsFileUpdated でまた試せる。同じ状態の失敗は出力ウィンドウに1回だけ知らせる
// - Excelなどで開いたままでも読める(読む時は、他のプログラムの読み書き・名前の変更を止めない開き方をする)。
//   ただし相手が書き込み中に他の読み取りを禁止している間は読めないので、そのときは失敗として、次の確認でまた試す
// - Reload で入れ替わるのは、Reload を呼んだ Csv が持つ中身だけ(先にコピーした Csv は古い中身のまま)
class Csv {
public:
    // 読み込む(毎回ファイルを読む。キャッシュはしない)。1行目は列の名前として扱う
    static Csv Load(const std::wstring& path);

    // 読み込んだ後にファイルが変わったか(更新日時か大きさが、読み込んだ時と違えばtrue)
    // - ファイルが今は無い(消された・保存の途中)ならfalse
    // - 読み込めなかった(無かった・読めなかった)が、今は読めそうなファイルがあるならtrue
    bool IsFileUpdated() const;

    // 同じパスを読み直して、成功したら中身を入れ替えてtrue。失敗したら今の中身のまま、falseを返す
    bool Reload();

    bool IsValid() const { return table != nullptr; }   // 読み込めたか
    int  GetRowCount() const;                            // 行の数(1行目の列の名前は数えない。読めなければ0)
    int  GetColumnCount() const;                         // 列の数(1行目の値の数。読めなければ0)
    bool HasColumn(const std::string& column) const;     // その名前の列があるか(出力ウィンドウには知らせない)

    std::wstring GetString(int row, const std::string& column) const;                            // 文字として読む(無ければ空文字列)
    int   GetInt(int row, const std::string& column, int defaultValue = 0) const;                // 整数として読む
    float GetFloat(int row, const std::string& column, float defaultValue = 0.f) const;          // 小数として読む

    // その列の値がvalueと同じ最初の行を探す(完全に同じ文字だけ。無い・列が無い・読めなければ-1)
    int FindRow(const std::string& column, const std::wstring& value) const;

private:
    struct Table;                              // 表の中身(コピーしても共有する)
    std::shared_ptr<const Table> table;        // 読めなかった時はnullptr

    // ファイルの状態(更新日時と大きさ)
    struct FileStamp {
        bool exists = false;                   // ファイルがあったか
        unsigned long long writeTime = 0;      // 最後に書き込まれた日時(FILETIME)
        unsigned long long size = 0;           // ファイルの大きさ(バイト)
        bool operator==(const FileStamp& other) const {
            return exists == other.exists && writeTime == other.writeTime && size == other.size;
        }
    };

    std::wstring path;                         // 読んだファイル(Reload用)
    FileStamp loadedStamp;                     // 読み込めた時のファイルの状態(読み込めていなければ、存在しない状態)
    FileStamp failedStamp;                     // 最後に読み込みに失敗した時のファイルの状態(同じ失敗を何度も知らせないため)
    bool hasFailed = false;                    // failedStampが有効か

    static FileStamp QueryStamp(const std::wstring& path);   // 今のファイルの状態を調べる

    // ファイルを読んで表を作る。失敗したらnullptr。知らせは、失敗が前回と同じ状態なら出さない
    // (stampは読む前に調べたファイルの状態。読んでいる途中で変わっても、次の確認で気付けるように、読む前の状態を使う)
    static std::shared_ptr<const Table> ReadTable(const std::wstring& path, const FileStamp& stamp, const FileStamp* knownFailure);

    // 行と列を確かめて、値のある場所を返す(使えなければ知らせてfalse)
    bool GetCell(int row, const std::string& column, const wchar_t* functionName, const std::wstring*& cell) const;
};
