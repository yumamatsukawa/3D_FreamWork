#pragma once
#include "Component.h"
#include "GameObject.h"
#include "Collider.h"

namespace physx { class PxRigidActor; }

// 剛体の種類
enum class BodyType {
    Dynamic,    // 重力や衝突で物理的に動く(PhysXの計算結果がTransformに反映される)
    Kinematic,  // 自分のコード(PlayerControllerなど)がTransformを動かす。
                // 他の動的な物体(Dynamic)を押すことはできるが、自分は重力や力の影響を受けない
    Static      // 完全に動かない(床・壁など)
};

// Box形状の3D剛体。PhysXのPxRigidActorを作り、毎フレーム同期する。
// Dynamicの時はPhysXの計算結果をTransformへ、Kinematicの時はTransformの値をPhysXへ反映する
// (どちらの場合も、Transformが「今どこにあるか」の単一の正解であり続ける)。
//
// 当たり判定のコールバック(OnTriggerEnter2D/OnCollisionEnter2Dなど)も、このComponentが
// 内部に持つCollider2Dを介して呼ばれる(Physics.cppのシミュレーションコールバックが呼ぶ)。
//
// ※ 2D・3DともTransform.position.yはY+が上方向で統一されているので、座標変換は不要
class BoxRigidbodyComponent : public Component {
private:
    physx::PxRigidActor* actor = nullptr;
    Collider2D collider;
    BodyType bodyType;
    DirectX::XMFLOAT3 size;
    float density;
    bool inScene = false;
    bool justActivated = false;  // ObjectPoolで再利用された直後、次のUpdate()でスイープを避けて直接テレポートする

public:
    // size: ワールド単位での全体のサイズ(半径ではない)。{0,0,0}(既定)ならOwnerのtransform.scaleを使う
    // density: 密度(質量=密度×体積として自動計算される)。Dynamic以外では無視される
    // isTrigger: trueだと、すり抜ける当たり判定(OnTriggerEnter2D等)になる
    // isStaticForPush: trueだと、Kinematic同士がぶつかった時に「押し戻されない」側になる
    //        (Enemyのように、自分は動かず相手だけ押し返したい時に使う。BodyType::Staticとは別の概念)
    explicit BoxRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        DirectX::XMFLOAT3 size = { 0.f, 0.f, 0.f }, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false);

    void Init() override;
    void Update(float dt) override;
    void Uninit() override;
    void OnActiveChanged(bool active) override;

    physx::PxRigidActor* GetActor() const { return actor; }
};

// Sphere形状の3D剛体。使い方はBoxRigidbodyComponentと同じ
class SphereRigidbodyComponent : public Component {
private:
    physx::PxRigidActor* actor = nullptr;
    Collider2D collider;
    BodyType bodyType;
    float radius;
    float density;
    bool inScene = false;
    bool justActivated = false;

public:
    // radius: 0(既定)ならOwnerのtransform.scale.xの半分を使う
    explicit SphereRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        float radius = 0.f, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false);

    void Init() override;
    void Update(float dt) override;
    void Uninit() override;
    void OnActiveChanged(bool active) override;

    physx::PxRigidActor* GetActor() const { return actor; }
};
