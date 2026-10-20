#include "CoordDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../Common/DemoMarker.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include <cstdio>

namespace {
    using DirectX::XMFLOAT2;
    using DirectX::XMFLOAT3;
    using DirectX::XMFLOAT4;

    constexpr float cameraSpeed = 450.f;       // 2D: カメラを動かす速さ(1秒あたり)
    constexpr float cameraLimit = 1500.f;      // 2D: カメラが動ける範囲(中心から)
    constexpr float rayLength = 10000.f;       // 3D: マウスの線の長さ

    // 説明の位置
    constexpr float listX = -610.f;            // 一覧の左端
    constexpr float listY = 192.f;             // 一覧の1行目(説明の板の下)
    constexpr float listGap = 26.f;            // 行の間隔

    const XMFLOAT4 yellow = { 1.0f, 0.9f, 0.4f, 1.0f };
    const XMFLOAT4 cyan = { 0.4f, 0.9f, 1.0f, 1.0f };
    const XMFLOAT4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
    const XMFLOAT4 gray = { 0.8f, 0.8f, 0.8f, 1.0f };

    // 英字だけの名前(GameObjectの名前)を、文字を描く関数に渡せる形にする
    std::wstring ToWide(const std::string& text) {
        return std::wstring(text.begin(), text.end());
    }

    // マウスの位置(画面の座標。中心が(0,0)、Y+=上。Input::GetMousePositionはY+=下なので、Yの符号を反転する)
    XMFLOAT2 GetMouseScreen() {
        const XMFLOAT2 mouse = Input::GetMousePosition();
        return { mouse.x, -mouse.y };
    }
}

void CoordDemoController::AddTarget(GameObject* object, MeshRenderer* mesh) {
    targets.push_back({ object, mesh, mesh->GetColor() });
}

void CoordDemoController::Uninit() {
    // 2Dカメラを原点へ戻す(次の画面に持ち越さない)
    Image::GetCamera().position.x = 0.f;
    Image::GetCamera().position.y = 0.f;
}

void CoordDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    if (mode == DemoMode::Mode3D) Update3D();
    else Update2D(dt);
}

void CoordDemoController::Update2D(float dt) {
    // WASD・矢印キー: 2Dカメラを動かす(物のワールドの位置は変わらず、画面に見える位置だけが変わる)
    float moveX = 0.f, moveY = 0.f;
    if (Input::GetKeyPress(KEY_D) || Input::GetKeyPress(KEY_RIGHT)) moveX += 1.f;
    if (Input::GetKeyPress(KEY_A) || Input::GetKeyPress(KEY_LEFT)) moveX -= 1.f;
    if (Input::GetKeyPress(KEY_W) || Input::GetKeyPress(KEY_UP)) moveY += 1.f;
    if (Input::GetKeyPress(KEY_S) || Input::GetKeyPress(KEY_DOWN)) moveY -= 1.f;
    Camera& camera = Image::GetCamera();
    camera.position.x = Math::Clamp(camera.position.x + moveX * cameraSpeed * dt, -cameraLimit, cameraLimit);
    camera.position.y = Math::Clamp(camera.position.y + moveY * cameraSpeed * dt, -cameraLimit, cameraLimit);

    // クリック: マウスの下のワールドの位置(Z=0の平面)に印を置く。古い印から消える
    if (Input::GetKeyDown(MOUSE_LEFT)) {
        const XMFLOAT3 world = Image::ScreenToWorld2D(GetMouseScreen());
        marks.push_back({ world.x, world.y });
        if (marks.size() > maxMarks) marks.erase(marks.begin());
    }

    // R: 印を消す
    if (Input::GetKeyDown(KEY_R)) marks.clear();
}

void CoordDemoController::Update3D() {
    // マウスの位置から奥へ伸びる線を作って、最初に当たった物を探す(トリガーは無視)
    XMFLOAT3 origin, direction;
    const XMFLOAT2 mouse = GetMouseScreen();
    Image::ScreenPointToRay3D(mouse, origin, direction);
    hasHit = Physics::Raycast(origin, direction, rayLength, hit);

    // クリック: 当たった物を選ぶ(調べられる物以外や、何も無い所をクリックしたら選びを外す)
    if (Input::GetKeyDown(MOUSE_LEFT)) {
        selected = nullptr;
        if (hasHit) {
            for (const Target& target : targets) {
                if (target.object == hit.object) selected = hit.object;
            }
        }
    }

    // 見た目: マウスが乗っている物は明るく、選んだ物は縁を光らせる
    for (const Target& target : targets) {
        const bool hover = hasHit && hit.object == target.object;
        const float lift = hover ? 0.35f : 0.f;
        target.mesh->SetColor(Math::Clamp(target.baseColor.x + lift, 0.f, 1.f), Math::Clamp(target.baseColor.y + lift, 0.f, 1.f),
            Math::Clamp(target.baseColor.z + lift, 0.f, 1.f), target.baseColor.w);
        target.mesh->SetRimLight({ 1.0f, 0.9f, 0.3f }, (target.object == selected) ? 1.2f : 0.f);
    }
}

void CoordDemoController::Draw() {
    if (mode == DemoMode::Mode3D) Draw3D();
    else Draw2D();
}

void CoordDemoController::Draw2D() {
    const XMFLOAT2 mouse = GetMouseScreen();
    const XMFLOAT3 world = Image::ScreenToWorld2D(mouse);
    const XMFLOAT2 mouseWorld = Input::GetMouseWorldPosition();
    const XMFLOAT2 back = Image::WorldToScreen2D(world);
    wchar_t buf[160];

    // マウスの位置の変換(画面 → ワールド → 画面)
    swprintf_s(buf, L"マウス(画面の座標) (%.0f, %.0f)   Input::GetMousePosition(Yの符号を反転)", mouse.x, mouse.y);
    Text::Draw(buf, listX, listY, MakeDemoTextStyle(19.f, white));
    swprintf_s(buf, L"→ ScreenToWorld2D でワールドの位置 (%.0f, %.0f)   Input::GetMouseWorldPosition も (%.0f, %.0f)", world.x, world.y, mouseWorld.x, mouseWorld.y);
    Text::Draw(buf, listX, listY - listGap, MakeDemoTextStyle(19.f, yellow));
    swprintf_s(buf, L"→ WorldToScreen2D で画面に戻すと (%.0f, %.0f)   元の位置と同じ(行き来できる)", back.x, back.y);
    Text::Draw(buf, listX, listY - listGap * 2.f, MakeDemoTextStyle(19.f, cyan));
    const XMFLOAT3 camera = Image::GetCamera().position;
    swprintf_s(buf, L"クリック: 印を置く / R: 印を消す / WASD: 2Dカメラを動かす   カメラ (%.0f, %.0f)", camera.x, camera.y);
    Text::Draw(buf, listX, listY - listGap * 3.f - 6.f, MakeDemoTextStyle(19.f, gray));

    // ワールドの原点(カメラを動かすと、画面の中の位置が変わる)
    const XMFLOAT2 originScreen = Image::WorldToScreen2D({ 0.f, 0.f, 0.f });
    Image::DrawRect(originScreen, { 12.f, 12.f }, white);
    Text::Draw(L"原点(0, 0)", originScreen.x, originScreen.y - 22.f, MakeDemoTextStyle(17.f, white, TextAlign::Center));

    // 置いた印(ワールドの位置を覚えているので、カメラが動いても同じ場所に見える)
    for (const XMFLOAT2& mark : marks) {
        const XMFLOAT2 screen = Image::WorldToScreen2D({ mark.x, mark.y, 0.f });
        Image::DrawRect(screen, { 12.f, 12.f }, yellow);
        swprintf_s(buf, L"(%.0f, %.0f)", mark.x, mark.y);
        Text::Draw(buf, screen.x, screen.y + 20.f, MakeDemoTextStyle(16.f, yellow, TextAlign::Center));
    }

    // 奥行き(z)の違うスプライト: zが違っても、画面での大きさと高さは同じ(zは大きさに影響しない)
    for (const GameObject* sprite : depthSprites) {
        const XMFLOAT3 position = sprite->transform.position;
        const XMFLOAT2 screen = Image::WorldToScreen2D(position);
        swprintf_s(buf, L"z=%.0f → 画面 (%.0f, %.0f)", position.z, screen.x, screen.y);
        Text::Draw(buf, screen.x, screen.y - sprite->transform.scale.y * 0.5f - 18.f, MakeDemoTextStyle(16.f, cyan, TextAlign::Center));
    }

    // マウスの十字(今の画面の位置)
    Image::DrawRect(mouse, { 22.f, 2.f }, cyan);
    Image::DrawRect(mouse, { 2.f, 22.f }, cyan);
}

void CoordDemoController::Draw3D() {
    wchar_t buf[200];

    // マウスの下の物(Raycastの結果)
    if (hasHit) {
        swprintf_s(buf, L"マウスの下: %ls   距離 %.0f   当たった場所 (%.0f, %.0f, %.0f)", ToWide(hit.object->GetName()).c_str(), hit.distance, hit.point.x, hit.point.y, hit.point.z);
    }
    else {
        swprintf_s(buf, L"マウスの下: 何にも当たっていません(空を向いています)");
    }
    Text::Draw(buf, listX, listY, MakeDemoTextStyle(19.f, yellow));
    Text::Draw(std::wstring(L"選んだ物: ") + (selected ? ToWide(selected->GetName()) : std::wstring(L"なし")) + L"   クリックで選ぶ(縁が光る)",
        listX, listY - listGap, MakeDemoTextStyle(19.f, white));
    Text::Draw(L"Image::ScreenPointToRay3D(マウス) → Physics::Raycast(線の始まり, 向き, 長さ, hit)   右ドラッグで視点を回してみてください",
        listX, listY - listGap * 2.f, MakeDemoTextStyle(18.f, gray));

    // 物の頭の上の名前と、画面の座標(WorldToScreen3D。カメラの後ろの物は出さない)
    for (const Target& target : targets) {
        XMFLOAT3 head = target.object->transform.position;
        head.y += target.object->transform.scale.y * 0.5f + 20.f;
        const XMFLOAT3 projected = Image::WorldToScreen3D(head);
        if (projected.z <= 0.f) continue;
        Text::Draw(ToWide(target.object->GetName()), projected.x, projected.y + 12.f, MakeDemoTextStyle(22.f, yellow, TextAlign::Center));
        swprintf_s(buf, L"画面 (%.0f, %.0f)", projected.x, projected.y);
        Text::Draw(buf, projected.x, projected.y - 12.f, MakeDemoTextStyle(15.f, white, TextAlign::Center));
    }

    // 当たった場所の印(ワールドの位置を画面の位置に直して描く)
    if (hasHit) {
        DrawDemoPoint(DemoMode::Mode3D, hit.point, 14.f, { 1.0f, 0.3f, 0.3f, 1.0f });
    }
}
