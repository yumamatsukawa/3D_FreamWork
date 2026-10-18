#include "ButtonComponent.h"
#include "../Core/GameObject.h"
#include "../Render/SpriteRenderer.h"
#include "SliderComponent.h"
#include "../Core/Input.h"
#include "../Render/Image.h"
#include "Text.h"
#include "../Physics/Collider.h"

namespace {
    // 押せない時の灰色
    constexpr float disabledSpriteShade = 0.4f;                                // 画像の明るさ
    constexpr DirectX::XMFLOAT4 disabledTextColor = { 0.6f, 0.6f, 0.6f, 1.f }; // 文字の色
}

void ButtonComponent::SetInteractable(bool enabled) {
    interactable = enabled;
    if (!interactable) pressed = false;   // 押している途中だったらキャンセル
}

void ButtonComponent::Init() {
    // 作った時の高さを、文字の大きさの基準にする
    baseHeight = GetOwner()->transform.scale.y;
}

void ButtonComponent::Update(float dt) {
    // マウスがボタンの上にあるか(押せない時と、スライダーのつまみをつかんでいる間は、乗っていないことにする)
    DirectX::XMFLOAT2 mouseWorldPos = Input::GetMouseWorldPosition();
    bool hover = interactable && !SliderComponent::IsAnyDragging()
        && IsPointInBox(mouseWorldPos, GetOwner()->transform.GetWorldTransform());

    // ボタンの上で押し始めた
    if (hover && Input::GetKeyDown(MOUSE_LEFT)) pressed = true;

    // 見た目: 押せない時は灰色、押している間は暗く、マウスが乗っている間は明るく、それ以外は少し暗めにする
    if (SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>()) {
        if (!interactable)    sprite->SetColor(disabledSpriteShade, disabledSpriteShade, disabledSpriteShade, 1.f);
        else if (pressed && hover) sprite->SetColor(0.6f, 0.6f, 0.6f, 1.f);
        else if (hover)       sprite->SetColor(1.0f, 1.0f, 1.0f, 1.f);
        else                  sprite->SetColor(0.8f, 0.8f, 0.8f, 1.f);
    }

    // クリック: ボタンの上で押して、ボタンの上で離した時だけ(途中で外に出たらキャンセル)
    if (Input::GetKeyUp(MOUSE_LEFT)) {
        bool clicked = pressed && hover;
        pressed = false;
        if (clicked && onClick) onClick();
    }
}

void ButtonComponent::Draw() {
    // ボタンの文字(Text::Drawは画面座標なので、ボタンのワールド座標を画面座標に直して画像に重ねる)
    const Transform t = GetOwner()->transform.GetWorldTransform();
    DirectX::XMFLOAT2 screenPos = Image::WorldToScreen2D(t.position);

    // 文字の大きさはボタンの高さに合わせる(TweenComponentで大きさが変わる途中も、画像と一緒に大きくなる)
    float textScale = (baseHeight != 0.f) ? GetOwner()->transform.scale.y / baseHeight : 1.f;
    if (textScale <= 0.01f) return;   // ほぼ大きさ0の時は描かない
    Text::Draw(label, screenPos.x, screenPos.y, textSize * textScale,
        interactable ? DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f) : disabledTextColor);
}
