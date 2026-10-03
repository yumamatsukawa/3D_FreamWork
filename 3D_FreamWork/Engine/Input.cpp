#include "Input.h"
#include "Graphics.h"
#include "Image.h"

namespace Input {
    BYTE         currentKey[256] = {};
    BYTE         previousKey[256] = {};
    POINT        mousePos = {};
    XINPUT_STATE currentPad[4] = {};
    XINPUT_STATE previousPad[4] = {};
    bool         padConnected[4] = {};
    HWND targetHwnd = nullptr;  // ★ 追加

    RECT              clientRect = {};          // ウィンドウの中(クライアント領域)の大きさ
    DirectX::XMFLOAT2 mouseDelta = { 0.f, 0.f };  // 前のフレームからのマウスの移動量
    DirectX::XMFLOAT2 lastMousePos = { 0.f, 0.f };  // 移動量を求める基準の位置(GetMousePositionと同じ単位)
    bool cursorLocked = false;    // カーソルを固定しているか(SetCursorLockedで設定)
    bool cursorCentered = false;  // 前のフレームで、実際にカーソルを中心へ戻したか
    bool cursorHidden = false;    // ShowCursor(FALSE)でカーソルを隠しているか(ShowCursorを必ず対で呼ぶため)
    bool wasActive = false;       // 前のフレームでウィンドウがアクティブだったか

    void SetHwnd(HWND hwnd) {
        targetHwnd = hwnd;
    }

    namespace {
        // クライアント座標 → GetMousePositionの単位(画面中央が原点、ゲーム画面の解像度に合わせた大きさ)。
        // ★ Graphics::width/heightはウィンドウ枠を含む大きさで、バックバッファはその大きさで作られ、
        //   ウィンドウの中(クライアント領域)に縮めて表示される。なのでクライアント領域の中での割合を、
        //   バックバッファの大きさに掛け直す(中心のずれと、縮めた分の大きさのずれを両方直す)
        DirectX::XMFLOAT2 ToGamePosition(POINT client) {
            float clientW = (float)(clientRect.right - clientRect.left);
            float clientH = (float)(clientRect.bottom - clientRect.top);
            if (clientW <= 0.f || clientH <= 0.f) {
                // 最小化中などで大きさが取れない時は、従来どおりの計算にする
                return { (float)client.x - Graphics::width / 2.f, (float)client.y - Graphics::height / 2.f };
            }
            return {
                ((float)client.x / clientW - 0.5f) * Graphics::width,
                ((float)client.y / clientH - 0.5f) * Graphics::height
            };
        }

        // カーソルの表示/非表示。ShowCursorは呼んだ回数を数える方式なので、状態が変わる時だけ1回ずつ呼んで対にする
        void SetCursorVisible(bool visible) {
            if (visible != cursorHidden) return;
            ShowCursor(visible ? TRUE : FALSE);
            cursorHidden = !visible;
        }

        // 自分のウィンドウが選ばれていて(一番手前)、最小化されていないか
        bool IsWindowActive() {
            return targetHwnd && GetForegroundWindow() == targetHwnd && !IsIconic(targetHwnd);
        }
    }

    // ─── Update ──────────────────────────────
    void Update() {
        memcpy(previousKey, currentKey, sizeof(currentKey));
        GetKeyboardState(currentKey);

        // マウス座標(クライアント座標)
        if (targetHwnd) GetClientRect(targetHwnd, &clientRect);
        GetCursorPos(&mousePos);
        if (targetHwnd) {
            ScreenToClient(targetHwnd, &mousePos);
        }

        // 別のウィンドウを選んだ・最小化した時は、カーソルの固定を解除する
        bool active = IsWindowActive();
        if (!active) cursorLocked = false;

        // マウスの移動量。固定した直後・アクティブに戻った直後は、基準の位置が当てにならないので0にする
        DirectX::XMFLOAT2 pos = ToGamePosition(mousePos);
        bool skipDelta = (active && !wasActive) || (cursorLocked && !cursorCentered);
        mouseDelta = skipDelta ? DirectX::XMFLOAT2{ 0.f, 0.f }
                               : DirectX::XMFLOAT2{ pos.x - lastMousePos.x, pos.y - lastMousePos.y };
        lastMousePos = pos;
        wasActive = active;

        // カーソルの固定: 中心へ戻して隠す
        cursorCentered = false;
        if (cursorLocked && targetHwnd) {
            POINT center = { (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };
            POINT screenCenter = center;
            ClientToScreen(targetHwnd, &screenCenter);
            SetCursorPos(screenCenter.x, screenCenter.y);

            // ★ ウィンドウが画面の外にはみ出していて中心が画面外だと、SetCursorPosは画面の端で止まる。
            //   中心を基準にすると毎フレーム同じずれが移動量に出て、視点が回り続けてしまうので、
            //   実際にカーソルが移動した先を読み直して、次のフレームの基準にする
            POINT actual;
            GetCursorPos(&actual);
            ScreenToClient(targetHwnd, &actual);
            lastMousePos = ToGamePosition(actual);
            cursorCentered = true;
        }
        SetCursorVisible(!cursorLocked);

        for (int i = 0; i < 4; i++) {
            previousPad[i] = currentPad[i];
            padConnected[i] = (XInputGetState(i, &currentPad[i]) == ERROR_SUCCESS);
        }
    }

    // ─── キーボード・マウス共通 ───────────────
    bool GetKeyDown(int key) {
        return !(previousKey[key] & 0x80) && (currentKey[key] & 0x80);
    }

    bool GetKeyPress(int key) {
        return (currentKey[key] & 0x80) != 0;
    }

    bool GetKeyUp(int key) {
        return (previousKey[key] & 0x80) && !(currentKey[key] & 0x80);
    }

    // ─── パッド ───────────────────────────────
    bool GetKeyDown(WORD button, int padIndex) {
        if (!padConnected[padIndex]) return false;
        bool cur = (currentPad[padIndex].Gamepad.wButtons & button) != 0;
        bool prev = (previousPad[padIndex].Gamepad.wButtons & button) != 0;
        return !prev && cur;
    }

    bool GetKeyPress(WORD button, int padIndex) {
        if (!padConnected[padIndex]) return false;
        return (currentPad[padIndex].Gamepad.wButtons & button) != 0;
    }

    bool GetKeyUp(WORD button, int padIndex) {
        if (!padConnected[padIndex]) return false;
        bool cur = (currentPad[padIndex].Gamepad.wButtons & button) != 0;
        bool prev = (previousPad[padIndex].Gamepad.wButtons & button) != 0;
        return prev && !cur;
    }

    // ─── マウス座標 ───────────────────────────
    DirectX::XMFLOAT2 GetMousePosition() {
        return ToGamePosition(mousePos);
    }

    DirectX::XMFLOAT2 GetMouseDelta() {
        return mouseDelta;
    }

    // ─── カーソルの固定 ───────────────────────
    void SetCursorLocked(bool locked) {
        cursorLocked = locked;
        // 解除はすぐにカーソルを表示する(固定の開始は、次のUpdate()でウィンドウの状態を見てから)
        if (!locked) SetCursorVisible(true);
    }

    bool IsCursorLocked() {
        return cursorLocked;
    }

    // ★ 2Dスプライト(UIモード)は「ワールド座標 - カメラ位置」がそのまま画面中央からの
    //   ピクセル数になる(Z=0のオブジェクトの場合)ので、逆にたどると
    //   ワールド座標 = カメラ位置 + マウス座標(Yは符号を反転して上方向に合わせる)
    DirectX::XMFLOAT2 GetMouseWorldPosition() {
        const Camera& camera = Image::GetCamera();
        DirectX::XMFLOAT2 screen = GetMousePosition();
        return { camera.position.x + screen.x, camera.position.y - screen.y };
    }

    // ─── スティック ───────────────────────────
    DirectX::XMFLOAT2 GetPadLeftStick(int padIndex) {
        if (!padConnected[padIndex]) return { 0.f, 0.f };
        float x = (float)currentPad[padIndex].Gamepad.sThumbLX;
        float y = (float)currentPad[padIndex].Gamepad.sThumbLY;
        if (fabsf(x) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)  x = 0.f;
        if (fabsf(y) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)  y = 0.f;
        return { x / 32767.f, y / 32767.f };
    }

    DirectX::XMFLOAT2 GetPadRightStick(int padIndex) {
        if (!padConnected[padIndex]) return { 0.f, 0.f };
        float x = (float)currentPad[padIndex].Gamepad.sThumbRX;
        float y = (float)currentPad[padIndex].Gamepad.sThumbRY;
        if (fabsf(x) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) x = 0.f;
        if (fabsf(y) < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) y = 0.f;
        return { x / 32767.f, y / 32767.f };
    }

    // ─── トリガー ─────────────────────────────
    float GetPadTrigger(int trigger, int padIndex) {
        if (!padConnected[padIndex]) return 0.f;
        BYTE value = (trigger == TRIGGER_LEFT)
            ? currentPad[padIndex].Gamepad.bLeftTrigger
            : currentPad[padIndex].Gamepad.bRightTrigger;
        return value / 255.f;
    }
}