#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../Common/DemoPlane.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>
#include <deque>

class GameObject;
class ChaseComponent;
class AccelerateChaseComponent;
class HomingComponent;

// Homingデモの操作役。同じ目標(マウスの目印)を追う3つの動きを比べる:
// 青 ChaseComponent(一定の速さ)・橙 AccelerateChaseComponent(加速して行き過ぎる)・緑 HomingComponent(曲がる角度に限り)。
// 通った道を点で残して、動きの違い(まっすぐ・行き過ぎて戻る・大回り)を見えるようにする。白い線は、今進んでいる向き。
// 1・2・3 で入り切り、↑↓ で緑の曲がる速さ、←→ で橙の加速、R で最初の位置へ戻す。
// 動く物は Kinematic(Transformを直接動かす)なので、動いた量(位置の差)から向きを求める
class HomingDemoController : public Component {
private:
    static constexpr int agentCount = 3;                        // 動く物の数
    static constexpr size_t trailMax = 70;                      // 残す道の点の数

    DemoMode mode;                                              // 2D版か3D版か
    DemoField field;                                            // 競技場(目印はこの中に収める)
    GameObject* agents[agentCount]{};                           // 青・橙・緑の動く物
    GameObject* targetMarker = nullptr;                         // 目標の目印(マウスに付いてくる)
    AccelerateChaseComponent* accelerate = nullptr;             // 橙の動き
    HomingComponent* homing = nullptr;                          // 緑の動き
    Component* motions[agentCount]{};                           // 3つの動き(入り切りに使う)
    bool active[agentCount] = { true, true, true };             // 入り切り(1・2・3)
    DirectX::XMFLOAT3 startPositions[agentCount]{};             // 出発点
    DirectX::XMFLOAT3 startRotates[agentCount]{};               // 出発した時の向き
    DirectX::XMFLOAT3 previousPositions[agentCount]{};          // 前のフレームの位置(進んでいる向きを求める)
    DirectX::XMFLOAT3 directions[agentCount]{};                 // 今進んでいる向き(平面の上。止まっていれば長さ0)
    std::deque<DirectX::XMFLOAT3> trails[agentCount];           // 通った道(古い順)
    float trailTimer = 0.f;                                     // 次に道の点を足すまでの時間

    float turnSpeed = 120.f;                                    // 緑の曲がる速さ(度/秒。↑↓)
    float acceleration = 500.f;                                 // 橙の加速(←→)
    float accelerationStep = 100.f;                             // 加速を1回で変える量
    float unitScale = 1.f;                                      // 2Dを1とした、3Dの世界の大きさの比

    void ApplySettings();                                       // 曲がる速さと加速を、動きに渡す
    void Reset();                                               // 最初の位置・向きへ戻す

public:
    explicit HomingDemoController(DemoMode demoMode) : mode(demoMode), field(GetDemoField(demoMode)) {}

    // 動く物と目印を、CreateHomingDemo()から渡してもらう(Init の後)
    void Setup(GameObject* chaseObject, GameObject* accelerateObject, GameObject* homingObject, GameObject* targetObject);

    void Update(float dt) override;
    void Draw() override;
};
