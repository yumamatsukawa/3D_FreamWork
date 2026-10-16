#include "Csv.h"
#include <windows.h>
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <clocale>

namespace {
    constexpr LONGLONG maxFileSize = 16 * 1024 * 1024;   // これより大きいファイルは読まない(表としてはありえない大きさ)
    constexpr UINT shiftJisCodePage = 932;               // Shift-JIS(CP932)

    void Log(const std::wstring& path, const std::wstring& message) {
        OutputDebugStringW((L"★ Csv: " + path + L": " + message + L"\n").c_str());
    }

    // ─── 文字の変換 ─────────────
    // 指定の文字コードとして読む。flagsにMB_ERR_INVALID_CHARSを付ければ、不正なバイトがある時にfalse
    bool ToWide(const char* data, size_t size, UINT codePage, DWORD flags, std::wstring& out) {
        out.clear();
        if (size == 0) return true;
        int length = MultiByteToWideChar(codePage, flags, data, (int)size, nullptr, 0);
        if (length <= 0) return false;
        out.resize(length);
        MultiByteToWideChar(codePage, flags, data, (int)size, &out[0], length);
        return true;
    }

    // 出力ウィンドウに出す用(ソースコードに書いた列の名前などを、そのまま表示する)
    std::wstring ToDisplay(const std::string& text) {
        std::wstring out;
        ToWide(text.data(), text.size(), CP_ACP, 0, out);
        return out;
    }

    // ─── 文字の整形 ─────────────
    bool IsSpace(wchar_t c) {
        return c == L' ' || c == L'\t' || c == 0x3000;   // 半角・タブ・全角の空白
    }

    std::wstring Trim(const std::wstring& text) {
        size_t begin = 0;
        size_t end = text.size();
        while (begin < end && IsSpace(text[begin])) begin++;
        while (end > begin && IsSpace(text[end - 1])) end--;
        return text.substr(begin, end - begin);
    }

    // 半角(ASCII)の文字だけならtrueにして、std::stringにする(列の名前・数の変換用)
    bool ToAscii(const std::wstring& text, std::string& out) {
        out.clear();
        for (wchar_t c : text) {
            if (c <= 0 || c >= 128) return false;
            out += (char)c;
        }
        return true;
    }

    // ─── 値の変換(SaveDataと同じ厳しさ) ─────────────
    struct LocaleHolder {
        _locale_t locale = _create_locale(LC_NUMERIC, "C");   // 小数の読み書き用(PCの地域設定で小数点が , にならないように)
        ~LocaleHolder() { if (locale) _free_locale(locale); }
    };

    _locale_t GetCLocale() {
        static LocaleHolder holder;
        return holder.locale;
    }

    // 全体が整数として読めて、intに入る時だけtrue(前後の空白は無視。"1.5" や "12abc" はfalse)
    bool ParseInt(const std::wstring& text, int& out) {
        std::string s;
        if (!ToAscii(Trim(text), s) || s.empty()) return false;
        char first = s[0];
        if (!((first >= '0' && first <= '9') || first == '-' || first == '+')) return false;
        char* end = nullptr;
        errno = 0;
        long long value = _strtoi64_l(s.c_str(), &end, 10, GetCLocale());
        if (end != s.c_str() + s.size() || errno == ERANGE) return false;
        if (value < INT_MIN || value > INT_MAX) return false;
        out = (int)value;
        return true;
    }

    // 全体が数として読めて、無限大・NaNでない時だけtrue(前後の空白は無視。16進数の "0x10" も読まない)
    bool ParseFloat(const std::wstring& text, float& out) {
        std::string s;
        if (!ToAscii(Trim(text), s) || s.empty()) return false;
        char first = s[0];
        if (!((first >= '0' && first <= '9') || first == '-' || first == '+' || first == '.')) return false;
        if (s.find_first_of("xX") != std::string::npos) return false;
        char* end = nullptr;
        float value = _strtof_l(s.c_str(), &end, GetCLocale());
        if (end != s.c_str() + s.size() || !std::isfinite(value)) return false;
        out = value;
        return true;
    }

    // ─── 文字コードの判定 ─────────────
    // UTF-8として読む。不正なバイトがあれば、Shift-JISとして読み直す(shiftJisがtrueになる)
    // Shift-JISとしても読めない所があれば、読めない所は代わりの文字になる(brokenがtrueになる)
    std::wstring Decode(const std::string& bytes, size_t offset, bool& shiftJis, bool& broken) {
        shiftJis = false;
        broken = false;
        const char* data = bytes.data() + offset;
        size_t size = bytes.size() - offset;

        std::wstring text;
        if (ToWide(data, size, CP_UTF8, MB_ERR_INVALID_CHARS, text)) return text;

        shiftJis = true;
        if (ToWide(data, size, shiftJisCodePage, MB_ERR_INVALID_CHARS, text)) return text;

        broken = true;
        ToWide(data, size, shiftJisCodePage, 0, text);
        return text;
    }

    // ─── 表の読み取り ─────────────
    // 文字全体を「行 → 値」に分ける。引用符で囲まれた値の中のカンマ・改行は、そのまま値に入る
    // 全部の値が空の行(空行・Excelが書く ",,")は飛ばす
    std::vector<std::vector<std::wstring>> ParseRecords(const std::wstring& text, bool& unclosedQuote) {
        std::vector<std::vector<std::wstring>> records;
        std::vector<std::wstring> record;
        std::wstring field;
        bool inQuotes = false;        // 引用符の中か
        bool fieldQuoted = false;     // この値は引用符で始まったか("" だけの空の値でも、後ろの " を文字として扱うため)
        bool recordStarted = false;   // この行に1文字でもあったか(最後の行に改行が無い時、終わらせるか決めるため)
        unclosedQuote = false;

        auto endField = [&]() {
            record.push_back(field);
            field.clear();
            fieldQuoted = false;
        };
        auto endRecord = [&]() {
            endField();
            bool allEmpty = true;
            for (const auto& value : record) {
                if (!value.empty()) { allEmpty = false; break; }
            }
            if (!allEmpty) records.push_back(std::move(record));
            record.clear();
            recordStarted = false;
        };

        for (size_t i = 0; i < text.size(); i++) {
            wchar_t c = text[i];
            if (inQuotes) {
                if (c == L'"') {
                    if (i + 1 < text.size() && text[i + 1] == L'"') { field += L'"'; i++; }   // "" は " 1文字
                    else inQuotes = false;                                                    // 閉じる
                }
                else field += c;   // カンマ・改行もそのまま値に入れる
                continue;
            }
            if (c == L'"' && field.empty() && !fieldQuoted) {   // 値の先頭の " だけが、囲みの始まり
                inQuotes = true;
                fieldQuoted = true;
                recordStarted = true;
            }
            else if (c == L',') {
                endField();
                recordStarted = true;
            }
            else if (c == L'\r' || c == L'\n') {
                if (c == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n') i++;   // CRLFは1つの改行
                endRecord();
            }
            else {
                field += c;
                recordStarted = true;
            }
        }
        if (inQuotes) unclosedQuote = true;
        if (recordStarted) endRecord();   // 最後の行に改行が無い場合
        return records;
    }
}

// 表の中身。Loadで作り、その後は変わらない(知らせ済みの記録だけ、constのGetでも書き換える)
struct Csv::Table {
    std::wstring path;                           // 読んだファイル(出力ウィンドウの表示用)
    std::map<std::string, int> columnIndex;      // 列の名前 → 何番目の列か
    int columnCount = 0;                         // 1行目の値の数
    std::vector<std::vector<std::wstring>> rows; // 1行目を除いた行(どの行もcolumnCount個の値がある)
    mutable std::set<std::string> warnedIDs;     // 知らせ済みの問題(毎フレームGetされても1回だけ知らせる)

    // 同じ問題は1回だけ知らせる
    void WarnOnce(const std::string& id, const std::wstring& message) const {
        if (warnedIDs.insert(id).second) Log(path, message);
    }
};

Csv::FileStamp Csv::QueryStamp(const std::wstring& path) {
    FileStamp stamp;
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data) && !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        stamp.exists = true;
        stamp.writeTime = ((unsigned long long)data.ftLastWriteTime.dwHighDateTime << 32) | data.ftLastWriteTime.dwLowDateTime;
        stamp.size = ((unsigned long long)data.nFileSizeHigh << 32) | data.nFileSizeLow;
    }
    return stamp;
}

std::shared_ptr<const Csv::Table> Csv::ReadTable(const std::wstring& path, const FileStamp& stamp, const FileStamp* knownFailure) {
    // 知らせは貯めておいて、最後にまとめて出す(失敗が前回と同じ状態なら出さない。保存の途中で毎回失敗しても静かにするため)
    std::vector<std::wstring> messages;
    auto log = [&](const std::wstring& message) { messages.push_back(message); };
    auto fail = [&]() -> std::shared_ptr<const Table> {
        bool sameFailure = knownFailure && *knownFailure == stamp;
        if (!sameFailure) {
            for (const auto& message : messages) Log(path, message);
        }
        return nullptr;
    };

    // 共有はすべて許可する(Excelなどが開いたままでも読めるように。DELETEを許可しないと、相手がDELETE権限で開いている間は共有違反で読めない)
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            log(L"ファイルが見つかりません(パスは、実行した時の作業フォルダからの場所です)");
        } else {
            log(L"読み込めませんでした(エラー " + std::to_wstring(error) + L"。Excelなどで開いたままかもしれません)");
        }
        return fail();
    }

    // 全部読む
    LARGE_INTEGER size = {};
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart <= maxFileSize;
    std::string bytes;
    if (ok) {
        bytes.resize((size_t)size.QuadPart);
        DWORD read = 0;
        ok = bytes.empty() || (ReadFile(file, &bytes[0], (DWORD)bytes.size(), &read, nullptr) && read == bytes.size());
    }
    CloseHandle(file);
    if (!ok) {
        log(L"読み込めませんでした(読めない、または16MBより大きい)");
        return fail();
    }

    // BOMを取り除く(UTF-16は読めない)
    size_t offset = 0;
    if (bytes.compare(0, 3, "\xEF\xBB\xBF") == 0) offset = 3;
    if (bytes.compare(0, 2, "\xFF\xFE") == 0 || bytes.compare(0, 2, "\xFE\xFF") == 0) {
        log(L"UTF-16のファイルは読めません。Excelでは『CSV UTF-8(コンマ区切り)』で保存してください");
        return fail();
    }

    // 文字コード
    bool shiftJis = false;
    bool broken = false;
    std::wstring text = Decode(bytes, offset, shiftJis, broken);
    if (shiftJis) {
        log(L"Shift-JIS のようです。Excel では『CSV UTF-8(コンマ区切り)』で保存してください(今回はShift-JISとして読みます)");
    }
    if (broken) {
        log(L"読めない文字が含まれていました(その部分は文字化けします)");
    }

    // 行と値に分ける
    bool unclosedQuote = false;
    auto records = ParseRecords(text, unclosedQuote);
    if (unclosedQuote) {
        log(L"\" が閉じていません。最後の値が、ファイルの終わりまで続いているものとして読みました");
    }
    if (records.empty()) {
        log(L"中身が空です(1行目に列の名前が必要です)");
        return fail();
    }

    auto table = std::make_shared<Table>();
    table->path = path;

    // 1行目は列の名前
    const auto& header = records[0];
    table->columnCount = (int)header.size();
    for (int i = 0; i < table->columnCount; i++) {
        std::wstring wideName = Trim(header[i]);
        if (wideName.empty()) continue;   // 名前の無い列(取り出せない)
        std::string name;
        if (!ToAscii(wideName, name)) {
            log(std::to_wstring(i + 1) + L"番目の列の名前 \"" + wideName + L"\" は、半角の文字ではないので使えません(列の名前は半角英数字にしてください)");
            continue;
        }
        if (!table->columnIndex.emplace(name, i).second) {   // 同じ名前は最初のものを使う
            log(L"列の名前 \"" + wideName + L"\" が2回あります。最初のものを使います");
        }
    }

    // 2行目からがデータ(列の数をそろえる)
    int shortRows = 0;
    int longRows = 0;
    int firstShortRow = -1;
    int firstLongRow = -1;
    for (size_t r = 1; r < records.size(); r++) {
        auto& values = records[r];
        int rowNumber = (int)r - 1;
        if ((int)values.size() < table->columnCount) {
            if (firstShortRow < 0) firstShortRow = rowNumber;
            shortRows++;
        }
        else if ((int)values.size() > table->columnCount) {
            if (firstLongRow < 0) firstLongRow = rowNumber;
            longRows++;
        }
        values.resize(table->columnCount);   // 足りない分は空、多い分は捨てる
        table->rows.push_back(std::move(values));
    }
    if (shortRows > 0) {
        log(L"列の名前より値が少ない行が" + std::to_wstring(shortRows) + L"行あります(足りない分は空として読みます。最初の行: row " + std::to_wstring(firstShortRow) + L")");
    }
    if (longRows > 0) {
        log(L"列の名前より値が多い行が" + std::to_wstring(longRows) + L"行あります(多い分は無視します。最初の行: row " + std::to_wstring(firstLongRow) + L")");
    }

    // 読めた時は、貯めておいた知らせをすべて出す
    for (const auto& message : messages) Log(path, message);
    OutputDebugStringW((L"★ Csv: 読み込みました(" + std::to_wstring(table->rows.size()) + L"行): " + path + L"\n").c_str());
    return table;
}

Csv Csv::Load(const std::wstring& path) {
    Csv csv;
    csv.path = path;

    // ファイルの状態は、読む前に調べる(読んでいる途中で保存されても、次のIsFileUpdatedでtrueになる)
    FileStamp stamp = QueryStamp(path);
    csv.table = ReadTable(path, stamp, nullptr);
    if (csv.table) {
        csv.loadedStamp = stamp;   // 読めた時だけ「読み込み済み」にする
    } else {
        csv.failedStamp = stamp;   // 同じ状態の失敗は、Reloadで知らせ直さない
        csv.hasFailed = true;
    }
    return csv;
}

bool Csv::IsFileUpdated() const {
    if (path.empty()) return false;
    FileStamp now = QueryStamp(path);
    if (!now.exists) return false;   // 消された・保存の途中。今の中身のまま待つ
    return !(now == loadedStamp);    // 読めていなかった時のloadedStampは「存在しない」なので、ファイルがあればtrue
}

bool Csv::Reload() {
    if (path.empty()) return false;

    FileStamp stamp = QueryStamp(path);
    auto newTable = ReadTable(path, stamp, hasFailed ? &failedStamp : nullptr);
    if (!newTable) {
        // 失敗: 今の中身と読み込み済みの日時はそのまま(次のIsFileUpdatedでまた試せる)
        failedStamp = stamp;
        hasFailed = true;
        return false;
    }

    // 成功: 自分の中身だけ入れ替える(先にコピーしたCsvは古い中身のまま)
    table = newTable;
    loadedStamp = stamp;
    hasFailed = false;
    return true;
}

int Csv::GetRowCount() const {
    return table ? (int)table->rows.size() : 0;
}

int Csv::GetColumnCount() const {
    return table ? table->columnCount : 0;
}

bool Csv::HasColumn(const std::string& column) const {
    return table && table->columnIndex.count(column) > 0;
}

bool Csv::GetCell(int row, const std::string& column, const wchar_t* functionName, const std::wstring*& cell) const {
    if (!table) return false;   // 読めなかった表(Loadの時に知らせてある)

    auto found = table->columnIndex.find(column);
    if (found == table->columnIndex.end()) {
        table->WarnOnce("Column:" + column, std::wstring(functionName) + L": 列 \"" + ToDisplay(column)
            + L"\" がありません(1行目の列の名前と同じ綴りにしてください。大文字小文字は区別します)。defaultValue を返します");
        return false;
    }
    if (row < 0 || row >= (int)table->rows.size()) {
        table->WarnOnce("Row", std::wstring(functionName) + L": 行 " + std::to_wstring(row) + L" は範囲外です(行は0〜"
            + std::to_wstring((int)table->rows.size() - 1) + L"。FindRow が -1 を返した時もここに来ます)。defaultValue を返します");
        return false;
    }
    cell = &table->rows[row][found->second];
    return true;
}

std::wstring Csv::GetString(int row, const std::string& column) const {
    const std::wstring* cell = nullptr;
    if (!GetCell(row, column, L"GetString", cell)) return L"";
    return *cell;
}

int Csv::GetInt(int row, const std::string& column, int defaultValue) const {
    const std::wstring* cell = nullptr;
    if (!GetCell(row, column, L"GetInt", cell)) return defaultValue;

    int value = 0;
    if (ParseInt(*cell, value)) return value;
    if (Trim(*cell).empty()) {
        table->WarnOnce("Empty:" + column, L"GetInt: 列 \"" + ToDisplay(column) + L"\" に空の値があります。defaultValue を返します");
    } else {
        table->WarnOnce("BadInt:" + column, L"GetInt: 列 \"" + ToDisplay(column) + L"\" の値 \"" + *cell
            + L"\" は整数として読めないので、defaultValue を返します");
    }
    return defaultValue;
}

float Csv::GetFloat(int row, const std::string& column, float defaultValue) const {
    const std::wstring* cell = nullptr;
    if (!GetCell(row, column, L"GetFloat", cell)) return defaultValue;

    float value = 0.f;
    if (ParseFloat(*cell, value)) return value;
    if (Trim(*cell).empty()) {
        table->WarnOnce("Empty:" + column, L"GetFloat: 列 \"" + ToDisplay(column) + L"\" に空の値があります。defaultValue を返します");
    } else {
        table->WarnOnce("BadFloat:" + column, L"GetFloat: 列 \"" + ToDisplay(column) + L"\" の値 \"" + *cell
            + L"\" は数として読めないので、defaultValue を返します");
    }
    return defaultValue;
}

int Csv::FindRow(const std::string& column, const std::wstring& value) const {
    if (!table) return -1;
    auto found = table->columnIndex.find(column);
    if (found == table->columnIndex.end()) {
        table->WarnOnce("Column:" + column, L"FindRow: 列 \"" + ToDisplay(column)
            + L"\" がありません(1行目の列の名前と同じ綴りにしてください。大文字小文字は区別します)");
        return -1;
    }
    for (int r = 0; r < (int)table->rows.size(); r++) {
        if (table->rows[r][found->second] == value) return r;
    }
    return -1;
}
