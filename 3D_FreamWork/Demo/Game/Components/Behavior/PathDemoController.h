#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../Common/DemoPlane.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class GameObject;
class ChaseComponent;
class NavGridComponent;
class PathFollowComponent;

// Pathデモの操作役。経路探索(NavGridComponent + PathFollowComponent)で壁を避けて歩く緑と、まっすぐ向かうだけで壁に止まる赤を比べる。
// クリックで、目的地の目印を動かす(2つとも、その目印へ向かう)。↑↓ で速さ、←→ で探し直す間隔(SetRepathInterval)。
// F1 で格子の表示(通れないマスの輪郭=赤い線と、今の経路=緑の線。NavGridComponent / PathFollowComponent の SetDebugDrawEnabled)。
// 動く物は Dynamic なので、R で戻す時は ResetDemoBody を使う
class PathDemoController : public Component {
private:
    DemoMode mode;                                  // 2D版か3D版か
    DemoField field;                                // 競技場(目印はこの中に収める)
    GameObject* pathAgent = nullptr;                // 緑: 経路を探して歩く
    GameObject* chaseAgent = nullptr;               // 赤: まっすぐ向かう
    GameObject* goal = nullptr;                     // 目的地の目印
    PathFollowComponent* follow = nullptr;          // 緑の動き
    ChaseComponent* chase = nullptr;                // 赤の動き
    NavGridComponent* navigation = nullptr;         // 格子
    DirectX::XMFLOAT3 pathStart{};                  // 緑の出発点(Rで戻す場所)
    DirectX::XMFLOAT3 chaseStart{};                 // 赤の出発点
    float speed = 180.f;                            // 速さ(↑↓)
    float speedStep = 30.f;                         // 速さを1回で変える量
    float repathInterval = 0.5f;                    // 探し直す間隔(秒。←→)
    bool gridVisible = true;                        // 格子と経路の線を出すか(F1)

    void ApplySettings();                           // 速さと探し直す間隔を渡す

public:
    explicit PathDemoController(DemoMode demoMode) : mode(demoMode), field(GetDemoField(demoMode)) {}

    // 動く物・目印・格子を、CreatePathDemo()から渡してもらう(Init の後)
    void Setup(GameObject* pathObject, GameObject* chaseObject, GameObject* goalObject, NavGridComponent* nav);

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
