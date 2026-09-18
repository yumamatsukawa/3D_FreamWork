#include "Collider.h"
#include "GameObject.h"
#include "Image.h"
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cmath>

using namespace DirectX;

static std::vector<Collider2D*> g_Colliders;
static std::unordered_set<uint64_t> g_PrevPairs;
static std::unordered_set<uint64_t> g_CurrPairs;
unsigned int g_DebugBoxTexID = UINT_MAX;
unsigned int g_DebugCircleTexID = UINT_MAX;

void InitCollider() {
    g_DebugBoxTexID = Image::LoadTexture(L"Assets/debug_box.png");
    g_DebugCircleTexID = Image::LoadTexture(L"Assets/debug_circle.png");
}

void UninitCollider() {
    Image::ReleaseTexture(g_DebugBoxTexID);
    Image::ReleaseTexture(g_DebugCircleTexID);
}

void RegisterCollider(Collider2D* col) {
    g_Colliders.push_back(col);
}

void UnregisterCollider(Collider2D* col) {
    g_Colliders.erase(
        std::remove(g_Colliders.begin(), g_Colliders.end(), col),
        g_Colliders.end());
}

const std::string& Collider2D::GetTag() const {
    return owner->GetTag();
}

// BoxCollider2D の GetCorners
void BoxCollider2D::GetCorners(XMFLOAT2 outCorners[4]) const {
    const Transform t = owner->transform.GetWorldTransform();  // 親を辿ったワールド座標で判定する

    // ★ size が 0 なら transform.scale を使う
    float w = (size.x > 0.f) ? size.x : t.scale.x;
    float h = (size.y > 0.f) ? size.y : t.scale.y;

    // ★ 見た目(Sprite)の回転方向に合わせるため、他のBox系関数(CheckCircleBox等)と
    //   同じく符号を反転させる(反転させないと見た目と逆向きに回転してしまう)
    float rad = -t.rotate.z * (XM_PI / 180.f);
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    // ★ offset を回転させてから中心座標に加算
    float ox = offset.x * cosA - offset.y * sinA;
    float oy = offset.x * sinA + offset.y * cosA;
    float cx = t.position.x + ox;
    float cy = t.position.y + oy;

    XMFLOAT2 local[4] = {
        { -w / 2, -h / 2 },
        {  w / 2, -h / 2 },
        {  w / 2,  h / 2 },
        { -w / 2,  h / 2 },
    };

    for (int i = 0; i < 4; i++) {
        float rx = local[i].x * cosA - local[i].y * sinA;
        float ry = local[i].x * sinA + local[i].y * cosA;
        outCorners[i] = { cx + rx, cy + ry };
    }
}

// CircleCollider2D の GetCenter / GetRadius
XMFLOAT2 CircleCollider2D::GetCenter() const {
    const Transform t = owner->transform.GetWorldTransform();  // 親を辿ったワールド座標で判定する

    // ★ offset を回転させてから中心座標に加算(見た目の回転方向に合わせて符号反転)
    float rad = -t.rotate.z * (XM_PI / 180.f);
    float cosA = cosf(rad);
    float sinA = sinf(rad);
    float ox = offset.x * cosA - offset.y * sinA;
    float oy = offset.x * sinA + offset.y * cosA;

    return { t.position.x + ox, t.position.y + oy };
}

float CircleCollider2D::GetRadius() const {
    return (radius > 0.f) ? radius : owner->transform.GetWorldTransform().scale.x / 2.f;
}

// ─── 判定関数 ─────────────────────────────────

// SAT（Box vs Box）
static void Project(const XMFLOAT2 corners[4], const XMFLOAT2& axis,
    float& outMin, float& outMax) {
    outMin = outMax = corners[0].x * axis.x + corners[0].y * axis.y;
    for (int i = 1; i < 4; i++) {
        float proj = corners[i].x * axis.x + corners[i].y * axis.y;
        outMin = min(outMin, proj);
        outMax = max(outMax, proj);
    }
}

static bool CheckBoxBox(BoxCollider2D* a, BoxCollider2D* b) {
    XMFLOAT2 cornersA[4], cornersB[4];
    a->GetCorners(cornersA);
    b->GetCorners(cornersB);

    XMFLOAT2 axes[4];
    auto getAxis = [](const XMFLOAT2& p1, const XMFLOAT2& p2) {
        return XMFLOAT2{ -(p2.y - p1.y), p2.x - p1.x };
        };
    axes[0] = getAxis(cornersA[0], cornersA[1]);
    axes[1] = getAxis(cornersA[1], cornersA[2]);
    axes[2] = getAxis(cornersB[0], cornersB[1]);
    axes[3] = getAxis(cornersB[1], cornersB[2]);

    for (int i = 0; i < 4; i++) {
        float len = sqrtf(axes[i].x * axes[i].x + axes[i].y * axes[i].y);
        if (len == 0.f) continue;
        XMFLOAT2 axis = { axes[i].x / len, axes[i].y / len };
        float minA, maxA, minB, maxB;
        Project(cornersA, axis, minA, maxA);
        Project(cornersB, axis, minB, maxB);
        if (maxA < minB || maxB < minA) return false;
    }
    return true;
}

// 円 vs 円
static bool CheckCircleCircle(CircleCollider2D* a, CircleCollider2D* b) {
    XMFLOAT2 ca = a->GetCenter();
    XMFLOAT2 cb = b->GetCenter();
    float dx = ca.x - cb.x;
    float dy = ca.y - cb.y;
    float dist2 = dx * dx + dy * dy;
    float r = a->GetRadius() + b->GetRadius();
    return dist2 <= r * r;
}

// 円 vs Box
static bool CheckCircleBox(CircleCollider2D* circle, BoxCollider2D* box) {
    // 円の中心をBoxのローカル座標に変換
    const Transform t = box->owner->transform.GetWorldTransform();  // 親を辿ったワールド座標で判定する
    float rad = -t.rotate.z * (XM_PI / 180.f);  // 逆回転
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    XMFLOAT2 center = circle->GetCenter();
    float dx = center.x - t.position.x;
    float dy = center.y - t.position.y;

    // ローカル座標に変換
    float localX = dx * cosA - dy * sinA;
    float localY = dx * sinA + dy * cosA;

    // 最近接点を求める（クランプ）
    float halfW = t.scale.x / 2.f;
    float halfH = t.scale.y / 2.f;
    float closestX = max(-halfW, min(halfW, localX));
    float closestY = max(-halfH, min(halfH, localY));

    // 最近接点と円の中心の距離を比較
    float distX = localX - closestX;
    float distY = localY - closestY;
    float dist2 = distX * distX + distY * distY;
    float r = circle->GetRadius();
    return dist2 <= r * r;
}

// ─── めり込み量と押し戻し方向を計算（Box vs Box）───
static bool CheckBoxBoxWithInfo(BoxCollider2D* a, BoxCollider2D* b,
    CollisionInfo& infoA, CollisionInfo& infoB) {
    XMFLOAT2 cornersA[4], cornersB[4];
    a->GetCorners(cornersA);
    b->GetCorners(cornersB);

    XMFLOAT2 axes[4];
    auto getAxis = [](const XMFLOAT2& p1, const XMFLOAT2& p2) {
        return XMFLOAT2{ -(p2.y - p1.y), p2.x - p1.x };
        };
    axes[0] = getAxis(cornersA[0], cornersA[1]);
    axes[1] = getAxis(cornersA[1], cornersA[2]);
    axes[2] = getAxis(cornersB[0], cornersB[1]);
    axes[3] = getAxis(cornersB[1], cornersB[2]);

    float    minOverlap = FLT_MAX;
    XMFLOAT2 minAxis = { 0.f, 0.f };

    for (int i = 0; i < 4; i++) {
        float len = sqrtf(axes[i].x * axes[i].x + axes[i].y * axes[i].y);
        if (len == 0.f) continue;
        XMFLOAT2 axis = { axes[i].x / len, axes[i].y / len };

        float minA, maxA, minB, maxB;
        Project(cornersA, axis, minA, maxA);
        Project(cornersB, axis, minB, maxB);

        if (maxA < minB || maxB < minA) return false;

        // ★ 重なり量を計算
        float overlap = min(maxA, maxB) - max(minA, minB);
        if (overlap < minOverlap) {
            minOverlap = overlap;
            minAxis = axis;
        }
    }

    // ★ 押し戻し方向を A→B に向ける
    XMFLOAT2 centerA = {
        (cornersA[0].x + cornersA[2].x) / 2.f,
        (cornersA[0].y + cornersA[2].y) / 2.f
    };
    XMFLOAT2 centerB = {
        (cornersB[0].x + cornersB[2].x) / 2.f,
        (cornersB[0].y + cornersB[2].y) / 2.f
    };

    XMFLOAT2 dir = { centerB.x - centerA.x, centerB.y - centerA.y };
    float dot = dir.x * minAxis.x + dir.y * minAxis.y;
    if (dot < 0) {
        minAxis.x = -minAxis.x;
        minAxis.y = -minAxis.y;
    }

    // A の情報（Bから離れる方向、= Bに向かう軸の逆向き）
    infoA.other = b;
    infoA.normal = { -minAxis.x, -minAxis.y };
    infoA.depth = minOverlap;

    // B の情報（Aから離れる方向）
    infoB.other = a;
    infoB.normal = minAxis;
    infoB.depth = minOverlap;

    return true;
}

static bool CheckCircleCircleWithInfo(CircleCollider2D* a, CircleCollider2D* b,
    CollisionInfo& infoA, CollisionInfo& infoB) {
    XMFLOAT2 ca = a->GetCenter();
    XMFLOAT2 cb = b->GetCenter();
    float dx = cb.x - ca.x;
    float dy = cb.y - ca.y;
    float dist = sqrtf(dx * dx + dy * dy);
    float r = a->GetRadius() + b->GetRadius();

    if (dist >= r) return false;

    // A→B の正規化ベクトル。押し戻しはお互いこの逆向き(離れる方向)にする
    float nx = (dist > 0.f) ? dx / dist : 1.f;
    float ny = (dist > 0.f) ? dy / dist : 0.f;
    float depth = r - dist;

    infoA = { b, { -nx, -ny }, depth };  // Aから見て、Bから離れる方向
    infoB = { a, {  nx,  ny }, depth };  // Bから見て、Aから離れる方向

    return true;
}

static bool CheckCircleBoxWithInfo(CircleCollider2D* circle, BoxCollider2D* box,
    CollisionInfo& infoCircle, CollisionInfo& infoBox) {
    const Transform t = box->owner->transform.GetWorldTransform();  // 親を辿ったワールド座標で判定する
    float rad = -t.rotate.z * (XM_PI / 180.f);
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    XMFLOAT2 center = circle->GetCenter();
    float dx = center.x - t.position.x;
    float dy = center.y - t.position.y;

    // ローカル座標に変換
    float localX = dx * cosA - dy * sinA;
    float localY = dx * sinA + dy * cosA;

    float halfW = (box->size.x > 0.f) ? box->size.x / 2.f : t.scale.x / 2.f;
    float halfH = (box->size.y > 0.f) ? box->size.y / 2.f : t.scale.y / 2.f;

    // 最近接点
    float closestX = max(-halfW, min(halfW, localX));
    float closestY = max(-halfH, min(halfH, localY));

    float distX = localX - closestX;
    float distY = localY - closestY;
    float dist2 = distX * distX + distY * distY;
    float r = circle->GetRadius();

    if (dist2 >= r * r) return false;

    float dist = sqrtf(dist2);
    float depth = r - dist;

    // ★ ローカル座標の押し戻し方向をワールド座標に戻す
    float lnx = (dist > 0.f) ? distX / dist : 0.f;
    float lny = (dist > 0.f) ? distY / dist : 1.f;

    // ローカル → ワールド（逆回転）
    float cosB = cosf(-rad);
    float sinB = sinf(-rad);
    float nx = lnx * cosB - lny * sinB;
    float ny = lnx * sinB + lny * cosB;

    infoCircle = { box,    {  nx,  ny }, depth };
    infoBox = { circle, { -nx, -ny }, depth };

    return true;
}

// ─── 判定ディスパッチ ─────────────────────────
static bool CheckCollision(Collider2D* a, Collider2D* b) {
    auto shapeA = a->GetShape();
    auto shapeB = b->GetShape();

    if (shapeA == Collider2D::Shape::Box &&
        shapeB == Collider2D::Shape::Box) {
        return CheckBoxBox(
            static_cast<BoxCollider2D*>(a),
            static_cast<BoxCollider2D*>(b));
    }
    if (shapeA == Collider2D::Shape::Circle &&
        shapeB == Collider2D::Shape::Circle) {
        return CheckCircleCircle(
            static_cast<CircleCollider2D*>(a),
            static_cast<CircleCollider2D*>(b));
    }
    if (shapeA == Collider2D::Shape::Circle &&
        shapeB == Collider2D::Shape::Box) {
        return CheckCircleBox(
            static_cast<CircleCollider2D*>(a),
            static_cast<BoxCollider2D*>(b));
    }
    if (shapeA == Collider2D::Shape::Box &&
        shapeB == Collider2D::Shape::Circle) {
        return CheckCircleBox(
            static_cast<CircleCollider2D*>(b),
            static_cast<BoxCollider2D*>(a));
    }

    return false;
}

static uint64_t MakePairId(int a, int b) {
    if (a > b) std::swap(a, b);
    return ((uint64_t)a << 32) | (uint64_t)b;
}

void UpdateCollider() {
    g_CurrPairs.clear();

    for (int i = 0; i < (int)g_Colliders.size(); i++) {
        if (!g_Colliders[i]->enabled) continue;
        for (int j = i + 1; j < (int)g_Colliders.size(); j++) {
            if (!g_Colliders[j]->enabled) continue;

            Collider2D* a = g_Colliders[i];
            Collider2D* b = g_Colliders[j];

            bool bothTrigger = a->isTrigger || b->isTrigger;

            if (bothTrigger) {
                // ─── Trigger 判定 ──────────────────
                if (CheckCollision(a, b)) {
                    uint64_t pairId = MakePairId(i, j);
                    g_CurrPairs.insert(pairId);

                    if (g_PrevPairs.count(pairId)) {
                        a->owner->OnTriggerStay2D(b);
                        b->owner->OnTriggerStay2D(a);
                    }
                    else {
                        a->owner->OnTriggerEnter2D(b);
                        b->owner->OnTriggerEnter2D(a);
                    }
                }
            }
            else {
                CollisionInfo infoA, infoB;
                bool hit = false;

                auto shapeA = a->GetShape();
                auto shapeB = b->GetShape();

                if (shapeA == Collider2D::Shape::Box &&
                    shapeB == Collider2D::Shape::Box) {
                    hit = CheckBoxBoxWithInfo(
                        static_cast<BoxCollider2D*>(a),
                        static_cast<BoxCollider2D*>(b),
                        infoA, infoB);
                }
                else if (shapeA == Collider2D::Shape::Circle &&
                    shapeB == Collider2D::Shape::Circle) {
                    hit = CheckCircleCircleWithInfo(
                        static_cast<CircleCollider2D*>(a),
                        static_cast<CircleCollider2D*>(b),
                        infoA, infoB);
                }
                else if (shapeA == Collider2D::Shape::Circle &&
                    shapeB == Collider2D::Shape::Box) {
                    hit = CheckCircleBoxWithInfo(
                        static_cast<CircleCollider2D*>(a),
                        static_cast<BoxCollider2D*>(b),
                        infoA, infoB);
                }
                else if (shapeA == Collider2D::Shape::Box &&
                    shapeB == Collider2D::Shape::Circle) {
                    // ★ 引数の順番に注意（Circle が先）
                    hit = CheckCircleBoxWithInfo(
                        static_cast<CircleCollider2D*>(b),
                        static_cast<BoxCollider2D*>(a),
                        infoB, infoA);  // ★ infoB, infoA の順番
                }

                if (hit) {
                    // ★ めり込み分の押し戻し。isStaticなら自分は動かず、相手側で全部吸収する
                    //   両方staticなら誰も動かない
                    float ratioA = a->isStatic ? 0.f : (b->isStatic ? 1.f : 0.5f);
                    float ratioB = b->isStatic ? 0.f : (a->isStatic ? 1.f : 0.5f);

                    // ワールド空間で計算した押し戻し量を、それぞれの親の回転・拡縮を
                    // 考慮した上でローカルのpositionに反映する
                    XMFLOAT2 worldPushA = { infoA.normal.x * infoA.depth * ratioA, infoA.normal.y * infoA.depth * ratioA };
                    XMFLOAT2 worldPushB = { infoB.normal.x * infoB.depth * ratioB, infoB.normal.y * infoB.depth * ratioB };
                    XMFLOAT2 localPushA = a->owner->transform.WorldDeltaToLocal(worldPushA);
                    XMFLOAT2 localPushB = b->owner->transform.WorldDeltaToLocal(worldPushB);

                    a->owner->transform.position.x += localPushA.x;
                    a->owner->transform.position.y += localPushA.y;
                    b->owner->transform.position.x += localPushB.x;
                    b->owner->transform.position.y += localPushB.y;

                    uint64_t pairId = MakePairId(i, j);
                    g_CurrPairs.insert(pairId);

                    if (g_PrevPairs.count(pairId)) {
                        a->owner->OnCollisionStay2D(infoA);
                        b->owner->OnCollisionStay2D(infoB);
                    }
                    else {
                        a->owner->OnCollisionEnter2D(infoA);
                        b->owner->OnCollisionEnter2D(infoB);
                    }
                }
            }
        }
    }

    // ─── Exit 判定 ────────────────────────────
    for (auto pairId : g_PrevPairs) {
        if (!g_CurrPairs.count(pairId)) {
            int ai = (int)(pairId >> 32);
            int bi = (int)(pairId & 0xFFFFFFFF);
            if (ai < (int)g_Colliders.size() &&
                bi < (int)g_Colliders.size()) {
                Collider2D* a = g_Colliders[ai];
                Collider2D* b = g_Colliders[bi];

                bool bothTrigger = a->isTrigger || b->isTrigger;
                if (bothTrigger) {
                    a->owner->OnTriggerExit2D(b);
                    b->owner->OnTriggerExit2D(a);
                }
                else {
                    CollisionInfo infoA = { b, { 0.f, 0.f }, 0.f };
                    CollisionInfo infoB = { a, { 0.f, 0.f }, 0.f };
                    a->owner->OnCollisionExit2D(infoA);
                    b->owner->OnCollisionExit2D(infoB);
                }
            }
        }
    }

    g_PrevPairs = g_CurrPairs;
}

void DrawColliders() {
    for (auto col : g_Colliders) {
        if (!col->enabled) continue;
        if (!col->owner->GetIsActive()) continue;
        unsigned int debugTexID = g_DebugBoxTexID;
        if (col->GetShape() == Collider2D::Shape::Circle)
        {
            debugTexID = g_DebugCircleTexID;
        }
        Image::Draw(col->owner->transform.GetWorldTransform(), debugTexID);
    }
}

bool IsPointInBox(XMFLOAT2 point, const Transform& transform) {
    // 回転を考慮:点をオブジェクトのローカル座標系に変換して判定
    float dx = point.x - transform.position.x;
    float dy = point.y - transform.position.y;

    float rad = -transform.rotate.z * (XM_PI / 180.f);  // 逆回転
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    float localX = dx * cosA - dy * sinA;
    float localY = dx * sinA + dy * cosA;

    float halfW = transform.scale.x / 2.f;
    float halfH = transform.scale.y / 2.f;

    return (localX >= -halfW && localX <= halfW &&
        localY >= -halfH && localY <= halfH);
}