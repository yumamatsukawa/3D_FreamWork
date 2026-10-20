#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class GameObject;
class RigidbodyComponent;
class SpriteRenderer;
class MeshRenderer;
class TouchCounterController;

// Triggerデモの操作役。動かせる球(WASD・矢印)を、黄色い丸(トリガー=すり抜ける)と青い箱(ぶつかる)に近づけて、
// それぞれで呼ばれたコールバックの回数(Enter・Stay・Exit)を見せる。回数は、トリガーと箱に付けた TouchCounterController が数える。
// 球は SetVelocity で動かす(Dynamicなので、Transformは直接書き換えない)。
// 3D版は、カメラ(DemoCameraController)の向きを基準に動く
class TriggerDemoController : public Component {
private:
    DemoMode mode;                                    // 2D版か3D版か
    GameObject* mover = nullptr;                      // 動かせる球
    RigidbodyComponent* moverBody = nullptr;          // 球の剛体
    GameObject* zone = nullptr;                       // トリガー(黄色い丸)
    GameObject* solid = nullptr;                      // ぶつかる箱
    TouchCounterController* zoneCounter = nullptr;    // トリガーで呼ばれた回数を数える役
    TouchCounterController* solidCounter = nullptr;   // 箱で呼ばれた回数を数える役
    GameObject* camera = nullptr;                     // 3D版: 見る点のオブジェクト(向きの基準)

    void MoveMover();                                 // WASD・矢印で球を動かす
    void DrawCounts(GameObject* object, float halfHeight, const wchar_t* name, const wchar_t* counts) const;   // 物の上に名前と回数を出す

public:
    explicit TriggerDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 動かす球・トリガー・箱を、CreateTriggerDemo()から渡してもらう(Init の後)。camera は3D版だけ
    void Setup(GameObject* moverObject, GameObject* zoneObject, GameObject* solidObject, GameObject* cameraObject);

    void Update(float dt) override;
    void Draw() override;
};
