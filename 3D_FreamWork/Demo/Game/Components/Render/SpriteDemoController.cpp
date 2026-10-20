#include "SpriteDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../Common/DemoMarker.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include <cmath>
#include <cstdio>

namespace {
    constexpr float frameSeconds = 0.5f;      // コマを1つ進める時間(秒)
    constexpr int frameCount = 9;             // box.png のコマの数(3x3)
    constexpr float colorSpeed = 0.5f;        // 色が1周する速さ(1秒あたりの周回数)
    constexpr float sizeAmount = 0.4f;        // 大きさが変わる幅(元の大きさの割合)
    constexpr float sizeSpeed = 3.f;          // 大きさが変わる速さ
    constexpr float rotateSpeed = 90.f;       // 回る速さ(度 / 秒)

    // 説明の位置
    constexpr float listX = -610.f;           // 一覧の左端
    constexpr float listY = 192.f;            // 一覧の1行目(説明の板の下)
    constexpr float listGap = 28.f;           // 行の間隔
    constexpr float noteY = -140.f;           // 2D: 重ねた2つの説明の1行目
    constexpr float noteGap = 28.f;           // 2D: 行の間隔
    constexpr float label2D = -95.f;          // 2D: スプライトの名前(スプライトの中心から下へ)
    constexpr float label3D = 80.f;           // 3D: スプライトの名前(スプライトの中心から上へ)

    // 0〜1 の値から、虹色の1周を作る
    DirectX::XMFLOAT3 GetRainbow(float t) {
        const float angle = t * 6.2831853f;
        return { 0.5f + 0.5f * sinf(angle), 0.5f + 0.5f * sinf(angle + 2.094f), 0.5f + 0.5f * sinf(angle + 4.189f) };
    }

    // 動きの名前・使っているAPI(1〜4キーの順)
    const wchar_t* const effectNames[4] = { L"コマ", L"色", L"大きさ", L"回転" };
    const wchar_t* const effectApis[4] = { L"SetSpriteIndex(コマ番号)", L"SetColor(r, g, b, a)", L"transform.scale", L"transform.rotate.z" };
}

void SpriteDemoController::AddSprite(GameObject* object, SpriteRenderer* sprite, int effects, const std::wstring& label) {
    items.push_back({ object, sprite, effects, object->transform.scale, label });
}

void SpriteDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1〜4: 動きの入り切り
    for (int i = 0; i < effectCount; i++) {
        if (Input::GetKeyDown(KEY_1 + i)) enabled[i] = !enabled[i];
        if (enabled[i]) timers[i] += dt;
    }

    // 2D: 5キーで、重ねた2つの描き方をUI → 2D → 3Dの順に切り替える
    if (mode == DemoMode::Mode2D && Input::GetKeyDown(KEY_5) && nearSprite && farSprite) {
        pairMode = (pairMode == SpriteMode::UI) ? SpriteMode::World2D : (pairMode == SpriteMode::World2D) ? SpriteMode::World3D : SpriteMode::UI;
        nearSprite->SetMode(pairMode);
        farSprite->SetMode(pairMode);
    }

    // 動きを、受けるスプライトに反映する(動きが切ってある間は、その時の見た目のまま止まる)
    const int frame = (int)(timers[0] / frameSeconds) % frameCount;
    const DirectX::XMFLOAT3 color = GetRainbow(timers[1] * colorSpeed);
    const float scale = 1.f + sizeAmount * sinf(timers[2] * sizeSpeed);
    for (Item& item : items) {
        if (item.effects & Frame) item.sprite->SetSpriteIndex(frame);
        if (item.effects & Color) item.sprite->SetColor(color.x, color.y, color.z, 1.f);
        if (item.effects & Size) item.object->transform.scale = { item.baseScale.x * scale, item.baseScale.y * scale, item.baseScale.z };
        if (item.effects & Rotate) item.object->transform.rotate.z = timers[3] * rotateSpeed;
    }
}

void SpriteDemoController::Draw() {
    // 1〜4の一覧(入っている動きは緑、切ってある動きは灰色)
    for (int i = 0; i < effectCount; i++) {
        const bool on = enabled[i];
        wchar_t buf[96];
        swprintf_s(buf, L"[%d] %ls: %ls", i + 1, effectNames[i], on ? L"ON " : L"OFF");
        Text::Draw(buf, listX, listY - listGap * (float)i, MakeDemoTextStyle(21.f, on ? DirectX::XMFLOAT4(0.6f, 1.0f, 0.6f, 1.0f) : DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f)));
        Text::Draw(effectApis[i], listX + 220.f, listY - listGap * (float)i, MakeDemoTextStyle(18.f, { 0.8f, 0.9f, 1.0f, 1.0f }));
    }

    // スプライトの名前
    for (const Item& item : items) {
        const float offset = (mode == DemoMode::Mode3D) ? label3D : label2D;
        DrawDemoLabel(mode, item.object->transform.position, item.label, offset);
    }

    if (mode == DemoMode::Mode2D) {
        // 重ねた2つの説明
        const wchar_t* modeName = (pairMode == SpriteMode::UI) ? L"UI(SpriteMode::UI)" : (pairMode == SpriteMode::World2D) ? L"2D(SpriteMode::World2D)" : L"3D(SpriteMode::World3D)";
        const DirectX::XMFLOAT4 gray = { 0.8f, 0.8f, 0.8f, 1.0f };
        Text::Draw(std::wstring(L"[5] 重ねた2つ: ") + modeName, listX, noteY, MakeDemoTextStyle(21.f, { 1.0f, 0.9f, 0.4f, 1.0f }));
        Text::Draw(L"赤は手前(z=-100)、青は奥(z=+100)、緑は常にUI(z=+500)", listX, noteY - noteGap, MakeDemoTextStyle(19.f));
        if (pairMode == SpriteMode::UI) {
            Text::Draw(L"UI: 前後は「作った順」(後に作った青が手前。zは使わない)", listX, noteY - noteGap * 2.f, MakeDemoTextStyle(19.f, gray));
            Text::Draw(L"大きさはzで変わらない。必ず最前面(2D・3Dの物より手前)", listX, noteY - noteGap * 3.f, MakeDemoTextStyle(19.f, gray));
        }
        else if (pairMode == SpriteMode::World2D) {
            Text::Draw(L"2D: zが小さい(手前)ほど手前(赤が手前)。深度で決まる", listX, noteY - noteGap * 2.f, MakeDemoTextStyle(19.f, gray));
            Text::Draw(L"大きさはzで変わらない(遠近なし)。UIの緑には隠れる", listX, noteY - noteGap * 3.f, MakeDemoTextStyle(19.f, gray));
        }
        else {
            Text::Draw(L"3D: 3Dカメラの遠近で、手前の赤は大きく、奥の青は小さい", listX, noteY - noteGap * 2.f, MakeDemoTextStyle(19.f, gray));
            Text::Draw(L"前後は深度で決まる(赤が手前)。UIの緑には隠れる", listX, noteY - noteGap * 3.f, MakeDemoTextStyle(19.f, gray));
        }
    }
    else {
        Text::Draw(L"右ドラッグで視点を回すと、ビルボードの向きの違いが分かります(Q・E でズーム)", listX, noteY - 60.f, MakeDemoTextStyle(19.f));
        Text::Draw(L"手前のスプライトは立方体を隠し、奥のスプライトは立方体に隠れます(深度で前後が決まる)", listX, noteY - noteGap - 60.f, MakeDemoTextStyle(19.f, { 0.8f, 0.8f, 0.8f, 1.0f }));
    }
}
