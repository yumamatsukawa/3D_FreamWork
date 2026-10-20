#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Physics/Physics.h"
#include <DirectXMath.h>

class GameObject;
class RigidbodyComponent;

// Forceデモの操作役。質量が違う2つの箱(軽い箱・重い箱)を、同じ大きさの力で押して、
// 4種類の力(ForceMode: Force / Impulse / Acceleration / VelocityChange)と、SetVelocity(速さを上書き)・AddTorque(回す)の違いを見せる。
// 1〜4 で力の種類、SPACE で力をかける(Force・Accelerationは押している間ずっと、Impulse・VelocityChangeは押すたびに1回)。
// 箱は横にだけ動くよう、Y・Zを固定してある(力の違いだけを見るため)。3D版は重力も切ってある
class ForceDemoController : public Component {
private:
    // 箱1つ分の情報
    struct Box {
        GameObject* object = nullptr;
        RigidbodyComponent* rigidbody = nullptr;
        DirectX::XMFLOAT3 start{};     // 最初の位置(Rキーで戻す)
        float size = 0.f;              // 一辺の長さ(名札と回す力の計算に使う)
        float mass = 0.f;              // 質量(SetMassで決めた値)
    };

    DemoMode mode;                     // 2D版か3D版か
    Box light;                         // 軽い箱
    Box heavy;                         // 重い箱
    ForceMode forceMode = ForceMode::Force;   // 今の力の種類(1〜4キー)

    void Push(const Box& box);         // 今の力の種類で、箱を右へ押す
    void SetupBox(Box& box, GameObject* object, float mass);   // 質量・空気抵抗・固定を決める
    void Reset();                      // 箱を最初の位置へ戻す
    void DrawBoxLabel(const Box& box, const wchar_t* name) const;   // 箱の上に、名前・質量・速さを出す

public:
    explicit ForceDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 動かす2つの箱を、CreateForceDemo()から渡してもらう(Init の後)
    void Setup(GameObject* lightBody, GameObject* heavyBody);

    void Update(float dt) override;
    void Draw() override;
};
