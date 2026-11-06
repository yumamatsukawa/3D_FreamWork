#pragma once

// 入力の中身(2D/3Dどちらにも使える汎用の形)。クライアントがホストへ送り、ホストが Network::GetInput で読む。
// ボタンの番号(0〜15)はゲーム側で決める(例: 0=Jump, 1=Attack)
struct NetworkInput {
    float moveX = 0.f, moveY = 0.f;     // 移動の軸(-1〜1)。2Dは (左右, 上下)、3Dは (左右, 前後)
    float lookX = 0.f, lookY = 0.f;     // 視点・狙いの軸(-1〜1)。使わなければ 0
    unsigned short buttons = 0;         // ボタンを押している状態(ビット。最大16個)
    unsigned short pressed = 0;         // 「押した瞬間」のビット(1フレームの間だけ立つ)

    void SetButton(int bit, bool down) {   // bit は 0〜15
        if (bit < 0 || bit > 15) return;
        if (down) buttons |= (unsigned short)(1u << bit);
        else buttons &= (unsigned short)~(1u << bit);
    }
    bool IsButtonDown(int bit) const { return bit >= 0 && bit <= 15 && (buttons & (1u << bit)) != 0; }
    bool WasButtonPressed(int bit) const { return bit >= 0 && bit <= 15 && (pressed & (1u << bit)) != 0; }
};
