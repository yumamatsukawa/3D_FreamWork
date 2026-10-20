#include "CsvDemoController.h"
#include "../Common/DemoStatus.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Render/Image.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <windows.h>

namespace {
    const wchar_t* csvPath = L"Assets/Demo/DemoTable.csv";   // デモ専用の表(サンプルの CSV は読まない)

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 blue = { 0.5f, 0.75f, 1.f, 1.f };
    const DirectX::XMFLOAT4 orange = { 1.f, 0.7f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.5f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 gray = { 0.6f, 0.6f, 0.6f, 1.f };

    // 表の位置(画面の中央が(0,0)、Y+=上)
    constexpr float tableLeft = -600.f;      // 表の左端
    constexpr float tableTop = 70.f;         // 表の上端
    constexpr float tableWidth = 1000.f;     // 表の幅
    constexpr float rowHeight = 30.f;        // 1行の高さ
    constexpr float cellTextSize = 21.f;     // セルの文字の大きさ

    // 列の左端(表の左端からのずらし量): 行の番号, Name, Hp, Speed, Memo
    constexpr float columnX[5] = { 12.f, 60.f, 270.f, 420.f, 610.f };
}

void CsvDemoController::Init() {
    // 読んでいるファイルの場所(画面に出す)。出力先の Assets/Demo/ の下にあるコピー
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetFullPathNameW(csvPath, MAX_PATH, buffer, nullptr);
    fullPath = (length > 0 && length < MAX_PATH) ? std::wstring(buffer) : std::wstring(csvPath);

    // 最初の読み込み(毎回ファイルを読むので、Initで1回だけ)
    table = Csv::Load(csvPath);
    message = table.IsValid() ? L"読み込めました(Csv::Load)" : L"読み込めません(ファイルが無い・空・読めない)";
    messageError = !table.IsValid();
}

void CsvDemoController::ReloadNow() {
    // 成功したら中身が入れ替わる。失敗したら今の中身のまま(保存の途中などで読めない時)
    if (table.Reload()) {
        reloadCount++;
        sinceReload = 0.f;
        flashTimer = flashSeconds;
        message = L"ファイルが保存し直されたので読み直しました(Reload)";
        messageError = false;
    }
    else {
        message = L"読み直せませんでした。今の中身をそのまま使っています(次の確認でまた試します)";
        messageError = true;
    }
}

void CsvDemoController::Update(float dt) {
    sinceReload += dt;
    flashTimer = (std::max)(flashTimer - dt, 0.f);

    // R: 今すぐ読み直す
    if (Input::GetKeyDown(KEY_R)) ReloadNow();

    // 0.5秒ごとに、ファイルが変わったかを調べる(更新日時と大きさを見るだけで軽い。毎フレームは調べない)
    if (checkTimer.Update(dt)) {
        checkCount++;
        if (table.IsFileUpdated()) ReloadNow();
    }
}

void CsvDemoController::Draw() {
    // 上の状態の板
    wchar_t buf[300];
    std::vector<DemoStatusLine> lines;
    lines.push_back({ L"編集するファイル(保存すると反映):  " + fullPath, blue });
    swprintf_s(buf, L"Csv::Load %ls   行 %d / 列 %d   Reload %d 回   IsFileUpdated を %.1f 秒ごとに確認(%d 回目)",
        table.IsValid() ? L"成功" : L"失敗", table.GetRowCount(), table.GetColumnCount(), reloadCount, checkInterval, checkCount);
    lines.push_back({ buf, orange });
    lines.push_back({ message, messageError ? red : green });
    lines.push_back({ L"メモ帳などで上のファイルを編集して保存してみよう(ゲームを止めなくてよい)。  R 今すぐ読み直す", green });
    DrawDemoStatus(lines, 1230.f);

    // 表(読めなかった時は、板だけ出して理由を書く)
    const int rowCount = (std::min)(table.GetRowCount(), maxRows);
    const float panelHeight = rowHeight * (float)(rowCount + 2) + 16.f;
    const DirectX::XMFLOAT2 panelSize = { tableWidth, panelHeight };
    const DirectX::XMFLOAT2 panelCenter = { tableLeft + tableWidth * 0.5f, tableTop - panelHeight * 0.5f };
    const bool flashing = flashTimer > 0.f;
    Image::DrawRect(panelCenter, { panelSize.x + 8.f, panelSize.y + 8.f }, flashing ? DirectX::XMFLOAT4{ 0.5f, 1.f, 0.5f, 1.f } : DirectX::XMFLOAT4{ 0.4f, 0.4f, 0.5f, 1.f });
    Image::DrawRect(panelCenter, panelSize, { 0.f, 0.f, 0.f, 0.8f });

    if (!table.IsValid()) {
        Text::Draw(L"表がありません(IsValid が false)。ファイルを置いて保存すると、自動で読み込まれます", tableLeft + 20.f, tableTop - 40.f, MakeDemoTextStyle(cellTextSize, red));
        return;
    }

    // 列の見出し: 列の名前と、取り出す関数(列が無ければ赤で「列なし」)
    const char* names[4] = { "Name", "Hp", "Speed", "Memo" };
    const wchar_t* headers[4] = { L"Name (GetString)", L"Hp (GetInt)", L"Speed (GetFloat)", L"Memo (GetString)" };
    bool has[4];
    float y = tableTop - rowHeight * 0.5f - 8.f;
    Text::Draw(L"#", tableLeft + columnX[0], y, MakeDemoTextStyle(cellTextSize, gray));
    for (int c = 0; c < 4; c++) {
        has[c] = table.HasColumn(names[c]);
        const std::wstring header = has[c] ? headers[c] : std::wstring(names[c], names[c] + strlen(names[c])) + L" (列なし)";
        Text::Draw(header, tableLeft + columnX[c + 1], y, MakeDemoTextStyle(cellTextSize, has[c] ? blue : red));
    }

    // 各行のセル(列が無い時は - を出す。値が読めない時は、Get の既定値になる)
    for (int row = 0; row < rowCount; row++) {
        y -= rowHeight;
        swprintf_s(buf, L"%d", row);
        Text::Draw(buf, tableLeft + columnX[0], y, MakeDemoTextStyle(cellTextSize, gray));
        if (has[0]) Text::Draw(table.GetString(row, "Name"), tableLeft + columnX[1], y, MakeDemoTextStyle(cellTextSize, white));
        if (has[1]) { swprintf_s(buf, L"%d", table.GetInt(row, "Hp")); Text::Draw(buf, tableLeft + columnX[2], y, MakeDemoTextStyle(cellTextSize, orange)); }
        if (has[2]) { swprintf_s(buf, L"%.1f", table.GetFloat(row, "Speed")); Text::Draw(buf, tableLeft + columnX[3], y, MakeDemoTextStyle(cellTextSize, orange)); }
        if (has[3]) Text::Draw(table.GetString(row, "Memo"), tableLeft + columnX[4], y, MakeDemoTextStyle(cellTextSize, white));
    }
    if (table.GetRowCount() > maxRows) {
        y -= rowHeight;
        swprintf_s(buf, L"... あと %d 行(画面には %d 行まで)", table.GetRowCount() - maxRows, maxRows);
        Text::Draw(buf, tableLeft + columnX[0], y, MakeDemoTextStyle(cellTextSize, gray));
    }

    // 名前で行を探す(FindRow)。表の下に1行
    const float findY = tableTop - panelHeight - 24.f;
    const float findLeft = tableLeft + 190.f;   // 左下の Back ボタン(x -610〜-450)と重ならないように右へずらす
    const int dragon = table.FindRow("Name", L"Dragon");
    if (dragon >= 0) swprintf_s(buf, L"FindRow(\"Name\", \"Dragon\") = %d 行目   その Hp = GetInt(%d, \"Hp\") = %d", dragon, dragon, table.GetInt(dragon, "Hp"));
    else swprintf_s(buf, L"FindRow(\"Name\", \"Dragon\") = -1(Dragon の行が無い)");
    Text::Draw(buf, findLeft, findY, MakeDemoTextStyle(cellTextSize, green));
    swprintf_s(buf, L"最後に読み込んでから %.1f 秒   (Excel の場合は「CSV UTF-8(コンマ区切り)」で保存)", sinceReload);
    Text::Draw(buf, findLeft, findY - 28.f, MakeDemoTextStyle(cellTextSize - 2.f, gray));
}
