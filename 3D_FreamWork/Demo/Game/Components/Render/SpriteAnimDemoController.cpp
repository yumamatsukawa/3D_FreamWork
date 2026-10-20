#include "SpriteAnimDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../Common/DemoMarker.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Animation/SpriteAnimationComponent.h"

namespace {
    // 再生の速さ(1〜4キーの順)
    constexpr float speeds[4] = { 0.5f, 1.f, 2.f, 0.f };
    const wchar_t* const speedNames[4] = { L"0.5倍", L"通常(1倍)", L"2倍", L"止める(0)" };

    // 文字の位置
    constexpr float listX = -610.f;       // 一覧の左端
    constexpr float listY = 192.f;        // 一覧の1行目(説明の板の下)
    constexpr float listGap = 30.f;       // 行の間隔
}

void SpriteAnimDemoController::AddEntry(GameObject* object, SpriteAnimationComponent* anim, const std::string& clip, bool once,
    const std::wstring& label, float labelOffset) {
    entries.push_back({ object, anim, clip, once, label, labelOffset });
}

void SpriteAnimDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // SPACE: 再生 ⇔ 停止(Stopしてからのは、1回だけのクリップも最初のコマから)
    if (Input::GetKeyDown(KEY_SPACE)) {
        playing = !playing;
        for (Entry& entry : entries) {
            if (playing) entry.anim->Play(entry.clip);
            else entry.anim->Stop();
        }
    }

    // 1〜4: 再生の速さ(全員に同じ速さを渡す)
    for (int i = 0; i < 4; i++) {
        if (Input::GetKeyDown(KEY_1 + i)) {
            speedIndex = i;
            for (Entry& entry : entries) entry.anim->SetSpeed(speeds[i]);
        }
    }
}

void SpriteAnimDemoController::Draw() {
    // 再生の状態(SPACE)
    Text::Draw(std::wstring(L"[SPACE] ") + (playing ? L"再生中(Play)  押すと Stop" : L"停止中(Stop)  押すと Play(最初のコマから)"),
        listX, listY, MakeDemoTextStyle(21.f, playing ? DirectX::XMFLOAT4(0.6f, 1.0f, 0.6f, 1.0f) : DirectX::XMFLOAT4(1.0f, 0.6f, 0.5f, 1.0f)));

    // 再生の速さ(1〜4)。今の速さは黄色
    float x = listX;
    for (int i = 0; i < 4; i++) {
        const std::wstring text = L"[" + std::to_wstring(i + 1) + L"] " + speedNames[i];
        const TextStyle style = MakeDemoTextStyle(20.f, (i == speedIndex) ? DirectX::XMFLOAT4(1.0f, 0.9f, 0.4f, 1.0f) : DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
        Text::Draw(text, x, listY - listGap, style);
        x += Text::Measure(text, style).x + 30.f;
    }
    Text::Draw(L"SetSpeed(速さ)    SetRandomStart(true): 始まるコマをずらす(下の段)", listX, listY - listGap * 2.f,
        MakeDemoTextStyle(18.f, { 0.8f, 0.9f, 1.0f, 1.0f }));

    // スプライトごとの名前(1回だけのクリップは、IsFinished の結果も)
    for (const Entry& entry : entries) {
        const DirectX::XMFLOAT3 position = entry.object->transform.position;
        if (!entry.label.empty()) DrawDemoLabel(mode, position, entry.label, entry.labelOffset);
        if (entry.once) {
            DrawDemoLabel(mode, position, entry.anim->IsFinished() ? L"IsFinished = true" : L"IsFinished = false", entry.labelOffset - 22.f);
        }
    }
}
