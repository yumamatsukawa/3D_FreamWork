#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>
#include <string>

// Inputデモの表示役。キーボード・マウス・ゲームパッド(XInput)の今の状態を、画面に出す。
// ゲームパッドが無い環境でも動く(つながっていない時は「未接続」と出す)。カーソルの固定は全体の設定なので、Uninitで必ず戻す
class InputDemoController : public Component {
private:
    static constexpr float eventShowTime = 1.0f;     // 押した/離した瞬間のキーの名前を出しておく時間(秒)
    static constexpr float padCheckInterval = 0.5f;  // ゲームパッドがつながっているかを調べる間隔(秒)

    std::wstring lastDownName = L"-";      // 最後に押した(KeyDown)キーの名前
    std::wstring lastUpName = L"-";        // 最後に離した(KeyUp)キーの名前
    std::wstring lastPadDownName = L"-";   // 最後に押したパッドのボタンの名前
    float lastDownAge = 0.f;               // 押してからの時間
    float lastUpAge = 0.f;                 // 離してからの時間
    float lastPadDownAge = 0.f;            // パッドのボタンを押してからの時間
    float padCheckTimer = 0.f;             // 次にパッドを調べるまでの時間
    bool padConnected = false;             // 1番のゲームパッドがつながっているか

    void CheckPad();                       // XInputで、1番のパッドがつながっているかを調べる
    void DrawKeyboard() const;             // キーボードの欄
    void DrawMouse() const;                // マウスの欄(位置・動き・ボタン・十字の印)
    void DrawPad() const;                  // ゲームパッドの欄
    void DrawStick(float x, float y, DirectX::XMFLOAT2 value, const wchar_t* label) const;   // スティックの四角と点

public:
    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
