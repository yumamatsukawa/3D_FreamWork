#pragma once
#include "../../../Engine/Component.h"

class GameObject;
class VisionComponent;
class MeshRenderer;
class LightComponent;

// 見張り役の動きを担当する(ゆっくり回りながら、視野に相手が入ると赤くなる。視野の判定はVisionComponent、懐中電灯の光はLightComponentが担当)
class GuardController : public Component {
private:
    GameObject* target = nullptr;            // 見張る相手(Player)
    VisionComponent* vision = nullptr;       // 視野(見えるかどうかの判定)
    MeshRenderer* renderer = nullptr;        // 見た目(見えた時に色を変える)
    LightComponent* torch = nullptr;         // 懐中電灯(視線の方向を照らす)

    static constexpr float turnSpeed = 40.0f;   // 回る速さ(1秒に何度。rotate.yを増やす)
    static constexpr float torchPitch = 12.0f;  // 懐中電灯を下に向ける角度(度。水平だと地面に当たる所が遠いため)

public:
    // 見張る相手を、CreateGuard()から渡してもらう
    void Setup(GameObject* targetObject) { target = targetObject; }

    void Init() override;
    void Update(float dt) override;
};