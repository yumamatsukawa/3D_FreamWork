#pragma once
#include "Component.h"
#include "GameObject.h"
#include "Collider.h"

namespace physx {
    class PxRigidActor;
    class PxGeometry;
}

// 剛体の種類
enum class BodyType {
    Dynamic,    // 重力や衝突で物理的に動く(PhysXの計算結果がTransformに反映される)
    Kinematic,  // 自分のコードがTransformを動かす。他の動的な物体(Dynamic)を押すことはできるが、自分は重力や力の影響を受けない
    Static      // 完全に動かない(床・壁など)
};

// PhysXの剛体を持ち、毎フレーム同期する処理の共通部分。
//
// Dynamicの時はPhysXの計算結果をTransformへ、Kinematicの時はTransformの値をPhysXへ反映する
//
// 当たり判定のコールバック(OnTriggerEnter2D/OnCollisionEnter2Dなど)も、このComponentが
// 内部に持つCollider2Dを介して呼ばれる
//
// 形状ごとの違いは[ BoxRigidbodyComponent / SphereRigidbodyComponent ]が担う
// GetComponent<RigidbodyComponent>() で探せば、Ownerの形状がBoxでもSphereでも同じように扱える。
class RigidbodyComponent : public Component {
protected:
    physx::PxRigidActor* actor = nullptr;
    Collider2D collider;
    BodyType bodyType;
    float density;
    bool rotationFrozen = false; // SetFreezeRotation(true)の状態。trueの間はPhysXの回転をTransformへ反映しない
    bool inScene = false;
    bool justActivated = false;  // ObjectPoolで再利用された直後、次のUpdate()でスイープを避けて直接テレポートする
    bool flat2D = false;         // 2D向け(Square/Circle)かどうか。デバッグ表示で断面だけを描くのに使う

    RigidbodyComponent(BodyType bodyType, float density,
        bool isTrigger, bool isStaticForPush);

    // 派生クラス(Box/Sphere)がInit()の中で、自分の形状を渡して呼ぶ。
    // アクター作成・シーンへの追加など、形状によらない共通処理をまとめて行う
    void InitWithGeometry(const physx::PxGeometry& geometry);

public:
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
    void OnActiveChanged(bool active) override;

    // trueにすると、全てのRigidbodyComponentの当たり判定の形・大きさを、
    // 枠線(緑=通常の当たり判定、黄=すり抜けるトリガー)で描画する。
    // Box=立方体の12辺、Sphere=3方向の円、Square/Circle=Z軸方向で切った断面(四角/丸)。
    // 実際にPhysXが使っている形状(size/radius)をそのまま表示するので、
    // 見た目(SpriteRenderer/MeshRenderer)とズレていないか確認する時に使う
    static void SetDebugDrawEnabled(bool enabled) { debugDrawEnabled = enabled; }

    // 剛体に速度を設定する
    // ""Dynamicの時だけ意味がある""
    void SetVelocity(DirectX::XMFLOAT3 velocity);

    // 現在の速度を取得する(Kinematic/Staticでは常に{0,0,0}を返す)
    DirectX::XMFLOAT3 GetVelocity() const;

    // 重力の影響を受けるかどうか。false にすると、落下しなくなる(既定は物理的にtrue相当)。
    // ""Dynamicの時だけ意味がある""
    void SetUseGravity(bool useGravity);

    // trueにすると、ぶつかってもゴロゴロ回転しなくなる
    // ""Dynamicの時だけ意味がある""
    void SetFreezeRotation(bool freeze);

    // trueにすると、指定方向に押し出されなくなる
    // ""Dynamicの時だけ意味がある""
    void SetFreezePositionX(bool freeze);
    void SetFreezePositionY(bool freeze);
    void SetFreezePositionZ(bool freeze);

    // PhysXのAPIを直接触りたい時だけ使う(通常は上のメソッドで十分)
    physx::PxRigidActor* GetActor() const { return actor; }

private:
    static bool debugDrawEnabled;
};

// Box形状の3D剛体。詳しい説明はRigidbodyComponentのコメントを参照
// --------------------------- 引数 ----------------------------------
// size            : サイズ。質量の計算にも使われる。規定ならOwnerのtransform.scaleになる。
// density         : 密度(質量 = 密度×体積として自動計算される)。1.0fが規定。Dynamic以外では無視される
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter2D等)になる
// isStaticForPush : trueだと、Kinematic同士がぶつかった時に「押し戻されない」側になる
class BoxRigidbodyComponent : public RigidbodyComponent {
private:
    DirectX::XMFLOAT3 size;

public:
    explicit BoxRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        DirectX::XMFLOAT3 size = { 0.f, 0.f, 0.f }, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false);

    void Init() override;
};

// Sphere形状の3D剛体。詳しい説明はRigidbodyComponentのコメントを参照
// --------------------------- 引数 ----------------------------------
// radius          : 半径。既定ならOwnerのtransform.scale.xの半分を使う
// density         : 密度(質量 = 密度×体積として自動計算される)。1.0fが規定。Dynamic以外では無視される
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter2D等)になる
// isStaticForPush : trueだと、Kinematic同士がぶつかった時に「押し戻されない」側になる
class SphereRigidbodyComponent : public RigidbodyComponent {
private:
    float radius;

public:
    explicit SphereRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        float radius = 0.f, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false);

    void Init() override;
};

// 2D用の円形コライダー。中身はSphereRigidbodyComponentと同じ(PhysXに2D専用の形状は無いため)
// だが、トップダウン2Dゲームで使うと分かりやすいよう名前を分けてある。
// --------------------------- 引数 ----------------------------------
// radius          : 半径。既定ならOwnerのtransform.scale.xの半分を使う
// density         : 密度(質量 = 密度×体積として自動計算される)。1.0fが規定。Dynamic以外では無視される
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter2D等)になる
// isStaticForPush : trueだと、Kinematic同士がぶつかった時に「押し戻されない」側になる
class CircleRigidbodyComponent : public RigidbodyComponent {
private:
    float radius;

public:
    explicit CircleRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        float radius = 0.f, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false);

    void Init() override;
};

// 2D用の四角形コライダー。中身はBoxRigidbodyComponentと同じ(立方体)だが、
// 一辺の長さ(size)だけ指定すればよい分、2Dゲームでは扱いやすい。
// 2Dゲームで使う時は SetFreezePositionZ(true) で奥行き方向に動かないようにしておくこと
// (奥行きにも一辺と同じ厚みがあるが、Zを固定していれば見た目や動きには影響しない)。
// --------------------------- 引数 ----------------------------------
// size            : 一辺の長さ。既定(0)ならOwnerのtransform.scale.xを使う(奥行きも同じ長さになる)
// density         : 密度(質量 = 密度×体積として自動計算される)。1.0fが規定。Dynamic以外では無視される
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter2D等)になる
// isStaticForPush : trueだと、Kinematic同士がぶつかった時に「押し戻されない」側になる
class SquareRigidbodyComponent : public RigidbodyComponent {
private:
    float size;

public:
    explicit SquareRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        float size = 0.f, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false);

    void Init() override;
};
