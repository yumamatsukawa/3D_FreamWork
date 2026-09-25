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

// ─────────────────────────── BoxRigidbodyComponent ───────────────────────────

BoxRigidbodyComponent::BoxRigidbodyComponent(BodyType bodyType, XMFLOAT3 size, float density,
    bool isTrigger, bool isStaticForPush)
    : bodyType(bodyType), size(size), density(density) {
    collider.isTrigger = isTrigger;
    collider.isStatic = isStaticForPush;
}

void BoxRigidbodyComponent::Init() {
    Transform& t = GetOwner()->transform;
    XMFLOAT3 s = (size.x > 0.f) ? size : t.scale;

    PxTransform pose(ToPxPos(t), ToPxQuat(t.rotate));
    PxBoxGeometry box(s.x * 0.5f, s.y * 0.5f, s.z * 0.5f);

    collider.owner = GetOwner();
    actor = CreateActor(bodyType, pose, box, Physics::GetDefaultMaterial(), density, collider.isTrigger);
    if (actor) {
        actor->userData = &collider;
        Physics::GetScene()->addActor(*actor);
        inScene = true;
    }
    else {
        OutputDebugStringA("★ BoxRigidbodyComponent: アクター作成失敗\n");
    }
}

void BoxRigidbodyComponent::Update(float dt) {
    if (!actor) return;
    if (bodyType == BodyType::Dynamic) {
        SyncTransformFromActor(actor, GetOwner()->transform);
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

void BoxRigidbodyComponent::Uninit() {
    if (actor) { actor->release(); actor = nullptr; }
}

void BoxRigidbodyComponent::OnActiveChanged(bool active) {
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

// ─────────────────────────── SphereRigidbodyComponent ───────────────────────────

SphereRigidbodyComponent::SphereRigidbodyComponent(BodyType bodyType, float radius, float density,
    bool isTrigger, bool isStaticForPush)
    : bodyType(bodyType), radius(radius), density(density) {
    collider.isTrigger = isTrigger;
    collider.isStatic = isStaticForPush;
}

void SphereRigidbodyComponent::Init() {
    Transform& t = GetOwner()->transform;
    float r = (radius > 0.f) ? radius : (t.scale.x * 0.5f);

    PxTransform pose(ToPxPos(t), ToPxQuat(t.rotate));
    PxSphereGeometry sphere(r);

    collider.owner = GetOwner();
    actor = CreateActor(bodyType, pose, sphere, Physics::GetDefaultMaterial(), density, collider.isTrigger);
    if (actor) {
        actor->userData = &collider;
        Physics::GetScene()->addActor(*actor);
        inScene = true;
    }
    else {
        OutputDebugStringA("★ SphereRigidbodyComponent: アクター作成失敗\n");
    }
}

void SphereRigidbodyComponent::Update(float dt) {
    if (!actor) return;
    if (bodyType == BodyType::Dynamic) {
        SyncTransformFromActor(actor, GetOwner()->transform);
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

void SphereRigidbodyComponent::Uninit() {
    if (actor) { actor->release(); actor = nullptr; }
}

void SphereRigidbodyComponent::OnActiveChanged(bool active) {
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
