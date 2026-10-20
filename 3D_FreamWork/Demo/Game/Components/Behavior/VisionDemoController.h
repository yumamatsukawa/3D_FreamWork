#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class GameObject;
class VisionComponent;
class RigidbodyComponent;

// Visionデモの操作役。見張り役(VisionComponent)の視野の設定を、キーで変えながら見せる。
// 扇形(角度と距離)の中にターゲットがいて、壁(タグ "Wall")にさえぎられていなければ、見える(CanSee)。
// 見張り役の正面は SetFacing で渡す(A・D で向き、SPACE で自動で回る)。W・S で視野の角度、1・2 で距離。
// 5 で壁のタグを外す(SetObstacleTag("") で壁を考えなくなる)。3D版は 4 で目の高さ(SetEyeOffset)を変えて、壁の上から見る。
// F1 で扇形の線(VisionComponent::SetDebugDrawEnabled。見えている時は赤、見えていない時は緑)。
// ターゲットは Dynamic なので、SetVelocity で動かす(3D版はカメラの向き基準)。
class VisionDemoController : public Component {
private:
    DemoMode mode;                                  // 2D版か3D版か
    GameObject* watcher = nullptr;                  // 見張り役
    GameObject* target = nullptr;                   // ターゲット
    GameObject* camera = nullptr;                   // 3D版: 向きの基準(DemoCameraController が付いたオブジェクト)
    VisionComponent* vision = nullptr;              // 見張り役の視野
    RigidbodyComponent* targetBody = nullptr;       // ターゲットの剛体
    DirectX::XMFLOAT3 targetStart{};                // ターゲットの出発点(Rで戻す場所)

    float facingAngle = 0.f;                        // 見張り役の正面の向き(ラジアン。0で+X、増えると平面の上で左回り)
    float fieldOfView = 90.f;                       // 視野の角度(度。W・S)
    float range = 800.f;                            // 視野の距離(1・2)
    float rangeStep = 100.f;                        // 距離を1回で変える量
    float targetSpeed = 250.f;                      // ターゲットの速さ
    bool spinning = false;                          // 自動で回るか(SPACE)
    bool wallsBlock = true;                         // 壁で視線をさえぎるか(5。false なら壁のタグを外す)
    bool eyeHigh = false;                           // 3D版: 目を高くするか(4)
    bool debugDraw = true;                          // 扇形の線を出すか(F1)

    // 今の結果(Update で調べて、Draw で出す)
    bool inCone = false;                            // ターゲットが扇形の中か
    bool seeing = false;                            // 見えるか(扇形の中で、壁にさえぎられていない)

    void MoveTarget();                              // 矢印キーでターゲットを動かす
    void ApplyVision();                             // 視野の設定を見張り役に渡す
    void Reset();                                   // ターゲットと向きを最初に戻す

public:
    explicit VisionDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 見張り役・ターゲット・カメラを、CreateVisionDemo()から渡してもらう(Init の後)。camera は3D版だけ
    void Setup(GameObject* watcherObject, GameObject* targetObject, GameObject* cameraObject);

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
