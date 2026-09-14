#pragma once
#include <windows.h>
#include <DirectXMath.h>
#include <Xinput.h>
#pragma comment(lib, "Xinput.lib")

// ─── キーボード定義 ───────────────────────────
#define KEY_UP        VK_UP    // ↑
#define KEY_DOWN      VK_DOWN  // ↓
#define KEY_LEFT      VK_LEFT  // ←
#define KEY_RIGHT     VK_RIGHT // →
#define KEY_SPACE     VK_SPACE
#define KEY_ENTER     VK_RETURN
#define KEY_ESCAPE    VK_ESCAPE
#define KEY_SHIFT     VK_SHIFT
#define KEY_CTRL      VK_CONTROL
#define KEY_ALT       VK_MENU
#define KEY_BACK      VK_BACK
#define KEY_TAB       VK_TAB
#define KEY_A         'A'
#define KEY_B         'B'
#define KEY_C         'C'
#define KEY_D         'D'
#define KEY_E         'E'
#define KEY_F         'F'
#define KEY_G         'G'
#define KEY_H         'H'
#define KEY_I         'I'
#define KEY_J         'J'
#define KEY_K         'K'
#define KEY_L         'L'
#define KEY_M         'M'
#define KEY_N         'N'
#define KEY_O         'O'
#define KEY_P         'P'
#define KEY_Q         'Q'
#define KEY_R         'R'
#define KEY_S         'S'
#define KEY_T         'T'
#define KEY_U         'U'
#define KEY_V         'V'
#define KEY_W         'W'
#define KEY_X         'X'
#define KEY_Y         'Y'
#define KEY_Z         'Z'
#define KEY_0         '0'
#define KEY_1         '1'
#define KEY_2         '2'
#define KEY_3         '3'
#define KEY_4         '4'
#define KEY_5         '5'
#define KEY_6         '6'
#define KEY_7         '7'
#define KEY_8         '8'
#define KEY_9         '9'
#define KEY_F1        VK_F1
#define KEY_F2        VK_F2
#define KEY_F3        VK_F3
#define KEY_F4        VK_F4
#define KEY_F5        VK_F5
#define KEY_F6        VK_F6
#define KEY_F7        VK_F7
#define KEY_F8        VK_F8
#define KEY_F9        VK_F9
#define KEY_F10       VK_F10
#define KEY_F11       VK_F11
#define KEY_F12       VK_F12

// ─── マウスボタン定義 ─────────────────────────
#define MOUSE_LEFT    VK_LBUTTON // 左クリック
#define MOUSE_RIGHT   VK_RBUTTON // 右クリック
#define MOUSE_MIDDLE  VK_MBUTTON // 中央クリック

// ─── パッドボタン定義 ─────────────────────────
#define PAD_UP        XINPUT_GAMEPAD_DPAD_UP
#define PAD_DOWN      XINPUT_GAMEPAD_DPAD_DOWN
#define PAD_LEFT      XINPUT_GAMEPAD_DPAD_LEFT
#define PAD_RIGHT     XINPUT_GAMEPAD_DPAD_RIGHT
#define PAD_START     XINPUT_GAMEPAD_START
#define PAD_BACK      XINPUT_GAMEPAD_BACK
#define PAD_LB        XINPUT_GAMEPAD_LEFT_SHOULDER
#define PAD_RB        XINPUT_GAMEPAD_RIGHT_SHOULDER
#define PAD_A         XINPUT_GAMEPAD_A
#define PAD_B         XINPUT_GAMEPAD_B
#define PAD_X         XINPUT_GAMEPAD_X
#define PAD_Y         XINPUT_GAMEPAD_Y
#define PAD_LTHUMB    XINPUT_GAMEPAD_LEFT_THUMB
#define PAD_RTHUMB    XINPUT_GAMEPAD_RIGHT_THUMB

// ─── トリガー定義 ─────────────────────────────
#define TRIGGER_LEFT  0
#define TRIGGER_RIGHT 1

namespace Input {
    void Update();
    void SetHwnd(HWND hwnd);

    // キーボード・マウス共通
    bool GetKeyDown(int key);
    bool GetKeyPress(int key);
    bool GetKeyUp(int key);

    // パッド
    bool GetKeyDown(WORD button, int padIndex = 0);
    bool GetKeyPress(WORD button, int padIndex = 0);
    bool GetKeyUp(WORD button, int padIndex = 0);

    // マウス座標
    DirectX::XMFLOAT2 GetMousePosition();

    // スティック・トリガー
    DirectX::XMFLOAT2 GetPadLeftStick(int padIndex = 0);
    DirectX::XMFLOAT2 GetPadRightStick(int padIndex = 0);
    float             GetPadTrigger(int trigger, int padIndex = 0);
}