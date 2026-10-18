#pragma once
#include "../Core/Component.h"
#include "Anchor.h"
#include "Text.h"
#include <DirectXMath.h>
#include <functional>
#include <string>

// マウスのドラッグで値を変えるスライダー(音量の設定など)を描いて動かすComponent。
//
// 使い方:
//   auto* slider = obj->AddComponent<SliderComponent>();
//   slider->SetRange(0.f, 1.f);                              // 値の範囲
//   slider->SetValue(0.8f);                                  // 最初の値
//   slider->SetAnchor(Anchor::Bottom, { 0.f, 80.f });        // 画面の下の真ん中から、上へ80(下の辺の真ん中がそこに来る)
//   slider->SetSize(300.f, 12.f);                            // 溝の幅と高さ
//   slider->SetLabel(L"Volume", style);                      // スライダーの上に出す文字(省略できる)
//   slider->SetOnChanged([](float v) { Audio::SetGroupVolume(AudioGroup::BGM, v); });   // 動かして値が変わった時に呼ばれる
//   slider->SetOnDragEnd([](float v) { SaveData::Save(); });           // つまみを離した時に1回呼ばれる(保存など)
//
// 決まり:
// - 画面に固定して表示する(Image::DrawRect。カメラの影響を受けない)。位置は SetAnchor(基準の位置+ずらす量)で決める。
//   画面座標は「画面中央が(0,0)、Y+=上」。溝の anchor 側の角(辺)が GetAnchoredPosition(anchor, offset) に合う(はみ出さない)
// - 溝のどこか(つまみのまわりも含む)を押すと、そこへつまみが動いて、押している間はマウスに付いてくる。
//   マウスが溝や画面の外に出ても、離すまでは最後まで付いてくる(値は範囲に収まる)。離したら終わり
//   (マウスの座標は Input::GetMousePosition() を、画面座標に直して(Yの符号を反転)比べている)
// - 値は SetRange の範囲に収まる。SetStep(刻み幅) を決めると、最小値から刻み幅ごとの値だけになる(0なら刻まない)
// - SetOnChanged は、つまみを動かして値が実際に変わった時だけ呼ばれる(同じ値のままなら呼ばれない)。
//   コードから SetValue / SetRange / SetStep で変えた時は呼ばれない(最初の値を決める時に、保存などが動かないように)
// - ポーズ中・シーン切り替え中(Time::IsPaused())は操作できない(つかんでいたら離したことになる)。
//   GameObject が非アクティブになった時も、つかんでいたら離したことになる
// - ウィンドウの外でボタンを離した時も、離したことになる(Input が気付けない場合に備えて、物理的なボタンの状態 GetAsyncKeyState も見ている)
// - ★ つかんでいる間に、このComponentだけを SetEnabled(false) しない(Update が呼ばれなくなって検知できず、
//   IsAnyDragging() が true のままになり、ボタンが反応しなくなる)。止めたい時は GameObject ごと SetActive(false) にする(離したことになる)
// - つかんでいる間は、ボタン(ButtonComponent)は反応しない(マウスが乗っても、離しても押されない)
class SliderComponent : public Component {
private:
    static constexpr float handleWidth = 16.f;         // つまみの幅(ピクセル)
    static constexpr float handleExtraHeight = 12.f;   // つまみの高さは、溝の高さよりこれだけ大きい(上下に半分ずつはみ出す)
    static constexpr float labelGap = 6.f;             // つまみの上の端から文字までの隙間(ピクセル)

    // 値
    float minValue = 0.f;     // 最小値
    float maxValue = 1.f;     // 最大値
    float value = 0.f;        // 今の値
    float step = 0.f;         // 刻み幅(0なら刻まない)
    std::function<void(float)> onChanged;   // 値が変わった時に呼ぶ
    std::function<void(float)> onDragEnd;   // つまみを離した時に呼ぶ

    // 見た目・位置
    Anchor anchor = Anchor::Center;                                 // 画面のどこを基準にするか
    DirectX::XMFLOAT2 offset{ 0.f, 0.f };                           // 基準からずらす量(画面座標)
    DirectX::XMFLOAT2 size{ 300.f, 12.f };                          // 溝の幅と高さ(ピクセル)
    DirectX::XMFLOAT4 trackColor{ 0.f, 0.f, 0.f, 0.6f };            // 溝の色
    DirectX::XMFLOAT4 fillColor{ 0.3f, 0.7f, 1.f, 1.f };            // 溝の左からつまみまでの色
    DirectX::XMFLOAT4 handleColor{ 1.f, 1.f, 1.f, 1.f };            // つまみの色(つかんでいる間は少し暗くなる)
    std::wstring label;                                             // スライダーの上に出す文字(空なら出さない)
    TextStyle labelStyle;                                           // 文字の見た目

    // 操作
    bool dragging = false;    // つまみをつかんでいるか

    DirectX::XMFLOAT2 GetCenter() const;             // 溝の中心の画面座標
    float GetValueRatio() const;                     // 今の値の位置(0〜1)
    float NormalizeValue(float newValue) const;      // 範囲と刻み幅に合わせた値にする
    bool IsPointOnSlider(DirectX::XMFLOAT2 point) const;   // 画面座標の点が、溝かつまみの上か
    void ApplyMouse(float mouseX);                   // マウスのx(画面座標)に合わせて値を変える
    void EndDrag();                                  // つまみを離す

public:
    // ─── 値 ─────────────
    void SetRange(float minimum, float maximum);   // 値の範囲(minimumがmaximumより大きければ入れ替える)。範囲の外の今の値は収める
    void SetValue(float newValue);         // 値を設定(範囲と刻み幅に合わせる。NaNなどは無視。onChangedは呼ばない)
    float GetValue() const { return value; }   // 今の値
    void SetStep(float stepSize);          // 刻み幅(0=刻まない。負の値・NaNは0)。今の値も合わせ直す
    void SetOnChanged(std::function<void(float)> callback) { onChanged = std::move(callback); }   // 値が変わった時に呼ぶ
    void SetOnDragEnd(std::function<void(float)> callback) { onDragEnd = std::move(callback); }   // つまみを離した時に呼ぶ(値が変わっていなくても1回)

    // ─── 見た目・位置 ─────────────
    void SetAnchor(Anchor anchorPoint, DirectX::XMFLOAT2 anchorOffset);   // 画面のどこに置くか
    void SetSize(float width, float height);                                // 溝の幅と高さ(ピクセル。0以下は無視)
    void SetColors(DirectX::XMFLOAT4 track, DirectX::XMFLOAT4 fill, DirectX::XMFLOAT4 handle);   // 溝・左側・つまみの色
    void SetLabel(const std::wstring& text, const TextStyle& style);        // スライダーの上に出す文字

    // ─── 操作の状態 ─────────────
    bool IsDragging() const { return dragging; }   // つまみをつかんでいるか
    static bool IsAnyDragging();                   // どれかのスライダーのつまみをつかんでいるか(ButtonComponentが、つかんでいる間は反応しないために使う)

    void Update(float dt) override;
    void Draw() override;
    void OnActiveChanged(bool active) override;
    void Uninit() override;
};
