#pragma once
#include "../../../Engine/Core/Component.h"

class ModelRenderer;
class ModelAnimationComponent;

// FBXモデルの見本の動き。
// アニメーション付きのFBX(animation有り)なら、3秒ごとに "Spin" と "Bob" のクリップをCrossFadeで切り替える。
// アニメーション無し(animationがnullptr)なら、「Top」というパーツだけを手でゆっくり回す(GetNodeTransformの使用例)
class ModelDisplayController : public Component {
private:
    ModelRenderer* renderer;                       // 動かすモデル(同じオブジェクトに付いている)
    ModelAnimationComponent* animation;            // アニメーションの再生役(無ければnullptr)
    float timer = 0.f;                             // 今のクリップを再生し始めてからの秒数
    bool spinning = true;                          // 今のクリップが "Spin" か
    static constexpr float spinSpeed = 90.f;       // 手で回す時のTopパーツの回転の速さ(度/秒)
    static constexpr float switchInterval = 3.f;   // クリップを切り替える間隔(秒)
    static constexpr float fadeSeconds = 0.3f;     // 切り替えにかける秒数

public:
    ModelDisplayController(ModelRenderer* renderer, ModelAnimationComponent* animation)
        : renderer(renderer), animation(animation) {}

    void Update(float dt) override;
};