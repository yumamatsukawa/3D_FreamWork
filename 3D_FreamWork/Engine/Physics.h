#pragma once

// PxPhysicsAPI.hは重いので、ポインタを扱うだけのここでは前方宣言だけしておく
namespace physx {
    class PxPhysics;
    class PxScene;
    class PxMaterial;
    class PxRigidActor;
}

// PhysXの初期化・シミュレーション更新・終了を管理するシングルトン的な窓口。
// Image/Audio/Textなどと同じ、名前空間+staticなグローバル状態というこのエンジンの流儀に合わせている。
namespace Physics {
    bool Init();
    void Update(float dt);
    void Uninit();

    // RigidbodyComponentなど、PxRigidActorを直接作りたい側から使う
    physx::PxPhysics* GetPhysics();
    physx::PxScene* GetScene();
    physx::PxMaterial* GetDefaultMaterial();

    // アクターをシーンに追加/削除したいことを予約する。
    // OnTriggerEnter2D/OnCollisionEnter2Dなど、PhysXのシミュレーションコールバックの中(または
    // そこから呼ばれる処理)からは、addActor/removeActorを直接呼んではいけない
    // (PhysXが「コールバック中のAPI書き込み禁止」という制約を持っているため)。
    // 代わりにこれを呼んでおくと、次のUpdate()のfetchResults()が終わった直後、
    // 安全なタイミングでまとめて実行される
    void QueueSceneChange(physx::PxRigidActor* actor, bool add);
}
