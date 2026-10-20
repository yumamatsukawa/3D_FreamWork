#include "VisibilityDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../Common/DemoCameraController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render/Visibility.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"
#include <cmath>
#include <cstdio>

namespace {
    using DirectX::XMFLOAT2;
    using DirectX::XMFLOAT3;
    using DirectX::XMFLOAT4;

    constexpr float cameraSpeed2D = 600.f;      // 2D: カメラを動かす速さ(1秒あたり)
    constexpr float cameraLimit2DX = 1700.f;    // 2D: カメラが動ける範囲(横)
    constexpr float cameraLimit2DY = 1300.f;    // 2D: カメラが動ける範囲(縦)
    constexpr float cameraSpeed3D = 700.f;      // 3D: 見る点を動かす速さ(1秒あたり)
    constexpr float cameraLimit3D = 1900.f;     // 3D: 見る点が動ける範囲
    constexpr float screenHalfW = 640.f;        // 2D: 画面の半分の幅
    constexpr float screenHalfH = 360.f;        // 2D: 画面の半分の高さ
    constexpr float fovHalfAngle = 0.7854f;     // 3D: 地図に描く視野の半分の角度(約45度。横の視野角に近い値)
    constexpr float fovLineLength = 1600.f;     // 3D: 地図に描く視野の線の長さ

    // 地図(画面の右上寄り。説明の板の下、右下のボタンの上)
    const XMFLOAT2 mapCenter = { 490.f, 40.f };
    const XMFLOAT2 mapSize = { 280.f, 280.f };
    constexpr float mapScale2D = 0.08f;         // 2D: ワールドの1が地図の何ピクセルか
    constexpr float mapScale3D = 0.07f;         // 3D: 同上

    // 文字の後ろの板(左上)
    constexpr XMFLOAT2 panelOffset = { 12.f, -150.f };
    constexpr XMFLOAT2 panelSize = { 600.f, 170.f };
    constexpr XMFLOAT2 textOffset = { 30.f, -172.f };
    constexpr float lineGap = 25.f;

    const XMFLOAT4 green = { 0.3f, 1.0f, 0.4f, 1.0f };
    const XMFLOAT4 gray = { 0.55f, 0.55f, 0.55f, 1.0f };
    const XMFLOAT4 yellow = { 1.0f, 0.9f, 0.4f, 1.0f };
}

void VisibilityDemoController::Init() {
    ApplyCulling();
}

void VisibilityDemoController::Uninit() {
    // 2Dカメラを原点へ戻す(次の画面に持ち越さない)
    Image::GetCamera().position.x = 0.f;
    Image::GetCamera().position.y = 0.f;
}

void VisibilityDemoController::ApplyCulling() {
    // 画面の外の物を描かない設定(SetFrustumCulling)を、全部の物に入れる
    for (const Item& item : items) {
        if (item.sprite) item.sprite->SetFrustumCulling(cullingOn);
        if (item.mesh) item.mesh->SetFrustumCulling(cullingOn);
    }
}

void VisibilityDemoController::MoveCamera(float dt) {
    // WASD・矢印キーの向き(右=x+、上=y+)
    float moveX = 0.f, moveY = 0.f;
    if (Input::GetKeyPress(KEY_D) || Input::GetKeyPress(KEY_RIGHT)) moveX += 1.f;
    if (Input::GetKeyPress(KEY_A) || Input::GetKeyPress(KEY_LEFT)) moveX -= 1.f;
    if (Input::GetKeyPress(KEY_W) || Input::GetKeyPress(KEY_UP)) moveY += 1.f;
    if (Input::GetKeyPress(KEY_S) || Input::GetKeyPress(KEY_DOWN)) moveY -= 1.f;

    if (mode == DemoMode::Mode2D) {
        // 2D: 2Dカメラを動かす
        Camera& camera = Image::GetCamera();
        camera.position.x = Math::Clamp(camera.position.x + moveX * cameraSpeed2D * dt, -cameraLimit2DX, cameraLimit2DX);
        camera.position.y = Math::Clamp(camera.position.y + moveY * cameraSpeed2D * dt, -cameraLimit2DY, cameraLimit2DY);
        return;
    }

    // 3D: カメラが向いている方(yaw)を基準に、見る点を動かす(カメラは見る点について回る)
    if (!cameraTarget) return;
    const DemoCameraController* orbit = cameraTarget->GetComponent<DemoCameraController>();
    const float yaw = orbit ? orbit->GetYaw() : 0.f;
    const float forwardX = sinf(yaw), forwardZ = cosf(yaw);
    const float rightX = cosf(yaw), rightZ = -sinf(yaw);
    XMFLOAT3& position = cameraTarget->transform.position;
    position.x = Math::Clamp(position.x + (forwardX * moveY + rightX * moveX) * cameraSpeed3D * dt, -cameraLimit3D, cameraLimit3D);
    position.z = Math::Clamp(position.z + (forwardZ * moveY + rightZ * moveX) * cameraSpeed3D * dt, -cameraLimit3D, cameraLimit3D);
}

void VisibilityDemoController::Update(float dt) {
    // 1キー: 画面の外の物を描かない設定のオン/オフ
    if (Input::GetKeyDown(KEY_1)) cullingOn = !cullingOn;
    ApplyCulling();

    MoveCamera(dt);
}

bool VisibilityDemoController::IsInView(const Item& item) const {
    const XMFLOAT3 position = item.object->transform.position;
    if (mode == DemoMode::Mode2D) {
        // 2D: 円か四角が、2Dカメラの画面に少しでも入っているか
        if (item.shape == Shape::Round) return Visibility::IsCircleInView2D(position, item.size);
        return Visibility::IsRectInView2D(position, { item.size, item.size });
    }
    // 3D: 球か箱が、3Dカメラの視錐台に少しでも入っているか
    if (item.shape == Shape::Round) return Visibility::IsSphereInView3D(position, item.size);
    return Visibility::IsBoxInView3D(position, { item.size, item.size, item.size });
}

void VisibilityDemoController::DrawMap() {
    const float scale = (mode == DemoMode::Mode2D) ? mapScale2D : mapScale3D;
    const bool is2D = (mode == DemoMode::Mode2D);

    // ワールドの位置(2D=x,y / 3D=x,z)→ 地図の上の位置。地図の外なら false
    auto toMap = [&](float worldX, float worldY, XMFLOAT2& out) {
        out = { mapCenter.x + worldX * scale, mapCenter.y + worldY * scale };
        return fabsf(out.x - mapCenter.x) < mapSize.x * 0.5f - 2.f && fabsf(out.y - mapCenter.y) < mapSize.y * 0.5f - 2.f;
    };

    // 地図の板
    Image::DrawRect(mapCenter, mapSize, { 0.0f, 0.0f, 0.0f, 0.6f });

    // 物(映っている=緑、映っていない=灰色)
    XMFLOAT2 point;
    for (const Item& item : items) {
        const XMFLOAT3 position = item.object->transform.position;
        if (toMap(position.x, is2D ? position.y : position.z, point)) {
            const float size = (item.shape == Shape::Round) ? 5.f : 7.f;
            Image::DrawRect(point, { size, size }, IsInView(item) ? green : gray);
        }
    }

    if (is2D) {
        // カメラが映している範囲(四角)
        const XMFLOAT3 camera = Image::GetCamera().position;
        const float width = screenHalfW * 2.f * scale, height = screenHalfH * 2.f * scale;
        XMFLOAT2 edge;
        if (toMap(camera.x, camera.y + screenHalfH, edge)) Image::DrawRect(edge, { width, 2.f }, yellow);
        if (toMap(camera.x, camera.y - screenHalfH, edge)) Image::DrawRect(edge, { width, 2.f }, yellow);
        if (toMap(camera.x - screenHalfW, camera.y, edge)) Image::DrawRect(edge, { 2.f, height }, yellow);
        if (toMap(camera.x + screenHalfW, camera.y, edge)) Image::DrawRect(edge, { 2.f, height }, yellow);
        return;
    }

    // 3D: カメラの位置と、映す範囲(視野の広がり)を線で描く
    const Camera& camera = Image::GetCamera3D();
    float directionX = camera.target.x - camera.position.x;
    float directionZ = camera.target.z - camera.position.z;
    const float length = sqrtf(directionX * directionX + directionZ * directionZ);
    if (length < 0.001f) return;
    directionX /= length;
    directionZ /= length;
    for (int side = -1; side <= 1; side += 2) {
        const float angle = fovHalfAngle * (float)side;
        const float lineX = directionX * cosf(angle) - directionZ * sinf(angle);
        const float lineZ = directionX * sinf(angle) + directionZ * cosf(angle);
        for (float distance = 0.f; distance <= fovLineLength; distance += 80.f) {
            if (toMap(camera.position.x + lineX * distance, camera.position.z + lineZ * distance, point)) Image::DrawRect(point, { 3.f, 3.f }, yellow);
        }
    }
    if (toMap(camera.position.x, camera.position.z, point)) Image::DrawRect(point, { 9.f, 9.f }, { 1.0f, 0.4f, 0.3f, 1.0f });
}

void VisibilityDemoController::Draw() {
    const bool is2D = (mode == DemoMode::Mode2D);

    // 物ごとに、画面に映っているかを Visibility で調べて数える(円/球と、四角/箱に分けて)
    int roundIn = 0, roundAll = 0, squareIn = 0, squareAll = 0;
    for (const Item& item : items) {
        const bool inView = IsInView(item);
        if (item.shape == Shape::Round) { roundAll++; if (inView) roundIn++; }
        else { squareAll++; if (inView) squareIn++; }
    }

    // 右上の地図
    DrawMap();
    Text::Draw(is2D ? L"地図(上から見た2Dの世界)" : L"地図(上から見た3Dの世界)", mapCenter.x, mapCenter.y + mapSize.y * 0.5f + 14.f,
        MakeDemoTextStyle(16.f, { 1.0f, 1.0f, 1.0f, 1.0f }, TextAlign::Center));
    Text::Draw(L"緑=映っている 灰=映っていない 黄=カメラの範囲", mapCenter.x, mapCenter.y - mapSize.y * 0.5f - 14.f,
        MakeDemoTextStyle(14.f, { 0.9f, 0.9f, 0.9f, 1.0f }, TextAlign::Center));

    // 文字の後ろの板と、状態の一覧
    Image::DrawRect(GetAnchoredCenter(Anchor::TopLeft, panelOffset, panelSize), panelSize, { 0.0f, 0.0f, 0.0f, 0.55f });
    XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, textOffset);
    wchar_t buf[160];
    auto line = [&](const wchar_t* text, const XMFLOAT4& color) {
        Text::Draw(text, pos.x, pos.y, MakeDemoTextStyle(19.f, color));
        pos.y -= lineGap;
    };
    swprintf_s(buf, L"[1] 画面の外を描かない(SetFrustumCulling): %ls", cullingOn ? L"ON" : L"OFF");
    line(buf, yellow);
    swprintf_s(buf, L"映っている物: %d / %d", roundIn + squareIn, roundAll + squareAll);
    line(buf, { 0.7f, 1.0f, 0.7f, 1.0f });
    swprintf_s(buf, L"  %ls: %d / %d", is2D ? L"円 IsCircleInView2D" : L"球 IsSphereInView3D", roundIn, roundAll);
    line(buf, { 0.7f, 1.0f, 0.7f, 1.0f });
    swprintf_s(buf, L"  %ls: %d / %d", is2D ? L"四角 IsRectInView2D" : L"箱 IsBoxInView3D", squareIn, squareAll);
    line(buf, { 0.7f, 1.0f, 0.7f, 1.0f });
    swprintf_s(buf, L"描かずに省いた数(GetCulledCount): %d", Visibility::GetCulledCount());
    line(buf, { 0.6f, 0.9f, 1.0f, 1.0f });
    line(is2D ? L"WASD・矢印: 2Dカメラを動かす" : L"WASD・矢印: 見る点を動かす(右ドラッグで向き)", { 1.0f, 1.0f, 1.0f, 1.0f });
}
