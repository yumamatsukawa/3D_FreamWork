#include "RigidbodyComponent.h"
#include "Physics.h"
#include "../Render3D/Mesh.h"
#include "../Render/Image.h"
#include "../Render3D/Terrain.h"
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <PxPhysicsAPI.h>

using namespace physx;
using namespace DirectX;

bool RigidbodyComponent::debugDrawEnabled = false;

namespace {
    // Transform.rotate(度数のオイラー角)を、PhysXの回転(四元数)に変換する。
    // 位置(ToPxPos)を同じ数値のまま渡しているので、回転も同じ数値の四元数をそのまま渡せばよい
    // (DirectXの四元数とPhysXのPxQuatは、同じ(x,y,z,w)なら同じ座標を同じ場所へ回す。
    //  例: Y軸まわりにθ回すと、どちらも+Zが(sinθ, 0, cosθ)へ行く)。
    // ※以前はX/Y成分の符号を反転していたが、位置を反転していないので逆向きに回ってしまっていた
    PxQuat ToPxQuat(const XMFLOAT3& eulerDegrees) {
        XMMATRIX m = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(eulerDegrees.x),
            XMConvertToRadians(eulerDegrees.y),
            XMConvertToRadians(eulerDegrees.z));
        XMFLOAT4 qf;
        XMStoreFloat4(&qf, XMQuaternionRotationMatrix(m));
        return PxQuat(qf.x, qf.y, qf.z, qf.w);
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

        // ToPxQuatと同じく、四元数は同じ数値のまま使う
        XMVECTOR q = XMVectorSet(pose.q.x, pose.q.y, pose.q.z, pose.q.w);
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
    // 回転はここでは動かさず、作成時の向きのまま維持する(例: Sample2Dのrotate.zは
    // スプライトの見た目の向きに使っているもので、当たり判定の向きにまで反映したくないため)
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

    // Collider可視化(デバッグ表示)用の、輪郭だけの共有メッシュ(線)。形ごとに1つずつ作り、
    // 全RigidbodyComponentのDraw()で使い回す(実際の大きさはTransform.scaleで調整する)
    enum class Outline { Cube, Square, Sphere, Circle };

    Mesh* GetOutlineMesh(Outline type) {
        static Mesh cube, square, sphere, circle;
        static bool initialized = false;
        if (!initialized) {
            cube.Init(Mesh::CreateCubeOutline(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            square.Init(Mesh::CreateSquareOutline(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            sphere.Init(Mesh::CreateSphereOutline(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            circle.Init(Mesh::CreateCircleOutline(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            initialized = true;
        }
        switch (type) {
        case Outline::Cube:   return &cube;
        case Outline::Square: return &square;
        case Outline::Sphere: return &sphere;
        default:              return &circle;
        }
    }
}

// ─────────────────────────── RigidbodyComponent(共通) ───────────────────────────

RigidbodyComponent::RigidbodyComponent(BodyType bodyType, float density,
    bool isTrigger, bool isStaticForPush)
    : bodyType(bodyType), density(density) {
    collider.isTrigger = isTrigger;
    collider.isStatic = isStaticForPush;
}

void RigidbodyComponent::InitWithGeometry(const PxGeometry& geometry, CapsuleAxis axis) {
    Transform& t = GetOwner()->transform;
    PxTransform pose(ToPxPos(t), ToPxQuat(t.rotate));

    collider.owner = GetOwner();
    capsuleAxis = axis;
    actor = CreateActor(bodyType, pose, geometry, Physics::GetDefaultMaterial(), density, collider.isTrigger);
    if (actor && axis != CapsuleAxis::X) {
        // PhysXのカプセルはX軸に伸びるので、形(shape)だけを回して向きを変える(アクター自身の回転は変えない)。
        // X→Y: Z軸まわりに+90度、X→Z: Y軸まわりに-90度
        PxShape* shape = nullptr;
        actor->getShapes(&shape, 1);
        if (shape) {
            PxQuat shapeRotation = (axis == CapsuleAxis::Y)
                ? PxQuat(PxHalfPi, PxVec3(0.f, 0.f, 1.f))
                : PxQuat(-PxHalfPi, PxVec3(0.f, 1.f, 0.f));
            shape->setLocalPose(PxTransform(shapeRotation));
            // 形を回したので、質量の中心・回りにくさ(慣性)を計算し直す
            if (PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>()) {
                PxRigidBodyExt::updateMassAndInertia(*dynamic, density);
            }
        }
    }
    if (actor) {
        actor->userData = &collider;
        Physics::GetScene()->addActor(*actor);
        inScene = true;
        // ★ AddComponentした後でtransform.positionを設定し直すことはよくある
        //   (ObjectPoolで新しく作った弾・敵に、Rentの後で出現位置を入れる場合など)。
        //   最初のUpdate()で今のTransformへ直接テレポートさせて、それに追従させる
        //   (しないと、Dynamicは作成時の位置に引き戻され、Kinematicは作成時の位置から
        //    スイープ扱いで移動して、途中にいる物に誤って当たってしまう)
        justActivated = true;
    }
    else {
        OutputDebugStringA("★ RigidbodyComponent: アクター作成失敗\n");
    }
}

void RigidbodyComponent::Update(float dt) {
    if (!actor) return;
    if (bodyType == BodyType::Dynamic) {
        if (justActivated) {
            // 作成直後・ObjectPoolで再利用された直後: Transform側(出現位置)が正しいので、
            // PhysX側をそこへテレポートさせ、前回使った時の速度も消しておく
            // (しないと、前回やられた場所に引き戻され、その時の速度で動き出してしまう)
            TeleportActor(actor, GetOwner()->transform);
            if (PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>()) {
                dynamic->setLinearVelocity(PxVec3(0.f), false);
                dynamic->setAngularVelocity(PxVec3(0.f), false);
            }
            justActivated = false;
            return;
        }
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

    // Box/Sphereは立体の輪郭(立方体の12辺/3方向の円)、2D向けのSquare/Circleは
    // Z軸方向で切った断面(四角/丸)だけを描く。断面は奥行きが無いのでscale.zは1でよい
    switch (shape->getGeometryType()) {
    case PxGeometryType::eBOX: {
        PxBoxGeometry box;
        shape->getBoxGeometry(box);
        gizmoSize = { box.halfExtents.x * 2.f, box.halfExtents.y * 2.f, flat2D ? 1.f : box.halfExtents.z * 2.f };
        gizmoMesh = GetOutlineMesh(flat2D ? Outline::Square : Outline::Cube);
        break;
    }
    case PxGeometryType::eSPHERE: {
        PxSphereGeometry sphere;
        shape->getSphereGeometry(sphere);
        float d = sphere.radius * 2.f;
        gizmoSize = { d, d, flat2D ? 1.f : d };
        gizmoMesh = GetOutlineMesh(flat2D ? Outline::Circle : Outline::Sphere);
        break;
    }
    case PxGeometryType::eCAPSULE: {
        // カプセルは大きさで形が変わるので、大きさ1の輪郭をscaleで伸ばせない。実際の大きさの輪郭を自分で持つ
        PxCapsuleGeometry capsule;
        shape->getCapsuleGeometry(capsule);
        if (!capsuleOutlineReady) {
            capsuleOutlineReady = capsuleOutline.Init(
                Mesh::CreateCapsuleOutline(capsule.radius, capsule.halfHeight, capsuleAxis, 24, flat2D),
                D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        }
        if (!capsuleOutlineReady) return;
        gizmoSize = { 1.f, 1.f, 1.f };
        gizmoMesh = &capsuleOutline;
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
    // 2D向け(Square/Circle。断面だけの枠)は、2Dスプライトと同じ2D(正射影)で描く(zで位置がずれず、スプライトの上に重なる)。Box/Sphere/Capsuleは3Dカメラの透視
    if (flat2D) gizmoMesh->DrawLines2D(t, Image::GetCamera(), color, fullBright);
    else gizmoMesh->Draw(t, Image::GetCamera3D(), color, fullBright);
    t.scale = originalScale;
}

void RigidbodyComponent::Uninit() {
    if (capsuleOutlineReady) {
        capsuleOutline.Uninit();
        capsuleOutlineReady = false;
    }
    if (actor) {
        // ★ SetActive(false)でプールに戻った直後にシーンが切り替わる、といった場合、
        //   「シーンから削除して」という予約(QueueSceneChange)がまだ消化されずに
        //   残っていることがある。先にそれを取り消してからでないと、次のPhysics::Update()で
        //   既に解放済み(もう存在しない)アクターへアクセスしてクラッシュする
        Physics::CancelQueuedSceneChange(actor);
        Physics::ForgetTriggerPairs(actor);   // 同じ理由で、OnTriggerStay用に覚えている組み合わせも忘れる
        actor->release();
        actor = nullptr;
    }
}

void RigidbodyComponent::OnActiveChanged(bool active) {
    if (!actor) return;
    // ★ OnTriggerEnter/OnCollisionEnter経由(PhysXのコールバックの中)から呼ばれる
    //   可能性があるため、addActor/removeActorを直接呼ばずQueueSceneChangeで予約する
    if (active) {
        if (!inScene) {
            Physics::QueueSceneChange(actor, true);
            inScene = true;
        }
        justActivated = true;
    }
    else {
        // プールに戻ったら、トリガーに重なっている組み合わせを忘れる(OnTriggerStayを止める。
        // また使われた時は、重なっていればOnTriggerEnterからやり直しになる)
        Physics::ForgetTriggerPairs(actor);
        if (inScene) {
            Physics::QueueSceneChange(actor, false);
            inScene = false;
        }
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

// ─── 力・重さ・空気抵抗 ─────────────────────────
PxRigidDynamic* RigidbodyComponent::GetDynamicActor(const char* functionName) {
    if (!actor) return nullptr;
    if (bodyType != BodyType::Dynamic) {
        // 毎フレーム呼ばれても出力ウィンドウが埋まらないように、1回だけ知らせる
        if (!warnedNotDynamic) {
            char message[256];
            sprintf_s(message, "★ RigidbodyComponent::%s: Dynamicではないので効きません(%s)\n",
                functionName, GetOwner()->GetName().c_str());
            OutputDebugStringA(message);
            warnedNotDynamic = true;
        }
        return nullptr;
    }
    return actor->is<PxRigidDynamic>();
}

void RigidbodyComponent::AddForce(XMFLOAT3 force, ForceMode mode) {
    if (!GetDynamicActor("AddForce")) return;
    if (!inScene) return;   // 非アクティブ(プールに戻っている)間は加えない
    Physics::ApplyForce(actor, force, mode, false);
}

void RigidbodyComponent::AddTorque(XMFLOAT3 torque, ForceMode mode) {
    if (!GetDynamicActor("AddTorque")) return;
    if (!inScene) return;
    Physics::ApplyForce(actor, torque, mode, true);
}

void RigidbodyComponent::SetMass(float mass) {
    PxRigidDynamic* dynamic = GetDynamicActor("SetMass");
    if (!dynamic || mass <= 0.f) return;
    // 形に合わせて回りにくさ(慣性)も一緒に計算し直す
    PxRigidBodyExt::setMassAndUpdateInertia(*dynamic, mass);
}

float RigidbodyComponent::GetMass() const {
    if (!actor) return 0.f;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    return dynamic ? dynamic->getMass() : 0.f;
}

void RigidbodyComponent::SetDrag(float drag) {
    PxRigidDynamic* dynamic = GetDynamicActor("SetDrag");
    if (!dynamic) return;
    dynamic->setLinearDamping(drag > 0.f ? drag : 0.f);
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
    flat2D = true;
}

void CircleRigidbodyComponent::Init() {
    float r = (radius > 0.f) ? radius : (GetOwner()->transform.scale.x * 0.5f);
    InitWithGeometry(PxSphereGeometry(r));
}

// ─────────────────────────── SquareRigidbodyComponent(2D用) ───────────────────────────

SquareRigidbodyComponent::SquareRigidbodyComponent(BodyType bodyType, float size, float density,
    bool isTrigger, bool isStaticForPush)
    : RigidbodyComponent(bodyType, density, isTrigger, isStaticForPush), size(size) {
    flat2D = true;
}

void SquareRigidbodyComponent::Init() {
    float s = (size > 0.f) ? size : GetOwner()->transform.scale.x;
    // ★ 奥行き(Z)も一辺と同じ長さにする(実質的には立方体)。以前は1.0の薄い箱にしていたが、
    //   PhysXは紙のように薄い形状同士の角の接触が苦手で、角同士がかすめるとすり抜けることがあった。
    //   2Dゲームでは SetFreezePositionZ(true) で奥行き方向に動かないようにしておけば、
    //   厚みがあっても見た目や動きには影響しない
    InitWithGeometry(PxBoxGeometry(s * 0.5f, s * 0.5f, s * 0.5f));
}

// ─────────────────────────── CapsuleRigidbodyComponent ───────────────────────────

namespace {
    // 半径と全体の高さから、PhysXのカプセルの形を作る。
    // PhysXは radius > 0 かつ halfHeight(円柱部分の長さの半分) > 0 でないと無効な形なので、
    // 高さが足りない時は、ほぼ球になるよう円柱部分に小さな長さを足す
    PxCapsuleGeometry MakeCapsuleGeometry(float radius, float height) {
        if (radius < 0.001f) radius = 0.001f;
        float halfHeight = height * 0.5f - radius;
        float minHalfHeight = radius * 0.01f;
        if (halfHeight < minHalfHeight) halfHeight = minHalfHeight;
        return PxCapsuleGeometry(radius, halfHeight);
    }
}

CapsuleRigidbodyComponent::CapsuleRigidbodyComponent(BodyType bodyType, float radius, float height,
    float density, bool isTrigger, bool isStaticForPush, CapsuleAxis axis)
    : RigidbodyComponent(bodyType, density, isTrigger, isStaticForPush),
      radius(radius), height(height), axis(axis) {
}

void CapsuleRigidbodyComponent::Init() {
    const Transform& t = GetOwner()->transform;
    float r = (radius > 0.f) ? radius : (t.scale.x * 0.5f);
    float h = (height > 0.f) ? height : t.scale.y;
    InitWithGeometry(MakeCapsuleGeometry(r, h), axis);
}

// ─────────────────────────── Capsule2DRigidbodyComponent(2D用) ───────────────────────────

Capsule2DRigidbodyComponent::Capsule2DRigidbodyComponent(BodyType bodyType, float radius, float height,
    float density, bool isTrigger, bool isStaticForPush, CapsuleAxis axis)
    : RigidbodyComponent(bodyType, density, isTrigger, isStaticForPush),
      radius(radius), height(height), axis(axis) {
    flat2D = true;
}

void Capsule2DRigidbodyComponent::Init() {
    const Transform& t = GetOwner()->transform;
    float r = (radius > 0.f) ? radius : (t.scale.x * 0.5f);
    float h = (height > 0.f) ? height : t.scale.y;
    InitWithGeometry(MakeCapsuleGeometry(r, h), axis);
}

// ─────────────────────────── TerrainRigidbodyComponent ───────────────────────────

TerrainRigidbodyComponent::TerrainRigidbodyComponent(const std::shared_ptr<Terrain>& terrain, bool isTrigger)
    : RigidbodyComponent(BodyType::Static, 1.0f, false, false), terrain(terrain) {
    if (isTrigger) OutputDebugStringA("★ TerrainRigidbodyComponent: 高さフィールドはトリガーにできません(通常の当たり判定にしました)\n");
}

void TerrainRigidbodyComponent::Init() {
    if (!terrain) { OutputDebugStringA("★ TerrainRigidbodyComponent: terrain が nullptr です\n"); return; }
    PxCooking* cooking = Physics::GetCooking();
    if (!cooking) return;

    // 高さを16ビットの整数にする: 実際の高さ = 整数 x heightScale。いちばん高い所が32767になる大きさにする
    const std::vector<float>& heights = terrain->GetHeights();
    float maxHeight = 0.f;
    for (float h : heights) if (h > maxHeight) maxHeight = h;
    float heightScale = maxHeight / 32767.f;
    if (heightScale < 0.001f) heightScale = 0.001f;   // 平らな地形でも、PhysXが受け付ける大きさにする

    // サンプルの並び = 行(X) x 列の数 + 列(Z)。Terrain::GetHeights() と同じ並び。tessFlag を立てると、
    // サンプル (x, z) のマスは (x, z) → (x+1, z+1) に対角線が走る(Terrainの三角形と同じ)
    PxU32 rows = (PxU32)terrain->GetVertexCountX();
    PxU32 columns = (PxU32)terrain->GetVertexCountZ();
    std::vector<PxHeightFieldSample> samples((size_t)rows * columns);
    for (size_t i = 0; i < samples.size(); i++) {
        PxHeightFieldSample& sample = samples[i];
        sample.height = (PxI16)lroundf(heights[i] / heightScale);
        sample.materialIndex0 = PxBitAndByte(0, true);   // 材質0 + tessFlag
        sample.materialIndex1 = PxBitAndByte(0, false);
    }

    PxHeightFieldDesc desc;
    desc.nbRows = rows;
    desc.nbColumns = columns;
    desc.format = PxHeightFieldFormat::eS16_TM;
    desc.samples.data = samples.data();
    desc.samples.stride = sizeof(PxHeightFieldSample);
    heightField = cooking->createHeightField(desc, Physics::GetPhysics()->getPhysicsInsertionCallback());
    if (!heightField) { OutputDebugStringA("★ TerrainRigidbodyComponent: 高さフィールドの作成失敗\n"); return; }

    // 行=X方向のマスの長さ、列=Z方向のマスの長さ
    PxHeightFieldGeometry geometry(heightField, PxMeshGeometryFlags(), heightScale,
        terrain->GetCellSizeX(), terrain->GetCellSizeZ());
    InitWithGeometry(geometry);
}

void TerrainRigidbodyComponent::Uninit() {
    RigidbodyComponent::Uninit();   // アクターを先に解放してから、高さフィールドを解放する
    if (heightField) {
        heightField->release();
        heightField = nullptr;
    }
}
