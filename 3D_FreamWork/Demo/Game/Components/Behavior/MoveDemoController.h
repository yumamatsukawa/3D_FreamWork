#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../Common/DemoPlane.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class GameObject;
class ChaseComponent;

// Moveデモの操作役。ChaseComponent(MoveComponent の仲間)を付けた2つの動く物を見せる。
// 青: SetTarget で、マウスに付いてくる目印を追いかける。橙: SetTargetPosition で、クリックした場所へ向かう。
// ↑↓ で速さ(SetSpeed)、←→ で止まる距離(SetStopDistance)を変える。目印の周りの円が、止まる距離。
// 壁にぶつかると、まっすぐ進むだけの動きはそこで止まる(避けて歩くには Path デモの経路探索を使う)。
// 動く物は Dynamic なので、R で戻す時は ResetDemoBody を使う
class MoveDemoController : public Component {
private:
    DemoMode mode;                                   // 2D版か3D版か
    DemoField field;                                 // 競技場(目印はこの中に収める)
    GameObject* followAgent = nullptr;               // 青: 目標を追う
    GameObject* pointAgent = nullptr;                // 橙: 場所へ向かう
    GameObject* targetMarker = nullptr;              // 目標の目印(マウスに付いてくる)
    GameObject* destMarker = nullptr;                // 目的地の目印(クリックで動く)
    ChaseComponent* follow = nullptr;                // 青の動き
    ChaseComponent* point = nullptr;                 // 橙の動き
    DirectX::XMFLOAT3 followStart{};                 // 青の出発点(Rで戻す場所)
    DirectX::XMFLOAT3 pointStart{};                  // 橙の出発点
    float speed = 200.f;                             // 速さ(↑↓)
    float stopDistance = 60.f;                       // 止まる距離(←→)
    float speedStep = 50.f;                          // 速さを1回で変える量
    float stopStep = 20.f;                           // 止まる距離を1回で変える量

    void ApplySettings();                            // 速さと止まる距離を、2つの動きに渡す
    void Reset();                                    // 動く物を出発点へ戻す

public:
    explicit MoveDemoController(DemoMode demoMode) : mode(demoMode), field(GetDemoField(demoMode)) {}

    // 動く物と目印を、CreateMoveDemo()から渡してもらう(Init の後)
    void Setup(GameObject* followObject, GameObject* pointObject, GameObject* targetObject, GameObject* destObject);

    void Update(float dt) override;
    void Draw() override;
};
