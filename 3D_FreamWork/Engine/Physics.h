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

    // QueueSceneChange()で予約した変更のうち、指定したactorに関するものを取り消す。
    // RigidbodyComponent::Uninit()がactor->release()する直前に必ず呼ぶこと。
    // 呼ばずに解放すると、「シーンに追加/削除して」という予約だけが残り、次のUpdate()で
    // 既に解放済み(もう存在しない)アクターへアクセスしてクラッシュする
    // (SetActive(false)でプールに戻った直後にシーンが切り替わる、などの場合に起こりうる)
    void CancelQueuedSceneChange(physx::PxRigidActor* actor);
}
