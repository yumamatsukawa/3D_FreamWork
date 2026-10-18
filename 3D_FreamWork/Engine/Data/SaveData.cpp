#include "SaveData.h"
#include <windows.h>
#include <shlobj.h>
#include <map>
#include <set>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <clocale>
#pragma comment(lib, "shell32.lib")   // SHGetKnownFolderPath
#pragma comment(lib, "ole32.lib")     // CoTaskMemFree

// ファイルの形式(メモ帳で開いて確認・書き換えできる):
// - UTF-8(BOM付きで書く。読む時はBOMがあっても無くてもよい)。改行はCRLFで書く(LFだけでも読める)
// - 1行に「キー=値」。キーの順に並べる。キーの前後の空白は読む時に無視する
// - 値は「最初の = の後ろから行の終わりまで」をそのまま使う(値の中の = 、先頭・末尾の空白もそのまま残る)
// - 値の中の \ 、改行、CR は、\\ 、\n 、\r と書く(1行に1つの値を収めるため)。それ以外の \ の後ろの文字はそのまま
// - 空の行と、# で始まる行は飛ばす(# の行は手で書いたメモ用。Saveし直すと消える)
// - = が無い行、キーに使えない文字がある行、UTF-8として読めない行は、壊れた行として飛ばす
namespace {
    std::map<std::string, std::string> values;   // キー → 値(UTF-8の文字。mapなのでキーの順に並ぶ)
    int currentSlot = 0;                         // 今の値がどのスロットのものか(最後にLoad/Saveに成功したスロット)
    bool hasSlot = false;                        // Load/Saveに1回でも成功したか(falseの間はcurrentSlotが決まっていない)
    bool dirty = false;                         // 最後のLoad/Saveの後に書き換えたか(終了時の自動保存に使う)
    std::set<std::string> warnedIDs;             // 知らせ済みの問題(毎フレームGetされても1回だけ知らせる)
    _locale_t cLocale = nullptr;                 // 小数の読み書き用(PCの地域設定で小数点が , にならないように)

    constexpr LONGLONG maxFileSize = 16 * 1024 * 1024;   // これより大きいファイルは読まない・書かない(セーブデータとしてはありえない大きさ)

    void Log(const std::wstring& message) {
        OutputDebugStringW((L"★ SaveData: " + message + L"\n").c_str());
    }

    // 同じ問題は1回だけ知らせる
    void WarnOnce(const std::string& id, const std::wstring& message) {
        if (warnedIDs.insert(id).second) Log(message);
    }

    // ─── 文字の変換 ─────────────
    bool Utf8ToWide(const std::string& text, std::wstring& out) {
        out.clear();
        if (text.empty()) return true;
        int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), nullptr, 0);
        if (length <= 0) return false;
        out.resize(length);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), &out[0], length);
        return true;
    }

    bool WideToUtf8(const std::wstring& text, std::string& out) {
        out.clear();
        if (text.empty()) return true;
        int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
        if (length <= 0) return false;
        out.resize(length);
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), (int)text.size(), &out[0], length, nullptr, nullptr);
        return true;
    }

    // 出力ウィンドウに出す用(読めない文字が入っていても何か表示する)
    std::wstring ToDisplay(const std::string& text, UINT codePage) {
        std::wstring out;
        int length = MultiByteToWideChar(codePage, 0, text.data(), (int)text.size(), nullptr, 0);
        if (length <= 0) return L"";
        out.resize(length);
        MultiByteToWideChar(codePage, 0, text.data(), (int)text.size(), &out[0], length);
        return out;
    }

    // ─── キー ─────────────
    bool IsValidKey(const std::string& key) {
        if (key.empty()) return false;
        for (char c : key) {
            bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
            if (!ok) return false;
        }
        return true;
    }

    // キーが使えなければ知らせてfalse(キーはソースコードに書いた文字なので、Shift-JISとして表示する)
    bool CheckKey(const std::string& key, const wchar_t* functionName) {
        if (IsValidKey(key)) return true;
        WarnOnce("Key:" + key, std::wstring(functionName) + L": キー \"" + ToDisplay(key, CP_ACP)
            + L"\" は使えません(英字・数字・_ だけにしてください)。保存されず、Getはいつも defaultValue を返します");
        return false;
    }

    // ─── 値の変換 ─────────────
    _locale_t GetCLocale() {
        if (!cLocale) cLocale = _create_locale(LC_NUMERIC, "C");
        return cLocale;
    }

    std::string Trim(const std::string& text) {
        size_t begin = text.find_first_not_of(" \t");
        if (begin == std::string::npos) return "";
        size_t end = text.find_last_not_of(" \t");
        return text.substr(begin, end - begin + 1);
    }

    // 全体が整数として読めて、intに入る時だけtrue(前後の空白は無視。"1.5" や "12abc" はfalse)
    bool ParseInt(const std::string& text, int& out) {
        std::string s = Trim(text);
        if (s.empty()) return false;
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

    // 全体が数として読めて、無限大・NaNでない時だけtrue(前後の空白は無視)
    bool ParseFloat(const std::string& text, float& out) {
        std::string s = Trim(text);
        if (s.empty()) return false;
        char first = s[0];
        if (!((first >= '0' && first <= '9') || first == '-' || first == '+' || first == '.')) return false;
        char* end = nullptr;
        float value = _strtof_l(s.c_str(), &end, GetCLocale());
        if (end != s.c_str() + s.size() || !std::isfinite(value)) return false;
        out = value;
        return true;
    }

    // 読み直すと同じ値に戻る、一番短い書き方にする(0.1f は "0.100000001" ではなく "0.1")
    std::string FormatFloat(float value) {
        char buf[64] = {};
        for (int precision = 6; precision <= 9; precision++) {
            _snprintf_s_l(buf, sizeof(buf), _TRUNCATE, "%.*g", GetCLocale(), precision, (double)value);
            float parsed = 0.f;
            if (ParseFloat(buf, parsed) && parsed == value) break;   // 9桁なら必ず元に戻る
        }
        return buf;
    }

    // 値の \ ・改行・CR を \\ ・\n ・\r にする(1行に収めるため)
    std::string Escape(const std::string& value) {
        std::string out;
        for (char c : value) {
            if (c == '\\')      out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else                out += c;
        }
        return out;
    }

    std::string Unescape(const std::string& text) {
        std::string out;
        for (size_t i = 0; i < text.size(); i++) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                char next = text[i + 1];
                if (next == '\\') { out += '\\'; i++; continue; }
                if (next == 'n')  { out += '\n'; i++; continue; }
                if (next == 'r')  { out += '\r'; i++; continue; }
            }
            out += text[i];   // 決まっていない \ の使い方は、そのまま残す
        }
        return out;
    }

    // 値を書く(同じ値なら書き換えたことにしない)
    void SetValue(const std::string& key, const std::string& value) {
        auto found = values.find(key);
        if (found != values.end() && found->second == value) return;
        values[key] = value;
        dirty = true;
        // 新しい値で、また読めない値になったら知らせ直す
        warnedIDs.erase("GetInt:" + key);
        warnedIDs.erase("GetFloat:" + key);
    }

    // ─── ファイル ─────────────
    // 保存先のパス(%LOCALAPPDATA%\3D_FreamWork\save{slot}.txt)。createFolderなら、フォルダが無ければ作る
    bool GetFilePath(int slot, bool createFolder, std::wstring& out) {
        PWSTR basePath = nullptr;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &basePath);
        if (FAILED(hr)) {
            CoTaskMemFree(basePath);
            Log(L"保存先のフォルダ(%LOCALAPPDATA%)が分かりませんでした");
            return false;
        }
        std::wstring folder = std::wstring(basePath) + L"\\3D_FreamWork";
        CoTaskMemFree(basePath);

        if (createFolder && !CreateDirectoryW(folder.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
            Log(L"保存先のフォルダを作れませんでした(エラー " + std::to_wstring(GetLastError()) + L"): " + folder);
            return false;
        }
        out = folder + L"\\save" + std::to_wstring(slot) + L".txt";
        return true;
    }

    // ファイルの中身を読んで、キーと値に分ける(壊れた行は飛ばす)
    std::map<std::string, std::string> Parse(const std::string& content, const std::wstring& path) {
        std::map<std::string, std::string> result;
        size_t pos = 0;
        if (content.compare(0, 3, "\xEF\xBB\xBF") == 0) pos = 3;   // BOMは飛ばす

        int lineNumber = 0;
        while (pos < content.size()) {
            size_t end = content.find('\n', pos);
            if (end == std::string::npos) end = content.size();
            std::string line = content.substr(pos, end - pos);
            pos = end + 1;
            lineNumber++;

            if (!line.empty() && line.back() == '\r') line.pop_back();   // CRLFのCR(値の中のCRは \r と書いてある)
            std::string trimmed = Trim(line);
            if (trimmed.empty() || trimmed[0] == '#') continue;          // 空の行・メモの行

            size_t equal = line.find('=');
            std::string key = (equal == std::string::npos) ? "" : Trim(line.substr(0, equal));
            std::string value = (equal == std::string::npos) ? "" : Unescape(line.substr(equal + 1));
            std::wstring check;
            if (!IsValidKey(key) || !Utf8ToWide(value, check)) {
                Log(path + L" の " + std::to_wstring(lineNumber) + L"行目は読めないので飛ばしました: " + ToDisplay(line, CP_UTF8));
                continue;
            }
            result[key] = value;   // 同じキーが2回あったら、後の行を使う
        }
        return result;
    }
}

namespace SaveData {
    void SetInt(const std::string& key, int value) {
        if (!CheckKey(key, L"SetInt")) return;
        SetValue(key, std::to_string(value));
    }

    int GetInt(const std::string& key, int defaultValue) {
        if (!CheckKey(key, L"GetInt")) return defaultValue;
        auto found = values.find(key);
        if (found == values.end()) return defaultValue;

        int value = 0;
        if (ParseInt(found->second, value)) return value;
        std::wstring text;
        Utf8ToWide(found->second, text);
        WarnOnce("GetInt:" + key, L"GetInt: \"" + ToDisplay(key, CP_ACP) + L"\" の値 \"" + text
            + L"\" は整数として読めないので、defaultValue を返します");
        return defaultValue;
    }

    void SetFloat(const std::string& key, float value) {
        if (!CheckKey(key, L"SetFloat")) return;
        // 無限大・NaNは読み直せないので書かない
        if (!std::isfinite(value)) {
            WarnOnce("SetFloat:" + key, L"SetFloat: \"" + ToDisplay(key, CP_ACP) + L"\" に無限大かNaNを書こうとしたので、書きませんでした");
            return;
        }
        SetValue(key, FormatFloat(value));
    }

    float GetFloat(const std::string& key, float defaultValue) {
        if (!CheckKey(key, L"GetFloat")) return defaultValue;
        auto found = values.find(key);
        if (found == values.end()) return defaultValue;

        float value = 0.f;
        if (ParseFloat(found->second, value)) return value;
        std::wstring text;
        Utf8ToWide(found->second, text);
        WarnOnce("GetFloat:" + key, L"GetFloat: \"" + ToDisplay(key, CP_ACP) + L"\" の値 \"" + text
            + L"\" は数として読めないので、defaultValue を返します");
        return defaultValue;
    }

    void SetString(const std::string& key, const std::wstring& value) {
        if (!CheckKey(key, L"SetString")) return;
        std::string utf8;
        if (!WideToUtf8(value, utf8)) {
            Log(L"SetString: \"" + ToDisplay(key, CP_ACP) + L"\" の文字に、UTF-8にできない文字が入っているので書きませんでした");
            return;
        }
        SetValue(key, utf8);
    }

    std::wstring GetString(const std::string& key, const std::wstring& defaultValue) {
        if (!CheckKey(key, L"GetString")) return defaultValue;
        auto found = values.find(key);
        if (found == values.end()) return defaultValue;

        std::wstring value;
        if (!Utf8ToWide(found->second, value)) return defaultValue;   // 書く時・読む時に確かめているので、普通は起きない
        return value;
    }

    bool HasKey(const std::string& key) {
        if (!CheckKey(key, L"HasKey")) return false;
        return values.count(key) > 0;
    }

    void DeleteKey(const std::string& key) {
        if (!CheckKey(key, L"DeleteKey")) return;
        if (values.erase(key) > 0) dirty = true;
    }

    bool Save(int slot) {
        if (slot < 0) {
            Log(L"Save: スロットは0以上にしてください(" + std::to_wstring(slot) + L")");
            return false;
        }
        std::wstring path;
        if (!GetFilePath(slot, true, path)) return false;

        // 中身を作る(BOM + 1行に「キー=値」)
        std::string content = "\xEF\xBB\xBF";
        for (const auto& pair : values) {
            content += pair.first + "=" + Escape(pair.second) + "\r\n";
        }
        // 次の起動で読めない大きさのファイルは作らない
        if ((LONGLONG)content.size() > maxFileSize) {
            Log(L"保存できませんでした(中身が16MBより大きい。Loadで読めなくなるため): " + path);
            return false;
        }

        // 一時ファイルに書く(ディスクに書き終わるまで待つ)
        std::wstring tempPath = path + L".tmp";
        HANDLE file = CreateFileW(tempPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            Log(L"保存できませんでした(一時ファイルを作れない。エラー " + std::to_wstring(GetLastError()) + L"): " + tempPath);
            return false;
        }
        DWORD written = 0;
        bool ok = WriteFile(file, content.data(), (DWORD)content.size(), &written, nullptr) && written == content.size();
        ok = ok && FlushFileBuffers(file);
        DWORD writeError = GetLastError();
        CloseHandle(file);
        if (!ok) {
            DeleteFileW(tempPath.c_str());
            Log(L"保存できませんでした(一時ファイルに書けない。エラー " + std::to_wstring(writeError) + L"): " + tempPath);
            return false;
        }

        // 本物と差し替える(失敗しても本物はそのまま残る)
        if (!MoveFileExW(tempPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DWORD moveError = GetLastError();
            DeleteFileW(tempPath.c_str());
            Log(L"保存できませんでした(差し替えられない。エラー " + std::to_wstring(moveError) + L"): " + path);
            return false;
        }

        currentSlot = slot;
        hasSlot = true;
        dirty = false;
        Log(L"保存しました: " + path);
        return true;
    }

    bool Load(int slot) {
        if (slot < 0) {
            Log(L"Load: スロットは0以上にしてください(" + std::to_wstring(slot) + L")");
            return false;
        }
        std::wstring path;
        if (!GetFilePath(slot, false, path)) return false;

        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            // ファイルが無いのは正常(初めての起動)。空の状態から始める
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
                values.clear();
                currentSlot = slot;
                hasSlot = true;
                dirty = false;
                Log(L"セーブデータがまだ無いので、空の状態から始めます: " + path);
                return true;
            }
            Log(L"読み込めませんでした(エラー " + std::to_wstring(error) + L"): " + path);
            return false;
        }

        // 全部読む
        LARGE_INTEGER size = {};
        bool ok = GetFileSizeEx(file, &size) && size.QuadPart <= maxFileSize;
        std::string content;
        if (ok) {
            content.resize((size_t)size.QuadPart);
            DWORD read = 0;
            ok = content.empty() || (ReadFile(file, &content[0], (DWORD)content.size(), &read, nullptr) && read == content.size());
        }
        CloseHandle(file);
        if (!ok) {
            Log(L"読み込めませんでした(読めない、または大きすぎる): " + path);
            return false;
        }

        values = Parse(content, path);
        currentSlot = slot;
        hasSlot = true;
        dirty = false;
        Log(L"読み込みました(" + std::to_wstring(values.size()) + L"個): " + path);
        return true;
    }

    void Init() {
        Load(0);
    }

    void Uninit() {
        // 書き換えた時だけ保存する(最後にLoad/Saveに成功したスロットへ。Load(1)した後なら save1.txt)
        // 1回も成功していない(起動時のLoad(0)が読めなかった)なら、読めなかったファイルを上書きしないように保存しない
        if (dirty && hasSlot) {
            Save(currentSlot);
        }
        else if (dirty) {
            Log(L"セーブデータを読み込めなかったので、終了時の自動保存はしません(読めなかったファイルを上書きしないため)");
        }
        if (cLocale) {
            _free_locale(cLocale);
            cLocale = nullptr;
        }
        warnedIDs.clear();
    }
}
