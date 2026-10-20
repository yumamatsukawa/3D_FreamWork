#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class GameObject;
class RigidbodyComponent;

// RigidbodySettingsデモの操作役。同じ箱を2つ(A=ふつう、B=設定を変えたもの)置き、同じ力で蹴って(SPACE)、動きの違いを比べる。
// 1〜5 で、Bに変える設定を選ぶ: 質量(SetMass)・空気抵抗(SetDrag)・重力(SetUseGravity)・回転固定(SetFreezeRotation)・位置固定(SetFreezePositionX/Y/Z。もう一度5で軸を変える)。
// 2D版は重力が無い世界なので、重力はBだけ「入れる」。3D版は重力がある世界なので、Bだけ「切る」
class RigidbodySettingsDemoController : public Component {
public:
    // Bに変える設定
    enum class Setting { Mass, Drag, Gravity, FreezeRotation, FreezePosition };

private:
    // 箱の剛体の設定の組(Aは常に既定、Bは選んだ設定だけ変える)
    struct State {
        float mass;
        float drag;
        bool gravity;
        bool freezeRotation;
        bool freezeX, freezeY, freezeZ;
    };

    // 箱1つ分の情報
    struct Box {
        GameObject* object = nullptr;
        RigidbodyComponent* rigidbody = nullptr;
        DirectX::XMFLOAT3 start{};     // 最初の位置(Rキーで戻す)
        float size = 0.f;              // 一辺の長さ
    };

    DemoMode mode;                     // 2D版か3D版か
    Box boxA;                          // A(ふつう)
    Box boxB;                          // B(設定を変えたもの)
    State stateA{};                    // Aの設定(いつも既定)
    State stateB{};                    // Bの設定
    Setting setting = Setting::Mass;   // Bに変えている設定
    int freezeAxis = 1;                // 位置固定の軸(0=X、1=Y、2=Z。3Dだけ Z がある)

    State MakeDefaultState() const;                       // 既定の設定(2Dは重力なし・Z固定、3Dは重力あり)
    void Apply(const Box& box, const State& state);       // 箱の剛体に、設定を全部入れる
    void ApplySetting();                                  // Bの設定を選んだ設定にして、両方を最初の位置へ戻す
    void Kick(const Box& box);                            // 箱を右上へ蹴る(Impulse)+回す(AddTorque)
    void Reset();                                         // 両方を最初の位置へ戻す
    void DrawBoxLabel(const Box& box, const wchar_t* name) const;   // 箱の上に名札を出す

public:
    explicit RigidbodySettingsDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 比べる2つの箱を、CreateRigidbodySettingsDemo()から渡してもらう(Init の後)
    void Setup(GameObject* normalBody, GameObject* changedBody);

    void Update(float dt) override;
    void Draw() override;
};
