#pragma once
#include "../Core/Component.h"
#include <string>
#include <functional>

// クリックできるボタン(見た目の変化・クリック判定・文字の表示)を担当するComponent。
// 見た目の画像は、同じオブジェクトに付けた SpriteRenderer を使う(先に付けておく)。
//
// 使い方:
//   GameObject* button = scene.CreateObject("Button");
//   button->transform.position = { 0.f, -150.f, 0.f };
//   button->transform.scale = { 320.f, 96.f, 1.f };                  // ボタンの大きさ(クリック判定もこの大きさ)
//   button->AddComponent<SpriteRenderer>(L"Assets/button.png");
//   button->AddComponent<ButtonComponent>(L"START", 48.f, [] { ... });   // 文字・文字の大きさ・押された時の処理
//
//   // あとから変える時
//   auto* component = button->GetComponent<ButtonComponent>();
//   component->SetOnClick([] { ... });
//   component->SetInteractable(false);    // 押せなくして、灰色にする
//
// 決まり:
// - クリック判定はワールド座標で行う(Input::GetMouseWorldPosition と transform の位置・大きさ・回転。2Dカメラのスクロールも考慮される)
// - 押す→離すで onClick が呼ばれる。ボタンの上で押して、ボタンの上で離した時だけ(途中でボタンの外に出たらキャンセル)
// - 見た目: 押している間は暗く、マウスが乗っている間は明るく、それ以外は少し暗めにする(SpriteRendererの色を変える)
// - 文字は画面座標に直して重ねる。文字の大きさは、作った時(Init)のボタンの高さに対する今の高さの割合で変わるので、
//   TweenComponent でボタンの大きさを変えても、文字が画像と一緒に大きくなる。ほぼ大きさ0の間は描かない
// - ボタンの大きさ(transform.scale)は AddComponent する前に決めておく(文字の大きさの基準になるため)
// - SetInteractable(false) の間は、クリックを受け付けず、画像も文字も灰色になる(押している途中だったらキャンセル)
// - スライダー(SliderComponent)のつまみをつかんでいる間は、マウスが乗っても反応しない
class ButtonComponent : public Component {
private:
    std::wstring label;               // ボタンに表示する文字
    float textSize;                   // 文字の大きさ
    std::function<void()> onClick;    // クリックされた時の処理
    bool interactable = true;         // 押せるか
    bool pressed = false;             // ボタンの上で押し始めたか
    float baseHeight = 0.f;           // 作った時のボタンの高さ(文字の大きさをボタンの大きさに合わせる基準)

public:
    ButtonComponent(const std::wstring& label = L"", float textSize = 48.f, std::function<void()> onClick = nullptr)
        : label(label), textSize(textSize), onClick(std::move(onClick)) {}

    void SetOnClick(std::function<void()> callback) { onClick = std::move(callback); }   // クリックされた時に呼ぶ
    void SetLabel(const std::wstring& text) { label = text; }                            // ボタンに表示する文字
    void SetInteractable(bool enabled);                                                  // falseで押せなくする(灰色にする)
    bool IsInteractable() const { return interactable; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
