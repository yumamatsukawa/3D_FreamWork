#include "Input.h"
#include "Graphics.h"

namespace Input {
    BYTE         currentKey[256] = {};
    BYTE         previousKey[256] = {};
    POINT        mousePos = {};
    XINPUT_STATE currentPad[4] = {};
    XINPUT_STATE previousPad[4] = {};
    bool         padConnected[4] = {};
    HWND targetHwnd = nullptr;  // ★ 追加

    void SetHwnd(HWND hwnd) {
        targetHwnd = hwnd;
    }

    // ─── Update ──────────────────────────────
    void Update() {
        memcpy(previousKey, currentKey, sizeof(currentKey));
        GetKeyboardState(currentKey);

        GetCursorPos(&mousePos);
        if (targetHwnd) {
            ScreenToClient(targetHwnd, &mousePos);
        }

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
        return {
            (float)mousePos.x - Graphics::width / 2.f,
            (float)mousePos.y - Graphics::height / 2.f
        };
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