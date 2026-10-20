#include "VisionComponent.h"
#include "../Core/GameObject.h"
#include "../Render/Image.h"
#include "../Physics/Physics.h"
#include "../Core/MathUtil.h"
#include <algorithm>
#include <cmath>

bool VisionComponent::debugDrawEnabled = false;

void VisionComponent::SetFieldOfView(float degrees) {
    fieldOfView = Math::Clamp(degrees, 0.f, 360.f);
}

XMFLOAT3 VisionComponent::GetEyePosition() const {
    // 親子関係があっても正しい、ワールドの位置
    XMFLOAT3 position;
    XMStoreFloat3(&position, GetOwner()->transform.GetWorldMatrix().r[3]);
    return position + eyeOffset;
}

XMFLOAT3 VisionComponent::ToPlane(XMFLOAT3 v) const {
    if (plane == MovePlane::XZ) return { v.x, 0.f, v.z };
    return { v.x, v.y, 0.f };
}

XMFLOAT3 VisionComponent::GetFacing() const {
    // 手で指定した向きが最優先
    XMFLOAT3 manual = Math::Normalize(ToPlane(facing));
    if (Math::Length(manual) > 0.f) return manual;

    // 指定が無ければ、transformの向き(XZ=rotate.y、XY=rotate.z)
    if (plane == MovePlane::XZ) return Math::Forward3D(GetOwner()->transform);
    XMFLOAT2 forward = Math::Forward2D(GetOwner()->transform);
    return { forward.x, forward.y, 0.f };
}

bool VisionComponent::IsInCone(XMFLOAT3 point) const {
    // 平面の上での、目から相手へのずれ
    XMFLOAT3 offset = ToPlane(point - GetEyePosition());

    // 距離(2乗で比べて、sqrtを省く)
    float distanceSq = offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
    if (distanceSq > range * range) return false;

    // ほぼ同じ位置なら、向きが決まらないので見えるとする
    if (distanceSq < 1e-6f) return true;

    // 角度: 正面との内積が、視野の半分の角度のcos以上なら扇形の中(360度以上なら角度は考えない)
    if (fieldOfView >= 360.f) return true;
    XMFLOAT3 forward = GetFacing();
    if (Math::Length(forward) <= 0.f) return false;
    float cosHalf = cosf(XMConvertToRadians(fieldOfView * 0.5f));
    return Math::Dot(Math::Normalize(offset), forward) >= cosHalf;
}

bool VisionComponent::IsBlocked(XMFLOAT3 point) const {
    // 壁のタグが無ければ、壁は考えない(""だとRaycastが全部のオブジェクトを探してしまうため)
    if (obstacleTag.empty()) return false;

    XMFLOAT3 eye = GetEyePosition();
    XMFLOAT3 toPoint = point - eye;
    float distance = Math::Length(toPoint);
    if (distance < 1e-6f) return false;

    // 壁のタグの物だけを対象に、一番近い壁を探す。相手より手前にあればさえぎられている
    RaycastHit hit;
    if (!Physics::Raycast(eye, toPoint, distance, hit, obstacleTag)) return false;
    return hit.distance < distance;
}

bool VisionComponent::CanSee(XMFLOAT3 point) const {
    return IsInCone(point) && !IsBlocked(point);
}

bool VisionComponent::CanSee(GameObject* target) const {
    if (!target) return false;

    XMFLOAT3 point;
    XMStoreFloat3(&point, target->transform.GetWorldMatrix().r[3]);

    // 扇形の外なら、壁を調べるまでもなく見えない(角度と距離は毎回調べる。軽い)
    if (!IsInCone(point)) {
        seeing = false;
        return false;
    }

    // 壁の判定は重いので、間隔をあけて、結果を覚えておく(相手が変わった時はすぐにやり直す)
    if (target != cachedTarget || checkTimer <= 0.f) {
        cachedVisible = !IsBlocked(point);
        cachedTarget = target;
        // 何体もいる時に同じフレームに集中しないよう、間隔を少しだけばらす
        checkTimer = checkInterval * Math::RandomRange(0.8f, 1.2f);
    }
    seeing = cachedVisible;
    return cachedVisible;
}

void VisionComponent::Update(float dt) {
    if (checkTimer > 0.f) checkTimer -= dt;
}

void VisionComponent::Draw() {
    if (!debugDrawEnabled) return;

    // 距離1の扇形の線を作る(角度か平面が変わった時だけ作り直す)
    if (!fanMeshReady || fanMeshFov != fieldOfView || fanMeshPlane != plane) {
        if (fanMeshReady) fanMesh.Uninit();

        int segments = (std::max)(2, (int)ceilf(fieldOfView / 10.f));
        auto makeVertex = [this](float angleRad) {
            // 正面を+Z(XZ)または+Y(XY)として、そこから angleRad だけ横へ
            MeshVertex v{};
            float side = sinf(angleRad), front = cosf(angleRad);
            v.Position = (plane == MovePlane::XZ) ? XMFLOAT3{ side, 0.f, front } : XMFLOAT3{ side, front, 0.f };
            v.Normal = { 0.f, 1.f, 0.f };
            v.Color = { 1.f, 1.f, 1.f, 1.f };
            return v;
        };
        MeshVertex origin{};
        origin.Normal = { 0.f, 1.f, 0.f };
        origin.Color = { 1.f, 1.f, 1.f, 1.f };

        // 線は2頂点で1本: 中心→左の端、弧(細かい線の連なり)、右の端→中心
        std::vector<MeshVertex> lines;
        float half = XMConvertToRadians(fieldOfView * 0.5f);
        lines.push_back(origin);
        lines.push_back(makeVertex(-half));
        for (int i = 0; i < segments; i++) {
            float a0 = -half + (half * 2.f) * i / segments;
            float a1 = -half + (half * 2.f) * (i + 1) / segments;
            lines.push_back(makeVertex(a0));
            lines.push_back(makeVertex(a1));
        }
        lines.push_back(makeVertex(half));
        lines.push_back(origin);

        fanMeshReady = fanMesh.Init(lines, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        fanMeshFov = fieldOfView;
        fanMeshPlane = plane;
        if (!fanMeshReady) return;
    }

    // 距離(scale)と向き(rotate)で、目の位置に扇形を置く
    XMFLOAT3 forward = GetFacing();
    Transform t;
    t.position = GetEyePosition();
    t.scale = { range, range, range };
    if (plane == MovePlane::XZ) t.rotate = { 0.f, XMConvertToDegrees(atan2f(forward.x, forward.z)), 0.f };
    else t.rotate = { 0.f, 0.f, XMConvertToDegrees(atan2f(-forward.x, forward.y)) };

    // 光の影響を受けない一定の色(見えている時は赤、見えていない時は緑)
    static const Light fullBright{ {0.f,-1.f,0.f}, {0.f,0.f,0.f}, {1.f,1.f,1.f} };
    XMFLOAT4 color = seeing ? XMFLOAT4{ 1.f, 0.f, 0.f, 1.f } : XMFLOAT4{ 0.f, 1.f, 0.f, 1.f };
    // XY平面(2D)は、2Dスプライトと同じ2D(正射影)で描く(zで位置がずれず、スプライトの上に重なる)。XZ平面(3D)は3Dカメラの透視
    if (plane == MovePlane::XY) fanMesh.DrawLines2D(t, Image::GetCamera(), color, fullBright);
    else fanMesh.Draw(t, Image::GetCamera3D(), color, fullBright);
}

void VisionComponent::Uninit() {
    if (fanMeshReady) fanMesh.Uninit();
    fanMeshReady = false;
}
