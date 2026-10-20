#include "PathFollowComponent.h"
#include "NavGridComponent.h"
#include "../Core/GameObject.h"
#include "../Render/Image.h"
#include "../Core/MathUtil.h"
#include <windows.h>
#include <cmath>
#include <cstdio>

bool PathFollowComponent::debugDrawEnabled = false;
int PathFollowComponent::instanceCount = 0;
Mesh PathFollowComponent::lineMesh;
bool PathFollowComponent::lineMeshReady = false;

namespace {
    constexpr float stuckCheckInterval = 1.f;     // 詰まりを調べる間隔(秒)
    constexpr float stuckSpeedRatio = 0.2f;       // この割合(速さ x 時間)も進めなかったら詰まったとみなす
    constexpr float arriveDistance = 0.5f;        // 最後の点にここまで近づいたら止まる
    constexpr float failedRepathScale = 4.f;      // 経路が見つからなかった時は、探し直す間隔をこの倍数にする(行けない所の探索は重いため。目的地や格子が変わればすぐ探し直す)
}

void PathFollowComponent::Init() {
    instanceCount++;
}

float PathFollowComponent::GetWaypointRadius() const {
    if (waypointRadius >= 0.f) return waypointRadius;
    return navigation ? navigation->GetGrid().GetSettings().cellSize * 0.5f : 25.f;
}

void PathFollowComponent::ResetPath() {
    path.clear();
    pathIndex = 0;
    finalApproach = false;
    needsRepath = true;
    repathTimer = 0.f;
    stuckTimer = 0.f;
}

void PathFollowComponent::Update(float dt) {
    // 目的地が無い・着いた時は止まる(経路は捨てて、次に動く時に探し直す)
    XMFLOAT3 offset;
    if (!GetOffsetToDestination(offset) || IsWithinStopDistance(offset)) {
        ResetPath();
        ApplyVelocity({ 0.f, 0.f, 0.f }, dt);
        return;
    }

    // 格子が渡されていない・まだ焼けていない間は止まる
    if (!navigation) {
        if (!warnedNoNavigation) {
            char message[256];
            sprintf_s(message, "★ PathFollowComponent: SetNavigation で格子を渡してください(%s)\n", GetOwner()->GetName().c_str());
            OutputDebugStringA(message);
            warnedNoNavigation = true;
        }
        ApplyVelocity({ 0.f, 0.f, 0.f }, dt);
        return;
    }
    if (!navigation->IsBaked()) {
        needsRepath = true;
        ApplyVelocity({ 0.f, 0.f, 0.f }, dt);
        return;
    }

    XMFLOAT3 position = GetOwner()->transform.position;
    XMFLOAT3 destination = position + offset;   // 平面の外の成分は自分と同じ
    NavGrid& grid = navigation->GetGrid();

    // 探し直す時か: 間隔・目的地が1マス以上動いた・格子が変わった
    repathTimer -= dt;
    if (repathTimer <= 0.f) {
        if (finalApproach) repathTimer = repathInterval;   // 壁際の目的地へ向かっている間は、目的地が動くまで探し直さない(寄せ先に引き戻されないように)
        else needsRepath = true;
    }
    // 目的地が1マス以上動いた時(経路が無い間は、間隔の下限を守る。行けない所の探索は重いため)
    if (Math::Length(ToPlane(destination - lastDestination)) >= grid.GetSettings().cellSize && (!path.empty() || repathTimer <= 0.f)) needsRepath = true;
    if (grid.GetVersion() != lastGridVersion) needsRepath = true;

    // 詰まり: 経路があるのに、1秒間にほとんど進めなかったら探し直す
    if (path.empty()) {
        stuckTimer = 0.f;
        stuckStartPosition = position;
    }
    else {
        stuckTimer += dt;
        if (stuckTimer >= stuckCheckInterval) {
            float moved = Math::Length(ToPlane(position - stuckStartPosition));
            if (moved < speed * stuckSpeedRatio * stuckTimer) needsRepath = true;
            stuckTimer = 0.f;
            stuckStartPosition = position;
        }
    }

    // 探索(1フレームの上限に達していたら、次のフレームにまわす)
    if (needsRepath && navigation->TryBeginSearch()) {
        path = navigation->GetPathFinder().FindPath(position, destination);
        pathIndex = 0;
        finalApproach = false;
        needsRepath = false;
        lastDestination = destination;
        lastGridVersion = grid.GetVersion();
        repathTimer = repathInterval * Math::RandomRange(0.8f, 1.2f);   // 何体もいる時に同じフレームに集中しないよう、少しばらす
        if (path.empty()) repathTimer *= failedRepathScale;
        stuckTimer = 0.f;
        stuckStartPosition = position;
    }

    // 経路が無い時は止まる
    if (path.empty()) {
        ApplyVelocity({ 0.f, 0.f, 0.f }, dt);
        return;
    }

    // 次の点へ: 近づいたら進める(最後の点は進めずに、そこで止まる)
    float radius = GetWaypointRadius();
    while (pathIndex + 1 < path.size() && Math::Length(ToPlane(path[pathIndex] - position)) < radius) pathIndex++;

    // 移動処理: 今向かっている点へ一定の速さで。最後の点は、行き過ぎて往復しないように残りの距離までにする
    XMFLOAT3 toPoint = ToPlane(path[pathIndex] - position);
    float distance = Math::Length(toPoint);
    bool isLastPoint = (pathIndex + 1 == path.size());
    if (isLastPoint && (finalApproach || distance < arriveDistance)) {
        // 最後の点に着いた。目的地が通れないマスの中で、近くのマスに寄せていた時は、残りをまっすぐ向かう(目的地に着いていれば、上で止まっている)
        finalApproach = true;
        toPoint = offset;
        distance = Math::Length(offset);
    }
    float moveSpeed = speed;
    if (isLastPoint && dt > 0.f && moveSpeed * dt > distance) moveSpeed = distance / dt;
    ApplyVelocity(Math::Normalize(toPoint) * moveSpeed, dt);
}

void PathFollowComponent::DrawLine(XMFLOAT3 from, XMFLOAT3 to) {
    // 長さ1の線(+Z向き)を、長さ(scale.z)と向き(rotate)で from から to へ
    XMFLOAT3 direction = to - from;
    float length = Math::Length(direction);
    if (length < 1e-4f) return;
    XMFLOAT3 unit = direction * (1.f / length);

    Transform t;
    t.position = from;
    t.scale = { 1.f, 1.f, length };
    t.rotate = { XMConvertToDegrees(-asinf(Math::Clamp(unit.y, -1.f, 1.f))), XMConvertToDegrees(atan2f(unit.x, unit.z)), 0.f };

    // 光の影響を受けない一定の緑
    static const Light fullBright{ {0.f,-1.f,0.f}, {0.f,0.f,0.f}, {1.f,1.f,1.f} };
    // XY平面(2D)は、2Dスプライトと同じ2D(正射影)で描く。XZ平面(3D)は3Dカメラの透視
    if (plane == MovePlane::XY) lineMesh.DrawLines2D(t, Image::GetCamera(), XMFLOAT4{ 0.f, 1.f, 0.f, 1.f }, fullBright);
    else lineMesh.Draw(t, Image::GetCamera3D(), XMFLOAT4{ 0.f, 1.f, 0.f, 1.f }, fullBright);
}

void PathFollowComponent::Draw() {
    if (!debugDrawEnabled || path.empty() || pathIndex >= path.size()) return;

    // 線のMeshは全員で1つ共有する(初めて描く時に作る)
    if (!lineMeshReady) {
        MeshVertex a{}, b{};
        a.Normal = b.Normal = { 0.f, 1.f, 0.f };
        a.Color = b.Color = { 1.f, 1.f, 1.f, 1.f };
        b.Position = { 0.f, 0.f, 1.f };
        lineMeshReady = lineMesh.Init({ a, b }, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        if (!lineMeshReady) return;
    }

    // 自分の位置 → 今向かっている点 → その先の点、と線でつなぐ(3Dは地面に埋もれないよう少し浮かせる)
    float lift = (plane == MovePlane::XZ) ? 5.f : 0.f;
    auto lifted = [lift, this](XMFLOAT3 p) { if (plane == MovePlane::XZ) p.y += lift; return p; };
    XMFLOAT3 current = GetOwner()->transform.position;
    if (plane == MovePlane::XZ) current.y = path[pathIndex].y;
    current = lifted(current);
    for (size_t i = pathIndex; i < path.size(); i++) {
        XMFLOAT3 next = lifted(path[i]);
        DrawLine(current, next);
        current = next;
    }
}

void PathFollowComponent::Uninit() {
    // 線のMeshは、使っている最後の1つが片付く時に解放する
    instanceCount--;
    if (instanceCount <= 0 && lineMeshReady) {
        lineMesh.Uninit();
        lineMeshReady = false;
        instanceCount = 0;
    }
}

void PathFollowComponent::OnActiveChanged(bool active) {
    // ObjectPoolで再利用された時は、前回の経路を残さない
    if (active) ResetPath();
}
