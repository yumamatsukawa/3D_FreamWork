#include "PoolDemoController.h"
#include "PoolBulletController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include <cstdio>

namespace {
    constexpr float infoX = -610.f;       // 数の文字の左端
    constexpr float infoY = 190.f;        // 数の1行目(説明の板の下)
    constexpr float infoGap = 30.f;       // 行の間隔
    constexpr float cellLeft = 60.f;      // 弾の一覧(1つ1つが四角)の左端
    constexpr float cellTop = 150.f;      // 弾の一覧の1行目
    constexpr float cellGap = 20.f;       // 四角の間隔
    constexpr int cellColumns = 28;       // 1行に並べる四角の数
}

void PoolDemoController::SetupBullet(GameObject* bullet) {
    // 弾の見た目と動き(プールに新しく作る時だけ呼ばれる。返す処理はプールの持ち主=このComponent)
    bullet->transform.scale = { 26.f, 26.f, 1.f };
    bullet->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
    bullet->AddComponent<PoolBulletController>([this](GameObject* expired) { bulletPool.Return(expired); });
}

void PoolDemoController::Init() {
    // 弾をあらかじめ用意しておく(非表示。足りなくなったらRentが自動で足す)
    bulletPool.Prewarm(*GetOwner()->GetScene(), prewarmCount, [this](GameObject* bullet) { SetupBullet(bullet); });
}

void PoolDemoController::Fire() {
    // 弾を借りる(待機中の弾があればそれを使い、無い時だけ新しく作る)
    GameObject* bullet = bulletPool.Rent(*GetOwner()->GetScene(), [this](GameObject* b) { SetupBullet(b); });
    rentCount++;

    // 借りた弾は前回の値が残っているので、見た目も毎回決め直す
    auto* sprite = bullet->GetComponent<SpriteRenderer>();
    sprite->SetSpriteIndex(Math::RandomRange(0, 8));

    // マウスの方向へ飛ばす(マウスが出す場所と重なっている時は真上)
    const DirectX::XMFLOAT2 mouse = Input::GetMouseWorldPosition();
    DirectX::XMFLOAT2 direction = Math::Normalize(mouse - DirectX::XMFLOAT2{ muzzle.x, muzzle.y });
    if (Math::Length(direction) < 0.5f) direction = { 0.f, 1.f };
    bullet->GetComponent<PoolBulletController>()->Launch(muzzle, direction * bulletSpeed, bulletLife);
}

int PoolDemoController::CountActive() const {
    int count = 0;
    for (const GameObject* bullet : bulletPool.GetObjects()) {
        if (bullet->GetIsActive()) count++;
    }
    return count;
}

void PoolDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 左クリックかSPACEを押している間、一定の間隔で弾を出す
    cooldown -= dt;
    const bool firing = Input::GetKeyPress(MOUSE_LEFT) || Input::GetKeyPress(KEY_SPACE);
    if (firing && cooldown <= 0.f) {
        Fire();
        cooldown = fireInterval;
    }
}

void PoolDemoController::Draw() {
    // 数の表示: 作った数(GetPoolSize)は、使用中が増えて足りなくなった時だけ増える
    const int active = CountActive();
    const int poolSize = bulletPool.GetPoolSize();
    wchar_t buf[96];
    float y = infoY;
    swprintf_s(buf, L"Prewarm(最初に用意)  %d 個", prewarmCount);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(22.f)); y -= infoGap;
    swprintf_s(buf, L"Rent した回数  %d 回", rentCount);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(22.f, { 1.0f, 0.9f, 0.4f, 1.0f })); y -= infoGap;
    swprintf_s(buf, L"使用中(出ている弾)  %d 個", active);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(22.f, { 0.7f, 1.0f, 0.7f, 1.0f })); y -= infoGap;
    swprintf_s(buf, L"待機中(返された弾)  %d 個", poolSize - active);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(22.f, { 0.8f, 0.8f, 0.8f, 1.0f })); y -= infoGap;
    swprintf_s(buf, L"作った数 GetPoolSize()  %d 個", poolSize);
    Text::Draw(buf, infoX, y, MakeDemoTextStyle(22.f, { 0.6f, 0.9f, 1.0f, 1.0f })); y -= infoGap * 1.3f;
    Text::Draw(L"何回出しても、作った数は増え続けません", infoX, y, MakeDemoTextStyle(19.f, { 0.8f, 0.8f, 0.8f, 1.0f }));

    // 弾の一覧: 1つ1つが四角で、緑=使用中、灰色=待機中
    Text::Draw(L"プールの中身(緑: 使用中 / 灰色: 待機中)", cellLeft, cellTop + 34.f, MakeDemoTextStyle(19.f));
    const std::vector<GameObject*>& bullets = bulletPool.GetObjects();
    for (size_t i = 0; i < bullets.size(); i++) {
        const float x = cellLeft + cellGap * (float)(i % cellColumns) + 8.f;
        const float cellY = cellTop - cellGap * (float)(i / cellColumns);
        const DirectX::XMFLOAT4 color = bullets[i]->GetIsActive() ? DirectX::XMFLOAT4{ 0.4f, 1.0f, 0.4f, 1.0f } : DirectX::XMFLOAT4{ 0.6f, 0.6f, 0.6f, 0.8f };
        Image::DrawRect({ x, cellY }, { 14.f, 14.f }, color);
    }
}
