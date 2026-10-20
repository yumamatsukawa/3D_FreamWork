#include "SaveDemoController.h"
#include "../Common/DemoStatus.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Data/SaveData.h"
#include <cstdio>
#include <vector>
#include <windows.h>

namespace {
    // このデモが使うキー(キーに使える文字は英字・数字・_ だけ。サンプルのキーとぶつからないよう Demo_ を付ける)
    const char* countKey = "Demo_Count";
    const char* ratioKey = "Demo_Ratio";
    const char* nameKey = "Demo_Name";

    // 文字(日本語)の候補
    const wchar_t* names[4] = { L"勇者", L"魔法使い", L"ねこ", L"ドラゴン" };

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 blue = { 0.5f, 0.75f, 1.f, 1.f };
    const DirectX::XMFLOAT4 orange = { 1.f, 0.7f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.5f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };
}

void SaveDemoController::UpdateSavePath() {
    // %LOCALAPPDATA%\3D_FreamWork\save{slot}.txt(SaveData と同じ場所)
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    const std::wstring base = (length > 0 && length < MAX_PATH) ? std::wstring(buffer) : std::wstring(L"%LOCALAPPDATA%");
    savePath = base + L"\\3D_FreamWork\\save" + std::to_wstring(slot) + L".txt";
}

void SaveDemoController::Init() {
    // 最初はスロット1を読み込む(前に保存した値があれば、ここで出てくる)
    SelectSlot(firstSlot);
}

void SaveDemoController::SelectSlot(int newSlot) {
    if (newSlot < firstSlot || newSlot > lastSlot) return;
    slot = newSlot;
    UpdateSavePath();

    // 読み込むと今の値はすべて捨てて、ファイルの中身に置き換わる(ファイルが無いのは正常で、空になる)
    ready = SaveData::Load(slot);
    dirty = false;
    if (!ready) {
        message = L"読み込めませんでした(ほかのアプリが開いている・大きすぎる)。このスロットは書き換えません";
        messageError = true;
        return;
    }
    messageError = false;
    message = SaveData::HasKey(countKey) || SaveData::HasKey(ratioKey) || SaveData::HasKey(nameKey)
        ? L"スロット " + std::to_wstring(slot) + L" を読み込みました(前に保存した値)"
        : L"スロット " + std::to_wstring(slot) + L" は空です(値を変えて S で保存してみよう)";
}

void SaveDemoController::AddCount() {
    if (!ready) return;
    SaveData::SetInt(countKey, SaveData::GetInt(countKey, 0) + 1);
    dirty = true;
    message = L"整数を 1 増やしました(SetInt)。まだファイルには保存していません";
    messageError = false;
}

void SaveDemoController::AddRatio() {
    if (!ready) return;
    float value = SaveData::GetFloat(ratioKey, 0.f) + 0.1f;
    if (value > 1.001f) value = 0.f;
    SaveData::SetFloat(ratioKey, value);
    dirty = true;
    message = L"小数を 0.1 増やしました(SetFloat。1を超えたら0へ)。まだファイルには保存していません";
    messageError = false;
}

void SaveDemoController::NextName() {
    if (!ready) return;
    // 今の文字が候補の何番目かを探して、次へ(無ければ最初)
    const std::wstring current = SaveData::GetString(nameKey, L"");
    int index = -1;
    for (int i = 0; i < 4; i++) {
        if (current == names[i]) index = i;
    }
    SaveData::SetString(nameKey, names[(index + 1) % 4]);
    dirty = true;
    message = L"文字を変えました(SetString。日本語もそのまま保存できる)。まだファイルには保存していません";
    messageError = false;
}

void SaveDemoController::SaveNow() {
    if (!ready) return;
    if (SaveData::Save(slot)) {
        saveCount++;
        dirty = false;
        message = L"ファイルに保存しました(Save)。アプリを閉じて開き直しても残ります";
        messageError = false;
    }
    else {
        message = L"保存できませんでした(出力ウィンドウに理由が出ます)";
        messageError = true;
    }
}

void SaveDemoController::LoadNow() {
    if (!ready) return;
    // 保存していない変更は捨てられる(ファイルの中身に戻る)
    const bool hadChanges = dirty;
    if (SaveData::Load(slot)) {
        dirty = false;
        message = hadChanges ? L"ファイルから読み込みました(Load)。保存していない変更は捨てました" : L"ファイルから読み込みました(Load)";
        messageError = false;
    }
    else {
        message = L"読み込めませんでした。今の値のままです";
        messageError = true;
    }
}

void SaveDemoController::DeleteKeys() {
    if (!ready) return;
    SaveData::DeleteKey(countKey);
    SaveData::DeleteKey(ratioKey);
    SaveData::DeleteKey(nameKey);
    dirty = true;
    message = L"3つのキーを消しました(DeleteKey)。ファイルは S で保存するまで変わりません";
    messageError = false;
}

void SaveDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    if (Input::GetKeyDown(KEY_Z)) AddCount();
    if (Input::GetKeyDown(KEY_X)) AddRatio();
    if (Input::GetKeyDown(KEY_C)) NextName();
    if (Input::GetKeyDown(KEY_S)) SaveNow();
    if (Input::GetKeyDown(KEY_L)) LoadNow();
    if (Input::GetKeyDown(KEY_D)) DeleteKeys();
    if (Input::GetKeyDown(KEY_1)) SelectSlot(1);
    if (Input::GetKeyDown(KEY_2)) SelectSlot(2);
    if (Input::GetKeyDown(KEY_3)) SelectSlot(3);
}

void SaveDemoController::Draw() {
    wchar_t buf[300];
    std::vector<DemoStatusLine> lines;

    // スロットと保存先
    lines.push_back({ L"スロット " + std::to_wstring(slot) + L"(1〜3。0はサンプル用なので使わない)  保存先: " + savePath, blue });

    // メモリの上の値(GetInt / GetFloat / GetString。キーが無い時は(なし))
    std::wstring countText = SaveData::HasKey(countKey) ? std::to_wstring(SaveData::GetInt(countKey, 0)) : L"(なし)";
    std::wstring ratioText = L"(なし)";
    if (SaveData::HasKey(ratioKey)) {
        swprintf_s(buf, L"%.1f", SaveData::GetFloat(ratioKey, 0.f));
        ratioText = buf;
    }
    std::wstring nameText = SaveData::HasKey(nameKey) ? SaveData::GetString(nameKey, L"") : L"(なし)";
    lines.push_back({ L"今の値:  Demo_Count = " + countText + L"   Demo_Ratio = " + ratioText + L"   Demo_Name = " + nameText, orange });

    // 保存したか
    swprintf_s(buf, L"状態: %ls   Save に成功した回数 %d", !ready ? L"このスロットは使えません" : dirty ? L"未保存の変更があります(S で保存)" : L"ファイルと同じです", saveCount);
    lines.push_back({ buf, !ready ? red : dirty ? yellow : green });

    lines.push_back({ message, messageError ? red : green });
    lines.push_back({ L"Z 整数 +1   X 小数 +0.1   C 名前   S 保存   L 読込   D 消す   1 2 3 スロット(切り替えると、そのファイルを読み込む)", green });
    DrawDemoStatus(lines, 1230.f);

    // ボタンの列の見出し(ボタンは CreateSaveDemo が置く)
    Text::Draw(L"値を変える(メモリの上だけ)", -400.f, 30.f, MakeDemoTextStyle(20.f, orange, TextAlign::Center));
    Text::Draw(L"ファイルとやり取り", -100.f, 30.f, MakeDemoTextStyle(20.f, blue, TextAlign::Center));
    Text::Draw(L"スロット(別のファイル)", 200.f, 30.f, MakeDemoTextStyle(20.f, white, TextAlign::Center));

    // 使い方(左下の Back ボタンと重ならないよう、右へずらす)
    Text::Draw(L"Save を忘れても、アプリを終了する時に変更があれば Engine が自動で保存します(最後に使ったスロットへ)", -410.f, -250.f, MakeDemoTextStyle(18.f, white));
    Text::Draw(L"サンプルのセーブ(スロット0。ハイスコアなど)には触りません。値は、保存してアプリを開き直すと確かめられます", -410.f, -280.f, MakeDemoTextStyle(18.f, white));
}
