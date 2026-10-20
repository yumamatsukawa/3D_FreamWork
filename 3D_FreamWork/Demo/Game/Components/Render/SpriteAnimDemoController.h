#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>
#include <string>
#include <vector>

class GameObject;
class SpriteAnimationComponent;

// SpriteAnimデモの操作役。SPACEで全員の再生・停止、1〜4キーで再生の速さを変える。
// ループするクリップ・1回だけのクリップ・始まりをずらしたクリップを並べて、IsFinished の様子を文字で出す
class SpriteAnimDemoController : public Component {
private:
    // アニメーションするスプライト1つ分
    struct Entry {
        GameObject* object;                // 持ち主
        SpriteAnimationComponent* anim;    // コマを送る部品
        std::string clip;                  // 再生するクリップの名前
        bool once;                         // 1回だけで止まるクリップか(IsFinishedを出す)
        std::wstring label;                // 近くに出す名前
        float labelOffset;                 // 名前を出す位置(スプライトの中心から画面で上へ。下へはマイナス)
    };

    DemoMode mode;                         // 2D版か3D版か
    std::vector<Entry> entries;            // アニメーションするスプライト
    bool playing = true;                   // 再生中か(falseは Stop した状態)
    int speedIndex = 1;                    // 今の速さ(speeds の何番目か)

public:
    explicit SpriteAnimDemoController(DemoMode demoMode) : mode(demoMode) {}

    // アニメーションするスプライトを足す(CreateSpriteAnimDemo()から)。クリップは作る側で登録して、Playもしておく
    void AddEntry(GameObject* object, SpriteAnimationComponent* anim, const std::string& clip, bool once, const std::wstring& label, float labelOffset);

    void Update(float dt) override;
    void Draw() override;
};
