#pragma once
#include "../../../../Engine/Core/Component.h"

// 行き来する2つのシーン(A: 2Dの背景 / B: 3Dの舞台)のどちらか
enum class TransitionPage { A, B };

// Transitionデモの操作役。1〜4キーで演出を選んで、もう一方のシーンへ切り替える(行き来して見比べる)。
// 切り替え中(Time::IsPaused)は、ゲームの時間が止まる様子(経過時間・回る箱)を見せる。
// 全体の設定は変えない(演出はSceneManagerが自分で終わらせる)
class TransitionDemoController : public Component {
private:
    TransitionPage page;       // 今のシーン
    float elapsed = 0.f;       // このシーンに入ってからの時間(Updateのdtで数えるので、切り替え中は止まる)

    void Start(int type);      // type(0〜3)の演出で、もう一方のシーンへ切り替える

public:
    explicit TransitionDemoController(TransitionPage demoPage) : page(demoPage) {}

    void Update(float dt) override;
    void Draw() override;
};
