#include "WorldUiDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/UI/TextRenderer.h"

namespace {
    constexpr float infoX = -610.f;      // 説明の文字の左端
    constexpr float infoY = 190.f;       // 説明の1行目(説明の板の下)
    constexpr float moveLimitX = 560.f;  // 主人公が動ける範囲
    constexpr float limit2DMinY = -200.f;   // 2D: 主人公が動ける下の限界
    constexpr float limit2DMaxY = 120.f;    // 2D: 主人公が動ける上の限界
    constexpr float limit3DMinZ = 100.f;    // 3D: 主人公が動ける手前の限界
    constexpr float limit3DMaxZ = 550.f;    // 3D: 主人公が動ける奥の限界
}

int WorldUiDemoController::AddEnemy(GameObject* object, const wchar_t* name, float maxHp) {
    enemies.push_back({ object, name, maxHp, maxHp });
    return (int)enemies.size() - 1;
}

void WorldUiDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 矢印キー: 主人公を動かす(2DはXY、3DはXZ)
    float moveX = 0.f, moveAxis = 0.f;
    if (Input::GetKeyPress(KEY_RIGHT)) moveX += 1.f;
    if (Input::GetKeyPress(KEY_LEFT)) moveX -= 1.f;
    if (Input::GetKeyPress(KEY_UP)) moveAxis += 1.f;
    if (Input::GetKeyPress(KEY_DOWN)) moveAxis -= 1.f;
    Transform& t = hero->transform;
    t.position.x = Math::Clamp(t.position.x + moveX * heroSpeed * dt, -moveLimitX, moveLimitX);
    if (mode == DemoMode::Mode3D) t.position.z = Math::Clamp(t.position.z + moveAxis * heroSpeed * dt, limit3DMinZ, limit3DMaxZ);
    else t.position.y = Math::Clamp(t.position.y + moveAxis * heroSpeed * dt, limit2DMinY, limit2DMaxY);

    // 1〜3: 敵にダメージ。HPが0になった敵は、非表示にする(頭上のゲージと名前も一緒に消える)
    for (size_t i = 0; i < enemies.size(); i++) {
        Enemy& enemy = enemies[i];
        if (!Input::GetKeyDown(KEY_1 + (int)i) || !enemy.object->GetIsActive()) continue;
        enemy.hp = Math::Clamp(enemy.hp - damageAmount, 0.f, enemy.maxHp);
        if (enemy.hp <= 0.f) enemy.object->SetActive(false);
    }

    // M: 主人公の名前の大きさの決め方を切り替える(Screen: 変わらない ⇄ World: 奥行きで変わる)
    if (Input::GetKeyDown(KEY_M)) {
        heroText->SetSizeMode(heroText->GetSizeMode() == TextSizeMode::Screen ? TextSizeMode::World : TextSizeMode::Screen);
    }

    // SPACE: 全員を全回復して、倒れた敵も戻す
    if (Input::GetKeyDown(KEY_SPACE)) {
        for (Enemy& enemy : enemies) {
            enemy.hp = enemy.maxHp;
            enemy.object->SetActive(true);
        }
    }
}

void WorldUiDemoController::Draw() {
    // 説明
    Text::Draw(L"主人公が敵に近づくと、名前の板が赤くなります", infoX, infoY, MakeDemoTextStyle(20.f));
    Text::Draw(L"HPゲージ: GaugeRenderer(SetFollowTarget)   名前: 敵の頭上の位置を画面の位置に直して描く", infoX, infoY - 28.f,
        MakeDemoTextStyle(18.f, { 0.8f, 0.8f, 0.8f, 1.0f }));

    // 主人公の名前(TextRenderer)の大きさの決め方。3Dだけ、World にすると奥行きで大きさが変わる
    const bool world = heroText->GetSizeMode() == TextSizeMode::World;
    const wchar_t* modeLabel = world
        ? (mode == DemoMode::Mode3D ? L"World(奥行きで大きさが変わる。遠いほど小さい)" : L"World(2Dカメラには奥行きが無いので、大きさは変わらない)")
        : L"Screen(大きさは変わらない)";
    Text::Draw(std::wstring(L"M: 主人公の名前の大きさ = ") + modeLabel, infoX, infoY - 56.f,
        MakeDemoTextStyle(18.f, { 1.0f, 0.9f, 0.4f, 1.0f }));

    // 敵ごとの名前の板(倒れて非表示の敵には出さない)
    for (const Enemy& enemy : enemies) {
        if (!enemy.object->GetIsActive()) continue;

        // 敵の頭の上のワールド座標 → 画面座標(3Dはカメラの後ろにある時は出さない)
        DirectX::XMFLOAT3 world = enemy.object->transform.position;
        world.y += nameOffsetY;
        DirectX::XMFLOAT2 screen;
        if (mode == DemoMode::Mode3D) {
            const DirectX::XMFLOAT3 projected = Image::WorldToScreen3D(world);
            if (projected.z <= 0.f) continue;
            screen = { projected.x, projected.y };
        }
        else {
            screen = Image::WorldToScreen2D(world);
        }

        // 主人公が近いか(近い時は板を赤くして、名前に ! を付ける)
        const bool alert = Math::Distance(hero->transform.position, enemy.object->transform.position) < alertRange;
        const std::wstring label = enemy.name + L"  " + std::to_wstring((int)enemy.hp) + L"/" + std::to_wstring((int)enemy.maxHp) + (alert ? L" !" : L"");
        const TextStyle style = MakeDemoTextStyle(19.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center);
        const DirectX::XMFLOAT2 size = Text::Measure(label, style);

        // 板(文字より先に描く)→ 文字
        Image::DrawRect(screen, { size.x + 18.f, size.y + 8.f }, alert ? DirectX::XMFLOAT4{ 0.8f, 0.15f, 0.15f, 0.8f } : DirectX::XMFLOAT4{ 0.0f, 0.0f, 0.0f, 0.6f });
        Text::Draw(label, screen.x, screen.y, style);
    }
}
