#include "SkinAnimDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../Objects/Render3D/SkinAnimDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/ModelRenderer.h"
#include "../../../../Engine/Animation/ModelAnimationComponent.h"
#include "../../../../Engine/UI/Text.h"
#include <cstdio>
#include <string>
#include <vector>

namespace {
    constexpr float fadeSeconds = 0.6f;      // CrossFadeにかける秒数
    constexpr float speedStep = 0.25f;       // 速さを1回で変える量
    constexpr float maxSpeed = 3.f;          // 速さの上限
    constexpr float listX = -610.f;          // 状態の一覧の左端
    constexpr float listY = 192.f;           // 一覧の1行目(説明の板の下)
    constexpr float listGap = 25.f;          // 行の間隔
    const DirectX::XMFLOAT4 outlineColor = { 0.3f, 1.0f, 0.5f, 1.0f };   // 枠線の色(緑)

    const DirectX::XMFLOAT4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
    const DirectX::XMFLOAT4 yellow = { 1.0f, 0.9f, 0.4f, 1.0f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.0f, 1.0f };

    // 英字だけの名前(クリップの名前)を、文字を描く関数に渡せる形にする
    std::wstring ToWide(const std::string& text) {
        return std::wstring(text.begin(), text.end());
    }

    // クリップの名前を「Spin, Bob」のように並べる
    std::wstring JoinClipNames(GameObject* object) {
        if (!object) return L"-";
        const ModelAnimationComponent* animation = object->GetComponent<ModelAnimationComponent>();
        if (!animation) return L"-";
        std::wstring text;
        for (const std::string& name : animation->GetClipNames()) {
            if (!text.empty()) text += L", ";
            text += ToWide(name);
        }
        return text.empty() ? L"(なし)" : text;
    }

    // 物の上に名前を出す
    void DrawLabel(GameObject* object, const wchar_t* label, float height) {
        if (!object) return;
        DirectX::XMFLOAT3 head = object->transform.position;
        head.y += height;
        const DirectX::XMFLOAT3 screen = Image::WorldToScreen3D(head);
        if (screen.z <= 0.f) return;
        Text::Draw(label, screen.x, screen.y, MakeDemoTextStyle(18.f, yellow, TextAlign::Center));
    }

    // 枠線を付ける/消す(骨で曲げたあとの形に枠線が付く)
    void SetModelOutline(GameObject* object, bool on) {
        if (!object) return;
        if (ModelRenderer* renderer = object->GetComponent<ModelRenderer>()) {
            renderer->SetOutline(outlineColor, on ? 3.f : 0.f);
        }
    }
}

ModelAnimationComponent* SkinAnimDemoController::GetOnceAnimation() const {
    return onceArm ? onceArm->GetComponent<ModelAnimationComponent>() : nullptr;
}

void SkinAnimDemoController::ApplySpeed() {
    // 速さ(0で止まる)を、3つのアニメーションに入れる
    if (boxAnimation) boxAnimation->SetSpeed(speed);
    if (loopArm) {
        if (ModelAnimationComponent* animation = loopArm->GetComponent<ModelAnimationComponent>()) animation->SetSpeed(speed);
    }
    if (ModelAnimationComponent* animation = GetOnceAnimation()) animation->SetSpeed(speed);
}

void SkinAnimDemoController::ApplyOutline() {
    SetModelOutline(box, outlined);
    SetModelOutline(loopArm, outlined);
    SetModelOutline(onceArm, outlined);
}

void SkinAnimDemoController::Update(float dt) {
    (void)dt;
    if (!boxAnimation) return;

    // 1・2キー: 箱のクリップを、すぐ切り替える(Play)
    if (Input::GetKeyDown(KEY_1)) { boxClip = "Spin"; boxAnimation->Play(boxClip); }
    if (Input::GetKeyDown(KEY_2)) { boxClip = "Bob"; boxAnimation->Play(boxClip); }

    // SPACEキー: もう1つのクリップへ、なめらかに切り替える(CrossFade)
    if (Input::GetKeyDown(KEY_SPACE)) {
        boxClip = (boxClip == "Spin") ? "Bob" : "Spin";
        boxAnimation->CrossFade(boxClip, fadeSeconds);
    }

    // 3キー: 1回だけ曲がる腕を作り直して、最初から再生する(古い腕は消す)
    if (Input::GetKeyDown(KEY_3) && GetOwner()->GetScene()) {
        if (onceArm) onceArm->Destroy();
        onceArm = CreateSkinArm(*GetOwner()->GetScene(), onceArmX, false);
        ApplySpeed();
        ApplyOutline();
    }

    // 4キー: 枠線(骨で曲げた形にも付く)
    if (Input::GetKeyDown(KEY_4)) {
        outlined = !outlined;
        ApplyOutline();
    }

    // 上下キー: 再生の速さ
    if (Input::GetKeyDown(KEY_UP)) { speed += speedStep; ApplySpeed(); }
    if (Input::GetKeyDown(KEY_DOWN)) { speed -= speedStep; ApplySpeed(); }
    speed = Math::Clamp(speed, 0.f, maxSpeed);
}

void SkinAnimDemoController::Draw() {
    // 物の上の名前
    DrawLabel(box, L"箱(骨なし)", 200.f);
    DrawLabel(loopArm, L"腕(骨で曲がる・ずっと)", 290.f);
    DrawLabel(onceArm, L"腕(1回だけ)", 290.f);

    wchar_t buf[160];
    float y = listY;
    auto line = [&](const std::wstring& text, const DirectX::XMFLOAT4& color) {
        Text::Draw(text, listX, y, MakeDemoTextStyle(18.f, color));
        y -= listGap;
    };

    // クリップの一覧(GetClipNames)と、今のクリップ
    line(L"箱 TestAnim.fbx のクリップ: " + JoinClipNames(box) + L"   今: " + ToWide(boxClip), cyan);
    line(L"腕 TestSkin.fbx のクリップ: " + JoinClipNames(loopArm) + L"(骨2本で曲がる)", cyan);

    // 1回だけの腕の状態(ループしないクリップは、終わると最後の形で止まる)
    const ModelAnimationComponent* once = GetOnceAnimation();
    line(std::wstring(L"1回だけの腕 IsFinished(): ") + (once ? (once->IsFinished() ? L"終わった" : L"再生中") : L"-"), white);
    y -= 6.f;

    // 操作の状態
    line(L"[1] 箱を Spin に切り替える(Play: すぐ)", white);
    line(L"[2] 箱を Bob に切り替える(Play: すぐ)", white);
    line(L"[SPACE] 箱をもう一方へ CrossFade(なめらか。0.6秒)", white);
    line(L"[3] 1回だけの腕を、最初から再生し直す", white);
    line(std::wstring(L"[4] 枠線: ") + (outlined ? L"ON(骨で曲げた形に付く)" : L"OFF"), white);
    swprintf_s(buf, L"[上下] 再生の速さ SetSpeed: %.2f倍", speed);
    line(buf, yellow);
}
