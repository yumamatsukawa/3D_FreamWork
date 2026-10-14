#include "ButtonController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/Image.h"
#include "../../../Engine/Text.h"
#include "../../../Engine/Collider.h"

void ButtonController::Init() {
    // 作った時の高さを、文字の大きさの基準にする
    baseHeight = GetOwner()->transform.scale.y;
}

void ButtonController::Update(float dt) {
    // マウスがボタンの上にあるか
    DirectX::XMFLOAT2 mouseWorldPos = Input::GetMouseWorldPosition();
    bool hover = IsPointInBox(mouseWorldPos, GetOwner()->transform.GetWorldTransform());

    // ボタンの上で押し始めた
    if (hover && Input::GetKeyDown(MOUSE_LEFT)) pressed = true;

    // 見た目: 押している間は暗く、マウスが乗っている間は明るく、それ以外は少し暗めにする
    if (SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>()) {
        if (pressed && hover) sprite->SetColor(0.6f, 0.6f, 0.6f, 1.f);
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

void ButtonController::Draw() {
    // ボタンの文字(Text::Drawは画面中央基準なので、カメラ位置を引いてボタンの画像に重ねる)
    const Camera& camera = Image::GetCamera();
    const Transform t = GetOwner()->transform.GetWorldTransform();

    // 文字の大きさはボタンの高さに合わせる(TweenComponentで大きさが変わる途中も、画像と一緒に大きくなる)
    float textScale = (baseHeight != 0.f) ? GetOwner()->transform.scale.y / baseHeight : 1.f;
    if (textScale <= 0.01f) return;   // ほぼ大きさ0の時は描かない
    Text::Draw(label, t.position.x - camera.position.x, t.position.y - camera.position.y, textSize * textScale);
}
