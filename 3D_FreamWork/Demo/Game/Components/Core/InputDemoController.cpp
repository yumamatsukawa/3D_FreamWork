#include "InputDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Render/Image.h"
#include <cstdio>
#include <vector>

namespace {
    // 調べるキーの名前(Tab と Esc は、デモの画面の切り替えに使うので入れない。F1 はカーソルの固定に使う)
    struct KeyName {
        int code;
        std::wstring name;
    };
    const std::vector<KeyName>& GetKeys() {
        static const std::vector<KeyName> keys = [] {
            std::vector<KeyName> list;
            for (wchar_t c = L'A'; c <= L'Z'; c++) list.push_back({ (int)c, std::wstring(1, c) });
            for (wchar_t c = L'0'; c <= L'9'; c++) list.push_back({ (int)c, std::wstring(1, c) });
            list.push_back({ KEY_UP, L"Up" });       list.push_back({ KEY_DOWN, L"Down" });
            list.push_back({ KEY_LEFT, L"Left" });   list.push_back({ KEY_RIGHT, L"Right" });
            list.push_back({ KEY_SPACE, L"Space" }); list.push_back({ KEY_ENTER, L"Enter" });
            list.push_back({ KEY_SHIFT, L"Shift" }); list.push_back({ KEY_CTRL, L"Ctrl" });
            list.push_back({ KEY_BACK, L"Back" });
            return list;
        }();
        return keys;
    }

    // 調べるパッドのボタンの名前
    struct PadButtonName {
        WORD button;
        const wchar_t* name;
    };
    constexpr PadButtonName padButtons[] = {
        { PAD_A, L"A" }, { PAD_B, L"B" }, { PAD_X, L"X" }, { PAD_Y, L"Y" },
        { PAD_LB, L"LB" }, { PAD_RB, L"RB" }, { PAD_START, L"Start" }, { PAD_BACK, L"Back" },
        { PAD_UP, L"Up" }, { PAD_DOWN, L"Down" }, { PAD_LEFT, L"Left" }, { PAD_RIGHT, L"Right" },
        { PAD_LTHUMB, L"LStick" }, { PAD_RTHUMB, L"RStick" },
    };

    constexpr float columnY = 190.f;       // 欄の1行目(説明の板の下)
    constexpr float lineGap = 30.f;        // 行の間隔
    constexpr float keyboardX = -610.f;    // キーボードの欄の左端
    constexpr float mouseX = -190.f;       // マウスの欄の左端
    constexpr float padX = 240.f;          // ゲームパッドの欄の左端
    constexpr size_t maxListChars = 26;    // 押している物の一覧の最大文字数(はみ出さないよう、超えたら省く)

    // 名前を並べた文字(長すぎたら省く)
    std::wstring TrimList(const std::wstring& list) {
        if (list.size() <= maxListChars) return list.empty() ? L"-" : list;
        return list.substr(0, maxListChars) + L"...";
    }

    TextStyle HeadingStyle() { return MakeDemoTextStyle(24.f, { 1.0f, 0.9f, 0.4f, 1.0f }); }
    TextStyle BodyStyle() { return MakeDemoTextStyle(20.f); }
}

void InputDemoController::CheckPad() {
    // 1番のパッドの状態を読めたらつながっている(Engineの Input は、つながっていない時は何も押されていない扱いにする)
    XINPUT_STATE state = {};
    padConnected = (XInputGetState(0, &state) == ERROR_SUCCESS);
}

void InputDemoController::Init() {
    CheckPad();
}

void InputDemoController::Update(float dt) {
    // つながっているかは、少しずつ間をあけて調べる(つながっていないパッドを毎フレーム調べると重くなることがあるため)
    padCheckTimer -= dt;
    if (padCheckTimer <= 0.f) {
        padCheckTimer = padCheckInterval;
        CheckPad();
    }

    // F1: カーソルの固定(固定すると、位置は動かず、動いた量(GetMouseDelta)だけが取れる)
    if (Input::GetKeyDown(KEY_F1)) Input::SetCursorLocked(!Input::IsCursorLocked());

    // 押した瞬間・離した瞬間のキー(出しておく時間を数える)
    lastDownAge += dt;
    lastUpAge += dt;
    lastPadDownAge += dt;
    for (const KeyName& key : GetKeys()) {
        if (Input::GetKeyDown(key.code)) { lastDownName = key.name; lastDownAge = 0.f; }
        if (Input::GetKeyUp(key.code)) { lastUpName = key.name; lastUpAge = 0.f; }
    }
    if (padConnected) {
        for (const PadButtonName& button : padButtons) {
            if (Input::GetKeyDown(button.button)) { lastPadDownName = button.name; lastPadDownAge = 0.f; }
        }
    }
}

void InputDemoController::DrawKeyboard() const {
    float y = columnY;
    Text::Draw(L"Keyboard", keyboardX, y, HeadingStyle());

    // 押している間(GetKeyPress)のキーの一覧
    std::wstring held;
    for (const KeyName& key : GetKeys()) {
        if (Input::GetKeyPress(key.code)) held += key.name + L" ";
    }
    y -= lineGap;
    Text::Draw(L"押している間(Press)", keyboardX, y, BodyStyle());
    y -= lineGap;
    Text::Draw(TrimList(held), keyboardX + 14.f, y, MakeDemoTextStyle(20.f, { 0.7f, 1.0f, 0.7f, 1.0f }));

    // 押した瞬間・離した瞬間(1フレームだけtrueなので、しばらく名前を残して見えるようにしている)
    y -= lineGap;
    Text::Draw(L"押した瞬間(Down): " + (lastDownAge < eventShowTime ? lastDownName : std::wstring(L"-")), keyboardX, y, BodyStyle());
    y -= lineGap;
    Text::Draw(L"離した瞬間(Up): " + (lastUpAge < eventShowTime ? lastUpName : std::wstring(L"-")), keyboardX, y, BodyStyle());
}

void InputDemoController::DrawMouse() const {
    float y = columnY;
    wchar_t buf[96];
    Text::Draw(L"Mouse", mouseX, y, HeadingStyle());

    // 位置(画面中央が(0,0)、Y+=下)、動いた量、ワールドの位置(2Dカメラを考慮、Y+=上)
    const DirectX::XMFLOAT2 position = Input::GetMousePosition();
    const DirectX::XMFLOAT2 delta = Input::GetMouseDelta();
    const DirectX::XMFLOAT2 world = Input::GetMouseWorldPosition();
    y -= lineGap;
    swprintf_s(buf, L"Position  (%.0f, %.0f)", position.x, position.y);
    Text::Draw(buf, mouseX, y, BodyStyle());
    y -= lineGap;
    swprintf_s(buf, L"Delta  (%.0f, %.0f)", delta.x, delta.y);
    Text::Draw(buf, mouseX, y, BodyStyle());
    y -= lineGap;
    swprintf_s(buf, L"World  (%.0f, %.0f)", world.x, world.y);
    Text::Draw(buf, mouseX, y, BodyStyle());

    // ボタン(押している間は ON)
    y -= lineGap;
    swprintf_s(buf, L"Left %s  Right %s  Middle %s",
        Input::GetKeyPress(MOUSE_LEFT) ? L"ON" : L"-", Input::GetKeyPress(MOUSE_RIGHT) ? L"ON" : L"-", Input::GetKeyPress(MOUSE_MIDDLE) ? L"ON" : L"-");
    Text::Draw(buf, mouseX, y, BodyStyle());

    // カーソルの固定
    y -= lineGap;
    Text::Draw(Input::IsCursorLocked() ? L"F1: カーソル固定 ON" : L"F1: カーソル固定 OFF", mouseX, y,
        MakeDemoTextStyle(20.f, { 0.6f, 0.9f, 1.0f, 1.0f }));

    // 十字の印(マウスの位置。Y+=下なので、画面座標にする時は符号を反転する。左ボタンを押すと大きくなる)
    const float armLength = Input::GetKeyPress(MOUSE_LEFT) ? 36.f : 22.f;
    const DirectX::XMFLOAT2 mark = { position.x, -position.y };
    Image::DrawRect(mark, { armLength, 3.f }, { 1.0f, 0.3f, 0.3f, 1.0f });
    Image::DrawRect(mark, { 3.f, armLength }, { 1.0f, 0.3f, 0.3f, 1.0f });
}

void InputDemoController::DrawStick(float x, float y, DirectX::XMFLOAT2 value, const wchar_t* label) const {
    // 四角(スティックが動く範囲)と、傾きに合わせた点
    constexpr float boxSize = 90.f;
    Image::DrawRect({ x, y }, { boxSize, boxSize }, { 0.0f, 0.0f, 0.0f, 0.6f });
    Image::DrawRect({ x, y }, { boxSize, 2.f }, { 1.0f, 1.0f, 1.0f, 0.3f });
    Image::DrawRect({ x, y }, { 2.f, boxSize }, { 1.0f, 1.0f, 1.0f, 0.3f });
    Image::DrawRect({ x + value.x * boxSize * 0.5f, y + value.y * boxSize * 0.5f }, { 14.f, 14.f }, { 1.0f, 0.9f, 0.4f, 1.0f });

    wchar_t buf[48];
    swprintf_s(buf, L"%s (%.2f, %.2f)", label, value.x, value.y);
    Text::Draw(buf, x - boxSize * 0.5f, y - boxSize * 0.5f - 18.f, MakeDemoTextStyle(16.f));
}

void InputDemoController::DrawPad() const {
    float y = columnY;
    Text::Draw(L"Gamepad (XInput)", padX, y, HeadingStyle());

    // つながっていない時は、これだけ出す
    y -= lineGap;
    if (!padConnected) {
        Text::Draw(L"未接続", padX, y, MakeDemoTextStyle(22.f, { 1.0f, 0.5f, 0.5f, 1.0f }));
        y -= lineGap;
        Text::Draw(L"パッドをつなぐと、ここに出ます", padX, y, MakeDemoTextStyle(18.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
        return;
    }
    Text::Draw(L"接続中", padX, y, MakeDemoTextStyle(22.f, { 0.7f, 1.0f, 0.7f, 1.0f }));

    // 押している間のボタン、押した瞬間のボタン
    std::wstring held;
    for (const PadButtonName& button : padButtons) {
        if (Input::GetKeyPress(button.button)) held += std::wstring(button.name) + L" ";
    }
    y -= lineGap;
    Text::Draw(L"Press: " + TrimList(held), padX, y, BodyStyle());
    y -= lineGap;
    Text::Draw(L"Down: " + (lastPadDownAge < eventShowTime ? lastPadDownName : std::wstring(L"-")), padX, y, BodyStyle());

    // トリガー(0〜1)
    wchar_t buf[64];
    y -= lineGap;
    swprintf_s(buf, L"Trigger  L %.2f   R %.2f", Input::GetPadTrigger(TRIGGER_LEFT), Input::GetPadTrigger(TRIGGER_RIGHT));
    Text::Draw(buf, padX, y, BodyStyle());

    // スティック(-1〜1)
    DrawStick(padX + 60.f, y - 75.f, Input::GetPadLeftStick(), L"L");
    DrawStick(padX + 230.f, y - 75.f, Input::GetPadRightStick(), L"R");
}

void InputDemoController::Draw() {
    DrawKeyboard();
    DrawMouse();
    DrawPad();
}

void InputDemoController::Uninit() {
    // カーソルの固定は全体の設定。固定したまま別の画面へ行かないように戻す
    Input::SetCursorLocked(false);
}
