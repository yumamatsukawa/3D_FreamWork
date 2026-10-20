#include "QueryDemoController.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoMouseGround.h"
#include "../Common/DemoStatus.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
    constexpr float rayMax2D = 1400.f;            // 2D: 線が届く限界
    constexpr float rayMax3D = 2500.f;            // 3D: 線が届く限界
    constexpr float sphereRadius2D = 130.f;       // 2D: OverlapSphere の半径(円)
    constexpr float sphereRadius3D = 250.f;       // 3D: OverlapSphere の半径
    const DirectX::XMFLOAT3 boxSize2D = { 260.f, 160.f, 100.f };   // 2D: OverlapBox の大きさ(奥行きは、物の厚みより大きく)
    const DirectX::XMFLOAT3 boxSize3D = { 450.f, 300.f, 450.f };   // 3D: OverlapBox の大きさ
    constexpr float normalLength2D = 60.f;        // 当たった面の向きの矢印の長さ
    constexpr float normalLength3D = 120.f;
    constexpr float highlightAlpha = 0.95f;       // 光らせた時の、半透明の物(トリガー)の濃さ
    constexpr float normalAlpha = 0.6f;          // 光らせない時の、半透明の物(トリガー)の濃さ
    constexpr float outlineWidth = 5.f;           // 3D: 光らせる枠線の太さ(ピクセル)

    const char* const tagNames[] = { "", "Target", "Decoy" };   // タグでの絞り込み(0=指定なし)

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.35f, 0.3f, 1.f };
    const DirectX::XMFLOAT4 orange = { 1.f, 0.65f, 0.3f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };

    // 円(2Dは XY の面、3Dは XY と XZ の2つの面)を点で描く
    void DrawRing(DemoMode mode, const DirectX::XMFLOAT3& center, float radius, bool horizontal, const DirectX::XMFLOAT4& color) {
        constexpr int count = 48;
        for (int i = 0; i < count; i++) {
            const float angle = 6.2832f * (float)i / (float)count;
            const float x = cosf(angle) * radius, y = sinf(angle) * radius;
            const DirectX::XMFLOAT3 p = horizontal ? DirectX::XMFLOAT3{ center.x + x, center.y, center.z + y }
                                                   : DirectX::XMFLOAT3{ center.x + x, center.y + y, center.z };
            DrawDemoPoint(mode, p, 4.f, color);
        }
    }

    // 箱の辺を点で描く(2Dは四角の4辺、3Dは12辺)
    void DrawBoxEdges(DemoMode mode, const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size, const DirectX::XMFLOAT4& color) {
        const int axisCount = (mode == DemoMode::Mode2D) ? 2 : 3;
        const int cornerCount = 1 << axisCount;
        auto corner = [&](int bits) {
            return DirectX::XMFLOAT3{ center.x + ((bits & 1) ? size.x : -size.x) * 0.5f,
                                      center.y + ((bits & 2) ? size.y : -size.y) * 0.5f,
                                      (axisCount == 3) ? center.z + ((bits & 4) ? size.z : -size.z) * 0.5f : center.z };
        };
        for (int i = 0; i < cornerCount; i++) {
            for (int axis = 0; axis < axisCount; axis++) {
                if (i & (1 << axis)) continue;   // 各辺は、片方の端(その軸が0の角)から1回だけ描く
                DrawDemoLine(mode, corner(i), corner(i | (1 << axis)), color, 14);
            }
        }
    }

    std::wstring ToWide(const std::string& text) {
        return std::wstring(text.begin(), text.end());   // 名前は英数字だけ
    }
}

bool QueryDemoController::IsHighlighted(const GameObject* object) const {
    if (query == Query::Raycast) return rayHit && hit.object == object;
    return std::find(found.begin(), found.end(), object) != found.end();
}

void QueryDemoController::RunQuery() {
    rayHit = false;
    found.clear();

    // 問い合わせの場所: 2Dはマウスのワールドの位置、3Dはマウスの下の「線の高さ」の面の位置
    if (mode == DemoMode::Mode2D) {
        const DirectX::XMFLOAT2 mouse = Input::GetMouseWorldPosition();
        point = { mouse.x, mouse.y, 0.f };
        hasPoint = true;
    }
    else {
        hasPoint = GetMouseGroundPoint(queryHeight, point);
    }
    if (!hasPoint) return;

    // 絞り込み(タグ・トリガー)
    const std::string tag = tagNames[tagIndex];
    const QueryTriggers triggers = includeTriggers ? QueryTriggers::Include : QueryTriggers::Ignore;
    const bool is2D = (mode == DemoMode::Mode2D);

    switch (query) {
    case Query::Raycast: {
        // 出発点からマウスの点へ、線を飛ばす(当たらなければ、届く限界までの線にする)
        const DirectX::XMFLOAT3 direction = point - origin;
        if (Math::Length(direction) < 1.f) { rayEnd = origin; return; }
        const float maxDistance = is2D ? rayMax2D : rayMax3D;
        rayEnd = origin + Math::Normalize(direction) * maxDistance;
        rayHit = Physics::Raycast(origin, direction, maxDistance, hit, tag, triggers);
        if (rayHit) rayEnd = hit.point;
        break;
    }
    case Query::OverlapSphere:
        found = Physics::OverlapSphere(point, is2D ? sphereRadius2D : sphereRadius3D, tag, triggers);
        break;
    case Query::OverlapBox:
        found = Physics::OverlapBox(point, is2D ? boxSize2D : boxSize3D, tag, triggers);
        break;
    }
}

void QueryDemoController::UpdateHighlight() {
    // 3D版: 見つかった(当たった)物に枠線を付ける。半透明の物(トリガー)は枠線が付かないので、濃くする
    for (GameObject* object : targets) {
        MeshRenderer* mesh = object->GetComponent<MeshRenderer>();
        if (!mesh) continue;
        const bool highlighted = IsHighlighted(object);
        mesh->SetOutline((query == Query::Raycast) ? red : yellow, highlighted ? outlineWidth : 0.f);
        const DirectX::XMFLOAT4 color = mesh->GetColor();
        if (color.w < 0.99f) mesh->SetColor(color.x, color.y, color.z, highlighted ? highlightAlpha : normalAlpha);
    }
}

void QueryDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1・2・3キー: 問い合わせの種類。4キー: タグでの絞り込み。5キー: トリガーも探すか
    if (Input::GetKeyDown(KEY_1)) query = Query::Raycast;
    if (Input::GetKeyDown(KEY_2)) query = Query::OverlapSphere;
    if (Input::GetKeyDown(KEY_3)) query = Query::OverlapBox;
    if (Input::GetKeyDown(KEY_4)) tagIndex = (tagIndex + 1) % 3;
    if (Input::GetKeyDown(KEY_5)) includeTriggers = !includeTriggers;

    // 問い合わせを調べる(読むだけなので、毎フレーム呼んでよい)
    RunQuery();
    if (mode == DemoMode::Mode3D) UpdateHighlight();
}

void QueryDemoController::DrawFrames() const {
    // 2D版: 見つかった(当たった)物の周りに、枠を描く
    for (const GameObject* object : targets) {
        if (!IsHighlighted(object)) continue;
        const DirectX::XMFLOAT2 center = Image::WorldToScreen2D(object->transform.position);
        const float width = object->transform.scale.x + 12.f, height = object->transform.scale.y + 12.f;
        const DirectX::XMFLOAT4& color = (query == Query::Raycast) ? red : yellow;
        Image::DrawRect({ center.x, center.y + height * 0.5f }, { width, 4.f }, color);
        Image::DrawRect({ center.x, center.y - height * 0.5f }, { width, 4.f }, color);
        Image::DrawRect({ center.x - width * 0.5f, center.y }, { 4.f, height }, color);
        Image::DrawRect({ center.x + width * 0.5f, center.y }, { 4.f, height }, color);
    }
}

void QueryDemoController::DrawQueryShape() const {
    const bool is2D = (mode == DemoMode::Mode2D);

    // 出発点(Raycastの時だけ)
    if (query == Query::Raycast) {
        DrawDemoPoint(mode, origin, 16.f, orange);
        DrawDemoLabel(mode, origin, L"出発点", 26.f);
    }
    if (!hasPoint) return;

    switch (query) {
    case Query::Raycast: {
        // 線(当たった時は、当たった所まで)
        const int dotCount = (int)Math::Clamp(Math::Distance(origin, rayEnd) / (is2D ? 14.f : 30.f), 8.f, 160.f);
        DrawDemoLine(mode, origin, rayEnd, cyan, dotCount);
        if (rayHit) {
            // 当たった場所・当たった面の向き・距離
            DrawDemoPoint(mode, hit.point, 16.f, red);
            DrawDemoLine(mode, hit.point, hit.point + hit.normal * (is2D ? normalLength2D : normalLength3D), red, 8);
            wchar_t buf[48];
            swprintf_s(buf, L"距離 %.0f", hit.distance);
            DrawDemoLabel(mode, hit.point, buf, 30.f);
        }
        break;
    }
    case Query::OverlapSphere: {
        // 球(2Dは円)の範囲
        const float radius = is2D ? sphereRadius2D : sphereRadius3D;
        DrawRing(mode, point, radius, false, yellow);
        if (!is2D) DrawRing(mode, point, radius, true, yellow);
        DrawDemoPoint(mode, point, 8.f, yellow);
        break;
    }
    case Query::OverlapBox:
        // 箱(2Dは四角)の範囲
        DrawBoxEdges(mode, point, is2D ? boxSize2D : boxSize3D, yellow);
        DrawDemoPoint(mode, point, 8.f, yellow);
        break;
    }
}

void QueryDemoController::Draw() {
    const bool is2D = (mode == DemoMode::Mode2D);
    if (is2D) DrawFrames();
    DrawQueryShape();

    // 今の問い合わせの説明
    const wchar_t* queryText = L"Raycast(出発点からマウスへ、線を飛ばす)";
    if (query == Query::OverlapSphere) queryText = L"OverlapSphere(マウスの周りの球。2Dは円)";
    if (query == Query::OverlapBox) queryText = L"OverlapBox(マウスの周りの箱。2Dは四角)";

    // 結果
    wchar_t buf[160];
    if (!hasPoint) {
        swprintf_s(buf, L"結果: マウスの下に地面が見えません");
    }
    else if (query == Query::Raycast) {
        if (rayHit) swprintf_s(buf, L"当たった物: %ls / 距離 %.0f / 場所 (%.0f, %.0f)", ToWide(hit.object->GetName()).c_str(), hit.distance,
            hit.point.x, is2D ? hit.point.y : hit.point.z);
        else swprintf_s(buf, L"当たった物: なし(線の先まで何も無い)");
    }
    else {
        int targetCount = 0, decoyCount = 0, otherCount = 0;
        for (const GameObject* object : found) {
            if (object->GetTag() == "Target") targetCount++;
            else if (object->GetTag() == "Decoy") decoyCount++;
            else otherCount++;
        }
        swprintf_s(buf, L"範囲に入った物: %d個(Target %d / Decoy %d / その他 %d)", (int)found.size(), targetCount, decoyCount, otherCount);
    }

    // 状態の一覧
    const wchar_t* tagText[] = { L"指定なし(全部)", L"Target だけ", L"Decoy だけ" };
    std::vector<DemoStatusLine> lines;
    lines.push_back({ L"[1]Raycast  [2]OverlapSphere  [3]OverlapBox", white });
    lines.push_back({ std::wstring(L"今: ") + queryText, yellow });
    lines.push_back({ std::wstring(L"[4] タグで絞る: ") + tagText[tagIndex], cyan });
    lines.push_back({ includeTriggers ? L"[5] トリガーも: 探す(QueryTriggers::Include)" : L"[5] トリガーも: 無視する(QueryTriggers::Ignore)", cyan });
    lines.push_back({ buf, green });
    DrawDemoStatus(lines, 660.f);
}
