#include "RigidbodyComponent.h"
#include "Physics.h"
#include "Mesh.h"
#include "Image.h"
#include <windows.h>
#include <PxPhysicsAPI.h>

using namespace physx;
using namespace DirectX;

bool RigidbodyComponent::debugDrawEnabled = false;

namespace {
    // DirectX(左手系)とPhysX(右手系)とで回転の向きが逆になるため、変換が必要。
    // X/Y成分だけを反転させる変換で、これは自分自身の逆変換にもなっている
    // (DirectX→PhysXでもPhysX→DirectXでも同じ式で変換できる)。
    PxQuat ToPxQuat(const XMFLOAT3& eulerDegrees) {
        XMMATRIX m = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(eulerDegrees.x),
            XMConvertToRadians(eulerDegrees.y),
            XMConvertToRadians(eulerDegrees.z));
        XMFLOAT4 qf;
        XMStoreFloat4(&qf, XMQuaternionRotationMatrix(m));
        return PxQuat(-qf.x, -qf.y, qf.z, qf.w);
    }

    PxVec3 ToPxPos(const Transform& t) {
        return PxVec3(t.position.x, t.position.y, t.position.z);
    }

    // PxRigidActorの現在の位置だけを、Transformへ書き戻す(Dynamic、syncRotation=false用)
    void SyncPositionFromActor(PxRigidActor* actor, Transform& transform) {
        PxTransform pose = actor->getGlobalPose();
        transform.position = { pose.p.x, pose.p.y, pose.p.z };
    }

    // PxRigidActorの現在の姿勢(位置・回転)を、Transformへ書き戻す(Dynamic用)。
    void SyncTransformFromActor(PxRigidActor* actor, Transform& transform) {
        PxTransform pose = actor->getGlobalPose();
        transform.position = { pose.p.x, pose.p.y, pose.p.z };

        XMVECTOR q = XMVectorSet(-pose.q.x, -pose.q.y, pose.q.z, pose.q.w);
        XMFLOAT4X4 mf;
        XMStoreFloat4x4(&mf, XMMatrixRotationQuaternion(q));

        // Transform::GetLocalMatrix()のXMMatrixRotationRollPitchYaw(pitch,yaw,roll)と
        // 対応する形で、行列からオイラー角(度数)を逆算する
        float sinPitch = -mf.m[2][1];
        sinPitch = (sinPitch < -1.f) ? -1.f : ((sinPitch > 1.f) ? 1.f : sinPitch);
        float pitch = asinf(sinPitch);
        float yaw = atan2f(mf.m[2][0], mf.m[2][2]);
        float roll = atan2f(mf.m[0][1], mf.m[1][1]);

        transform.rotate = {
            XMConvertToDegrees(pitch),
            XMConvertToDegrees(yaw),
            XMConvertToDegrees(roll)
        };
    }

    // Transformの現在位置を、PhysXのキネマティック目標姿勢として設定する(Kinematic用)。
    // 回転はここでは動かさず、作成時の向きのまま維持する(Player/Enemyのrotate.zは
    // 画面上のビルボード回転であって3D空間での向きではないため、そのまま3D回転には使わない)
    void SyncActorFromTransform(PxRigidActor* actor, const Transform& transform) {
        PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
        if (!dynamic) return;
        dynamic->setKinematicTarget(PxTransform(ToPxPos(transform), actor->getGlobalPose().q));
    }

    // ObjectPoolで再利用された直後など、スイープ(前回位置からの掃引判定)を避けて
    // 現在のTransformへ直接テレポートしたい時に使う
    void TeleportActor(PxRigidActor* actor, const Transform& transform) {
        actor->setGlobalPose(PxTransform(ToPxPos(transform), actor->getGlobalPose().q));
    }

    PxRigidActor* CreateActor(BodyType bodyType, const PxTransform& pose,
        const PxGeometry& geometry, PxMaterial* material, float density, bool isTrigger) {
        PxRigidActor* actor = nullptr;
        switch (bodyType) {
        case BodyType::Static:
            actor = PxCreateStatic(*Physics::GetPhysics(), pose, geometry, *material);
            break;
        case BodyType::Kinematic: {
            PxRigidDynamic* dynamic = PxCreateDynamic(*Physics::GetPhysics(), pose, geometry, *material, density);
            if (dynamic) dynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
            actor = dynamic;
            break;
        }
        case BodyType::Dynamic:
        default: {
            PxRigidDynamic* dynamic = PxCreateDynamic(*Physics::GetPhysics(), pose, geometry, *material, density);
            // ★ CCD(連続衝突判定)を有効にする。薄い箱(SquareRigidbodyComponentなど)や
            //   角同士の接触で、通常の判定だけだと押し返しが弱く貫通することがあるため
            if (dynamic) dynamic->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
            actor = dynamic;
            break;
        }
        }

        if (actor && isTrigger) {
            PxShape* shape = nullptr;
            actor->getShapes(&shape, 1);
            if (shape) {
                shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
                shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
            }
        }

        return actor;
    }

    // Collider可視化(デバッグ表示)用の共有メッシュ。単位立方体/単位球(どちらも一辺・直径1)を
    // 1つずつ作り、全RigidbodyComponentのDraw()で使い回す(実際の大きさはTransform.scaleで調整する)
    Mesh* GetGizmoCubeMesh() {
        static Mesh mesh;
        static bool initialized = false;
        if (!initialized) { mesh.Init(Mesh::CreateCube()); initialized = true; }
        return &mesh;
    }
    Mesh* GetGizmoSphereMesh() {
        static Mesh mesh;
        static bool initialized = false;
        if (!initialized) { mesh.Init(Mesh::CreateSphere()); initialized = true; }
        return &mesh;
    }
}

// ─────────────────────────── RigidbodyComponent(共通) ───────────────────────────

RigidbodyComponent::RigidbodyComponent(BodyType bodyType, float density,
    bool isTrigger, bool isStaticForPush)
    : bodyType(bodyType), density(density) {
    collider.isTrigger = isTrigger;
    collider.isStatic = isStaticForPush;
}

void RigidbodyComponent::InitWithGeometry(const PxGeometry& geometry) {
    Transform& t = GetOwner()->transform;
    PxTransform pose(ToPxPos(t), ToPxQuat(t.rotate));

    collider.owner = GetOwner();
    actor = CreateActor(bodyType, pose, geometry, Physics::GetDefaultMaterial(), density, collider.isTrigger);
    if (actor) {
        actor->userData = &collider;
        Physics::GetScene()->addActor(*actor);
        inScene = true;
    }
    else {
        OutputDebugStringA("★ RigidbodyComponent: アクター作成失敗\n");
    }
}

void RigidbodyComponent::Update(float dt) {
    if (!actor) return;
    if (bodyType == BodyType::Dynamic) {
        // 回転がロックされている間は、位置だけPhysXから反映する(回転は自分のコードに任せる)
        if (rotationFrozen) SyncPositionFromActor(actor, GetOwner()->transform);
        else SyncTransformFromActor(actor, GetOwner()->transform);
    }
    else if (bodyType == BodyType::Kinematic) {
        if (justActivated) {
            TeleportActor(actor, GetOwner()->transform);
            justActivated = false;
        }
        else {
            SyncActorFromTransform(actor, GetOwner()->transform);
        }
    }
}

void RigidbodyComponent::Draw() {
    if (!debugDrawEnabled || !actor) return;

    PxShape* shape = nullptr;
    actor->getShapes(&shape, 1);
    if (!shape) return;

    XMFLOAT3 gizmoSize{};
    Mesh* gizmoMesh = nullptr;

    switch (shape->getGeometryType()) {
    case PxGeometryType::eBOX: {
        PxBoxGeometry box;
        shape->getBoxGeometry(box);
        gizmoSize = { box.halfExtents.x * 2.f, box.halfExtents.y * 2.f, box.halfExtents.z * 2.f };
        gizmoMesh = GetGizmoCubeMesh();
        break;
    }
    case PxGeometryType::eSPHERE: {
        PxSphereGeometry sphere;
        shape->getSphereGeometry(sphere);
        gizmoSize = { sphere.radius * 2.f, sphere.radius * 2.f, sphere.radius * 2.f };
        gizmoMesh = GetGizmoSphereMesh();
        break;
    }
    default:
        return;
    }

    // 光源の影響を受けず、常に一定の色で見えるようにする(トリガーは黄、通常は緑)
    static const Light fullBright{ {0.f,-1.f,0.f}, {0.f,0.f,0.f}, {1.f,1.f,1.f} };
    XMFLOAT4 color = collider.isTrigger ? XMFLOAT4{ 1.f, 1.f, 0.f, 1.f } : XMFLOAT4{ 0.f, 1.f, 0.f, 1.f };

    // ★ 見た目(SpriteRenderer/MeshRenderer)のTransform.scaleとは独立して、
    //   実際にPhysXが使っている大きさで描画したいので、一時的にscaleだけ差し替えて描画する
    Transform& t = GetOwner()->transform;
    XMFLOAT3 originalScale = t.scale;
    t.scale = gizmoSize;
    gizmoMesh->Draw(t, Image::GetCamera3D(), color, fullBright, /*writeDepth*/ true, /*wireframe*/ true);
    t.scale = originalScale;
}

void RigidbodyComponent::Uninit() {
    if (actor) { actor->release(); actor = nullptr; }
}

void RigidbodyComponent::OnActiveChanged(bool active) {
    if (!actor) return;
    // ★ OnTriggerEnter2D/OnCollisionEnter2D経由(PhysXのコールバックの中)から呼ばれる
    //   可能性があるため、addActor/removeActorを直接呼ばずQueueSceneChangeで予約する
    if (active) {
        if (!inScene) {
            Physics::QueueSceneChange(actor, true);
            inScene = true;
        }
        justActivated = true;
    }
    else if (inScene) {
        Physics::QueueSceneChange(actor, false);
        inScene = false;
    }
}

void RigidbodyComponent::SetVelocity(XMFLOAT3 velocity) {
    if (!actor) return;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (!dynamic) return;  // Kinematic/Staticには速度という概念が無いので何もしない
    dynamic->setLinearVelocity(PxVec3(velocity.x, velocity.y, velocity.z));
}

XMFLOAT3 RigidbodyComponent::GetVelocity() const {
    if (!actor) return { 0.f, 0.f, 0.f };
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (!dynamic) return { 0.f, 0.f, 0.f };
    PxVec3 v = dynamic->getLinearVelocity();
    return { v.x, v.y, v.z };
}

void RigidbodyComponent::SetUseGravity(bool useGravity) {
    if (!actor) return;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (!dynamic) return;
    dynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !useGravity);
}

void RigidbodyComponent::SetFreezeRotation(bool freeze) {
    rotationFrozen = freeze;
    if (!actor) return;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (!dynamic) return;
    // 他の軸のロック状態(SetFreezePositionYなど)を消さないよう、今の設定を読んでから
    // X/Y/Z(角度方向)のビットだけ書き換える
    PxRigidDynamicLockFlags flags = dynamic->getRigidDynamicLockFlags();
    PxRigidDynamicLockFlags angular =
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y | PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
    dynamic->setRigidDynamicLockFlags(freeze ? (flags | angular) : (flags & ~angular));
}

void RigidbodyComponent::SetFreezePositionX(bool freeze) {
    if (!actor) return;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (!dynamic) return;
    PxRigidDynamicLockFlags flags = dynamic->getRigidDynamicLockFlags();
    dynamic->setRigidDynamicLockFlags(freeze
        ? (flags | PxRigidDynamicLockFlag::eLOCK_LINEAR_X)
        : (flags & ~PxRigidDynamicLockFlag::eLOCK_LINEAR_X));
}

void RigidbodyComponent::SetFreezePositionY(bool freeze) {
    if (!actor) return;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (!dynamic) return;
    PxRigidDynamicLockFlags flags = dynamic->getRigidDynamicLockFlags();
    dynamic->setRigidDynamicLockFlags(freeze
        ? (flags | PxRigidDynamicLockFlag::eLOCK_LINEAR_Y)
        : (flags & ~PxRigidDynamicLockFlag::eLOCK_LINEAR_Y));
}

void RigidbodyComponent::SetFreezePositionZ(bool freeze) {
    if (!actor) return;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (!dynamic) return;
    PxRigidDynamicLockFlags flags = dynamic->getRigidDynamicLockFlags();
    dynamic->setRigidDynamicLockFlags(freeze
        ? (flags | PxRigidDynamicLockFlag::eLOCK_LINEAR_Z)
        : (flags & ~PxRigidDynamicLockFlag::eLOCK_LINEAR_Z));
}

// ─────────────────────────── BoxRigidbodyComponent ───────────────────────────

BoxRigidbodyComponent::BoxRigidbodyComponent(BodyType bodyType, XMFLOAT3 size, float density,
    bool isTrigger, bool isStaticForPush)
    : RigidbodyComponent(bodyType, density, isTrigger, isStaticForPush), size(size) {
}

void BoxRigidbodyComponent::Init() {
    XMFLOAT3 s = (size.x > 0.f) ? size : GetOwner()->transform.scale;
    InitWithGeometry(PxBoxGeometry(s.x * 0.5f, s.y * 0.5f, s.z * 0.5f));
}

// ─────────────────────────── SphereRigidbodyComponent ───────────────────────────

SphereRigidbodyComponent::SphereRigidbodyComponent(BodyType bodyType, float radius, float density,
    bool isTrigger, bool isStaticForPush)
    : RigidbodyComponent(bodyType, density, isTrigger, isStaticForPush), radius(radius) {
}

void SphereRigidbodyComponent::Init() {
    float r = (radius > 0.f) ? radius : (GetOwner()->transform.scale.x * 0.5f);
    InitWithGeometry(PxSphereGeometry(r));
}

// ─────────────────────────── CircleRigidbodyComponent(2D用) ───────────────────────────

CircleRigidbodyComponent::CircleRigidbodyComponent(BodyType bodyType, float radius, float density,
    bool isTrigger, bool isStaticForPush)
    : RigidbodyComponent(bodyType, density, isTrigger, isStaticForPush), radius(radius) {
}

void CircleRigidbodyComponent::Init() {
    float r = (radius > 0.f) ? radius : (GetOwner()->transform.scale.x * 0.5f);
    InitWithGeometry(PxSphereGeometry(r));
}

// ─────────────────────────── SquareRigidbodyComponent(2D用) ───────────────────────────

SquareRigidbodyComponent::SquareRigidbodyComponent(BodyType bodyType, float size, float density,
    bool isTrigger, bool isStaticForPush)
    : RigidbodyComponent(bodyType, density, isTrigger, isStaticForPush), size(size) {
}

void SquareRigidbodyComponent::Init() {
    float s = (size > 0.f) ? size : GetOwner()->transform.scale.x;
    // 奥行き(Z)は1.0固定の薄い箱にする(2Dゲームでは厚みを意識しなくてよいように)
    InitWithGeometry(PxBoxGeometry(s * 0.5f, s * 0.5f, 0.5f));
}
