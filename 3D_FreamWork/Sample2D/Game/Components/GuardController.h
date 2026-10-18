#pragma once
#include "../../../Engine/Core/Component.h"

class GameObject;
class VisionComponent;
class SpriteRenderer;

// 見張り役の動きを担当する(ゆっくり回りながら、視野に相手が入ると赤くなる。視野の判定はVisionComponentが担当)
class GuardController : public Component {
private:
    GameObject* target = nullptr;            // 見張る相手(Player)
    VisionComponent* vision = nullptr;       // 視野(見えるかどうかの判定)
    SpriteRenderer* sprite = nullptr;        // 見た目(見えた時に色を変える)

    static constexpr float turnSpeed = 40.0f;   // 回る速さ(1秒に何度。rotate.zを増やす)

public:
    // 見張る相手を、CreateGuard()から渡してもらう
    void Setup(GameObject* targetObject) { target = targetObject; }

    void Init() override;
    void Update(float dt) override;
};