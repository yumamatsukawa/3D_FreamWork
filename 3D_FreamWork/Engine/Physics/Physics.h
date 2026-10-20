#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

// PxPhysicsAPI.hは重いので、ポインタを扱うだけのここでは前方宣言だけしておく
namespace physx {
    class PxPhysics;
    class PxScene;
    class PxMaterial;
    class PxRigidActor;
    class PxCooking;
}

class GameObject;

// Physics::Raycastで当たった物の情報
struct RaycastHit {
    GameObject* object = nullptr;        // 当たったオブジェクト
    DirectX::XMFLOAT3 point{};           // 当たった場所
    DirectX::XMFLOAT3 normal{};          // 当たった面の向き(長さ1)
    float distance = 0.f;                // 当たるまでの距離
};

// RigidbodyComponent::AddForce / AddTorque の力のかけ方(Unity・PhysXと同じ4種類)
enum class ForceMode {
    Force,          // 力(質量で割る、時間をかける): 押し続ける
    Impulse,        // 衝撃(質量で割る、一瞬): ノックバック
    Acceleration,   // 加速度(質量を無視、時間をかける)
    VelocityChange  // 速度の変化(質量を無視、一瞬): ジャンプ
};

// Physics::Raycast / OverlapSphere / OverlapBox で、トリガー(isTrigger=true)も探すか
enum class QueryTriggers {
    Ignore,     // トリガーを無視する(既定。視線の判定が弾などにさえぎられない)
    Include     // トリガーも探す(アイテムやゴールのような、トリガーそのものを探す時)
};

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

    // 形のデータ(高さフィールドなど)をPhysXが使える形に変換する「クッキング」の窓口(TerrainRigidbodyComponentが使う)。
    // 最初に呼ばれた時に作り、Uninit()で解放する。失敗したらnullptr
    physx::PxCooking* GetCooking();

    // アクターをシーンに追加/削除したいことを予約する。
    // OnTriggerEnter/OnCollisionEnterなど、PhysXのシミュレーションコールバックの中(または
    // そこから呼ばれる処理)からは、addActor/removeActorを直接呼んではいけない
    // (PhysXが「コールバック中のAPI書き込み禁止」という制約を持っているため)。
    // 代わりにこれを呼んでおくと、次のUpdate()のfetchResults()が終わった直後、
    // 安全なタイミングでまとめて実行される
    void QueueSceneChange(physx::PxRigidActor* actor, bool add);

    // QueueSceneChange()で予約した変更(とApplyForce()で預かっている力)のうち、指定したactorに関するものを取り消す。
    // RigidbodyComponent::Uninit()がactor->release()する直前に必ず呼ぶこと。
    // 呼ばずに解放すると、「シーンに追加/削除して」という予約だけが残り、次のUpdate()で
    // 既に解放済み(もう存在しない)アクターへアクセスしてクラッシュする
    // (SetActive(false)でプールに戻った直後にシーンが切り替わる、などの場合に起こりうる)
    void CancelQueuedSceneChange(physx::PxRigidActor* actor);

    // actorが入っている「トリガーに重なっている組み合わせ」(OnTriggerStayを毎フレーム呼ぶために覚えている物)を忘れる。
    // RigidbodyComponentが、SetActive(false)でシーンから外す時と、actor->release()する直前に必ず呼ぶこと。
    // 呼ばずに解放すると、次のUpdate()でOnTriggerStayを呼ぶ時に、解放済みのアクターへアクセスしてクラッシュする
    void ForgetTriggerPairs(physx::PxRigidActor* actor);

    // ─── 問い合わせ(「この線の先に何があるか」「この範囲に何がいるか」) ─────────────
    // 決まり:
    // - tagを指定すると、そのタグのオブジェクトだけを探す(""なら全部)
    // - トリガー(isTrigger=trueのすり抜ける当たり判定。弾など)は、QueryTriggers::Includeの時だけ探す
    //   (tagを指定してもトリガーは無視のまま。例: Physics::OverlapSphere(pos, 200.f, "Item", QueryTriggers::Include))
    // - 非アクティブ(ObjectPoolに戻った)・Destroy済みのオブジェクトは返さない
    // - 位置は、最後にPhysXが計算した時(このフレームの始め)のもの。このフレームにtransformを
    //   動かしたKinematicや、出したばかりのオブジェクトは、次のフレームから見つかる
    // - 読むだけなので、OnTriggerEnterなどPhysXのコールバックの中から呼んでもよい

    // originからdirectionの向きへ最大maxDistanceまで線を飛ばし、最初に当たった物をhitに入れる。
    // 当たればtrue。directionは長さ1でなくてもよい(中で長さ1にする。長さ0ならfalse)。
    // originが中に入っている物は当たらない(自分の中心から飛ばしても、自分には当たらない。Unityと同じ)
    bool Raycast(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT3 direction, float maxDistance,
        RaycastHit& hit, const std::string& tag = "", QueryTriggers triggers = QueryTriggers::Ignore);

    // 中心center・半径radiusの球に入っている物を全部探す(2Dでは円として使う)
    std::vector<GameObject*> OverlapSphere(DirectX::XMFLOAT3 center, float radius,
        const std::string& tag = "", QueryTriggers triggers = QueryTriggers::Ignore);

    // 中心center・大きさsize(transform.scaleと同じく、辺の長さ)の箱に入っている物を全部探す。箱は回転しない
    std::vector<GameObject*> OverlapBox(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 size,
        const std::string& tag = "", QueryTriggers triggers = QueryTriggers::Ignore);

    // ─── Engine内部用(RigidbodyComponent::AddForce / AddTorque だけが使う) ─────────────
    // actor(Dynamic)に力・回す力を加える。PhysXのコールバックの中(OnCollisionStayなど)で呼ばれた時は、
    // その場で加えずに預かり、fetchResults()が終わった直後にまとめて加える(コールバック中の書き込み禁止のため)
    void ApplyForce(physx::PxRigidActor* actor, DirectX::XMFLOAT3 value, ForceMode mode, bool torque);
}
