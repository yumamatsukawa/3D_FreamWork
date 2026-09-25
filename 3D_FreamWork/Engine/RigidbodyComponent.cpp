#include "RigidbodyComponent.h"
#include "Physics.h"
#include <windows.h>
#include <PxPhysicsAPI.h>

using namespace physx;
using namespace DirectX;

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
        default:
            actor = PxCreateDynamic(*Physics::GetPhysics(), pose, geometry, *material, density);
            break;
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

void RigidbodyComponent::SetFreezePositionY(bool freeze) {
    if (!actor) return;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (!dynamic) return;
    PxRigidDynamicLockFlags flags = dynamic->getRigidDynamicLockFlags();
    dynamic->setRigidDynamicLockFlags(freeze
        ? (flags | PxRigidDynamicLockFlag::eLOCK_LINEAR_Y)
        : (flags & ~PxRigidDynamicLockFlag::eLOCK_LINEAR_Y));
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
