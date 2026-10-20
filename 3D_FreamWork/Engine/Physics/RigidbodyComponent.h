#pragma once
#include "../Core/Component.h"
#include "../Core/GameObject.h"
#include "Collider.h"
#include "Physics.h"
#include "../Render3D/Mesh.h"
#include <memory>

namespace physx {
    class PxRigidActor;
    class PxRigidDynamic;
    class PxGeometry;
    class PxHeightField;
}

class Terrain;

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
// 当たり判定のコールバック(OnTriggerEnter/OnCollisionEnterなど)も、このComponentが
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
    bool flat2D = false;         // 2D向け(Square/Circle/Capsule2D)かどうか。デバッグ表示で断面だけを描くのに使う
    CapsuleAxis capsuleAxis = CapsuleAxis::X;  // カプセルの長い向き(Capsule系のみ。X=PhysXの元の向き=回転なし)
    Mesh capsuleOutline;         // カプセルのデバッグ表示用の輪郭(大きさで形が変わるので、各自で持つ。初めて描く時に作る)
    bool capsuleOutlineReady = false;
    bool warnedNotDynamic = false;  // 「Dynamicではないので効きません」を出力したか(毎フレーム出さないように1回だけ)

    // Dynamicなら、PhysXのPxRigidDynamicを返す。違えば1回だけ出力ウィンドウに知らせてnullptrを返す
    physx::PxRigidDynamic* GetDynamicActor(const char* functionName);

    RigidbodyComponent(BodyType bodyType, float density,
        bool isTrigger, bool isStaticForPush);

    // 派生クラス(Box/Sphere)がInit()の中で、自分の形状を渡して呼ぶ。
    // アクター作成・シーンへの追加など、形状によらない共通処理をまとめて行う。
    // axis: カプセルだけ指定する。PhysXのカプセルはX軸に伸びるので、Y/Zを指定すると形を回して向きを変える
    void InitWithGeometry(const physx::PxGeometry& geometry, CapsuleAxis axis = CapsuleAxis::X);

public:
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
    void OnActiveChanged(bool active) override;

    // trueにすると、全てのRigidbodyComponentの当たり判定の形・大きさを、
    // 枠線(緑=通常の当たり判定、黄=すり抜けるトリガー)で描画する。
    // Box=立方体の12辺、Sphere=3方向の円、Square/Circle=Z軸方向で切った断面(四角/丸)、
    // Capsule=両端の半円+輪+側面の線、Capsule2D=断面(角の丸い四角)。
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

    // ─── 力・重さ・空気抵抗 ─────────────────────────
    // ""Dynamicの時だけ効く""(Kinematic/Staticでは何もせず、出力ウィンドウに1回だけ知らせる)
    // ロックした軸(SetFreezePositionX/Y/Z・SetFreezeRotation)の向きの力は0になる(力をかけてもその向きには動かない)

    // 力を加える。例: rigidbody->AddForce(away * 600.f, ForceMode::VelocityChange);
    // OnCollisionStayなどPhysXのコールバックの中で呼んでもよい(その場合はPhysXの計算の直後にまとめて加わる)
    void AddForce(DirectX::XMFLOAT3 force, ForceMode mode = ForceMode::Force);
    // 回す力を加える(SetFreezeRotation(true)の時は回らない)。コールバックの中で呼んでもよい
    void AddTorque(DirectX::XMFLOAT3 torque, ForceMode mode = ForceMode::Force);

    // 重さを変える(作った時は「密度×体積」で自動計算されている)。0以下は無視する。
    // ※PhysXのコールバック(OnCollisionStayなど)の中からは呼ばない(Init/Updateで呼ぶ)
    void SetMass(float mass);
    // 今の重さ(Static・作成失敗の時は0)。Impulseの強さを「速さ×重さ」で決める時などに使う
    float GetMass() const;

    // 空気抵抗(速度が自然に落ちる。0で落ちない、大きいほどすぐ止まる)。負の値は0にする。
    // 重力で落ちる速さにも効く。※PhysXのコールバックの中からは呼ばない(Init/Updateで呼ぶ)
    void SetDrag(float drag);

    // 剛体の種類(Dynamic/Kinematic/Static)
    BodyType GetBodyType() const { return bodyType; }

    // PhysXのAPIを直接触りたい時だけ使う(通常は上のメソッドで十分)
    physx::PxRigidActor* GetActor() const { return actor; }

private:
    static bool debugDrawEnabled;
};

// Box形状の3D剛体。詳しい説明はRigidbodyComponentのコメントを参照
// --------------------------- 引数 ----------------------------------
// size            : サイズ。質量の計算にも使われる。規定ならOwnerのtransform.scaleになる。
// density         : 密度(質量 = 密度×体積として自動計算される)。1.0fが規定。Dynamic以外では無視される
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter等)になる
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
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter等)になる
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
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter等)になる
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
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter等)になる
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

// Capsule形状(円柱の両端が半球)の3D剛体。立った人や敵に向いていて、箱と違い角で段差に引っかからず、坂道も滑らかに登れる。
// 詳しい説明はRigidbodyComponentのコメントを参照。
// ※立てたまま回転ロック無しのDynamicにすると、ぶつかった時に倒れて転がる。人・敵には SetFreezeRotation(true) を付けること
// --------------------------- 引数 ----------------------------------
// radius          : 半径。既定(0)ならOwnerのtransform.scale.xの半分を使う
// height          : 全体の高さ(両端の半球を含む。Unityと同じ)。既定(0)ならOwnerのtransform.scale.yを使う。
//                   2×半径より小さい時は、(PhysXは円柱部分が0だと無効な形のため)ほぼ球になる値にする
// density         : 密度(質量 = 密度×体積として自動計算される)。1.0fが規定。Dynamic以外では無視される
// isTrigger       : trueだと、すり抜ける当たり判定(OnTriggerEnter等)になる
// isStaticForPush : trueだと、Kinematic同士がぶつかった時に「押し戻されない」側になる
// axis            : 長い向き(X/Y/Z)。既定はY(立った形)
class CapsuleRigidbodyComponent : public RigidbodyComponent {
private:
    float radius;
    float height;
    CapsuleAxis axis;

public:
    explicit CapsuleRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        float radius = 0.f, float height = 0.f, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false,
        CapsuleAxis axis = CapsuleAxis::Y);

    void Init() override;
};

// 2D用のカプセルコライダー。中身はCapsuleRigidbodyComponentと同じだが、デバッグ表示は断面(角の丸い四角)だけを描く。
// Circle/Squareと同じく、2Dゲームで使う時は SetFreezePositionZ(true) と SetFreezeRotation(true) を付けること。
// 引数の意味もCapsuleRigidbodyComponentと同じ(axisは Y=縦長、X=横長。Zは2Dでは使わない)
class Capsule2DRigidbodyComponent : public RigidbodyComponent {
private:
    float radius;
    float height;
    CapsuleAxis axis;

public:
    explicit Capsule2DRigidbodyComponent(BodyType bodyType = BodyType::Dynamic,
        float radius = 0.f, float height = 0.f, float density = 1.0f,
        bool isTrigger = false, bool isStaticForPush = false,
        CapsuleAxis axis = CapsuleAxis::Y);

    void Init() override;
};

// 地形(Terrain)の起伏に合わせた3Dの当たり判定(PhysXの高さフィールド)。いつもStatic(動かない)。
// TerrainRendererと同じTerrainを渡すこと(見た目と当たり判定が同じ高さのデータから作られ、ずれない)。
// Physics::Raycast / OverlapSphere / OverlapBox も地形に当たる。
// 決まり:
// - オーナーのtransform.positionが、地形の左手前の角(X・Zが最小の角)で、Yが高さ0。TerrainRendererと同じ
// - 位置は作った時に決まる(Staticなので、後でオーナーを動かしても当たり判定は動かない)。回転・拡大縮小は使えない
// - 高さは16ビット整数に丸めて持つ(誤差は最大の高さ÷65534以下)
// - 地形の下側からは通り抜けることがある(面は上向きの1枚)。地形の外は何もない
// - デバッグ表示(SetDebugDrawEnabled)は、地形には線を描かない
// --------------------------- 引数 ----------------------------------
// terrain   : 高さのデータ(TerrainRendererに渡した物と同じ)
// isTrigger : PhysXの高さフィールドはトリガーにできないため、trueにしても効かない(出力ウィンドウに知らせて通常の当たり判定にする)
class TerrainRigidbodyComponent : public RigidbodyComponent {
private:
    std::shared_ptr<Terrain> terrain;       // 高さのデータ
    physx::PxHeightField* heightField = nullptr;   // PhysXの高さフィールド(アクターより後に解放する)

public:
    explicit TerrainRigidbodyComponent(const std::shared_ptr<Terrain>& terrain, bool isTrigger = false);

    void Init() override;
    void Uninit() override;
};
