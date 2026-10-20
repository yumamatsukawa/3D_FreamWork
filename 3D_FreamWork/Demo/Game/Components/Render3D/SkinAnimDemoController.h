#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

class GameObject;
class ModelAnimationComponent;

// SkinAnimデモの操作役。箱のクリップを切り替え(Play / CrossFade)、3つのモデルの再生の速さを変える。
// 1回だけ曲がる腕は、3キーで作り直して最初から再生する(IsFinished で終わりが分かる)
class SkinAnimDemoController : public Component {
private:
    GameObject* box = nullptr;                          // 箱(TestAnim.fbx)
    ModelAnimationComponent* boxAnimation = nullptr;    // 箱のアニメーション
    GameObject* loopArm = nullptr;                      // ずっと曲がる腕
    GameObject* onceArm = nullptr;                      // 1回だけ曲がる腕(3キーで作り直す)

    std::string boxClip = "Spin";       // 箱が今のクリップ(CrossFadeの途中は、行き先のクリップ)
    float speed = 1.f;                  // 再生の速さ(上下キー)
    bool outlined = false;              // 枠線を付けているか(4キー)

    void ApplySpeed();                  // 速さを3つのアニメーションに入れる
    void ApplyOutline();                // 枠線を3つのモデルに入れる
    ModelAnimationComponent* GetOnceAnimation() const;   // 1回だけの腕のアニメーション

public:
    // 3つのモデルを、CreateSkinAnimDemo()から渡してもらう
    void Setup(GameObject* boxObject, ModelAnimationComponent* animation, GameObject* loopObject, GameObject* onceObject) {
        box = boxObject;
        boxAnimation = animation;
        loopArm = loopObject;
        onceArm = onceObject;
    }

    void Update(float dt) override;
    void Draw() override;
};
