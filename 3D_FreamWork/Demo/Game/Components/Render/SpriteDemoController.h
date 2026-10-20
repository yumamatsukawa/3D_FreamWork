#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Render/Sprite.h"
#include <DirectXMath.h>
#include <string>
#include <vector>

class GameObject;
class SpriteRenderer;

// Spriteデモの操作役。1〜4キーで、スプライトの見た目(コマ・色・大きさ・回転)を変える動きの入り切りをする。
// 2D版は5キーで、重ねた2つの描き方をUI → 2D → 3Dの順に切り替えて違いを見せる(UIは作った順で必ず最前面・大きさは変わらない、2Dはzが手前ほど手前・大きさは変わらない、3Dはzで大きさが変わる)。3D版はビルボードの向きの違いを、視点を回して見せる
class SpriteDemoController : public Component {
public:
    // 見た目を変える動きの種類(ビットで、スプライトごとにどれを受けるかを決める)
    enum Effect { Frame = 1, Color = 2, Size = 4, Rotate = 8 };

private:
    static constexpr int effectCount = 4;   // 動きの種類の数

    // 動かすスプライト1つ分
    struct Item {
        GameObject* object;                 // 持ち主
        SpriteRenderer* sprite;             // 見た目
        int effects;                        // 受ける動き(Effectのビットの組み合わせ)
        DirectX::XMFLOAT3 baseScale;        // 動かす前の大きさ
        std::wstring label;                 // 近くに出す名前
    };

    DemoMode mode;                          // 2D版か3D版か
    std::vector<Item> items;                // 動かすスプライト
    bool enabled[effectCount] = { true, true, true, true };   // 動きが入っているか(1〜4キー)
    float timers[effectCount] = {};         // 動きごとの経過時間(入っている間だけ進む)
    SpriteRenderer* nearSprite = nullptr;   // 2D: 重ねて見せる手前(z=-100)のスプライト
    SpriteRenderer* farSprite = nullptr;    // 2D: 重ねて見せる奥(z=+100)のスプライト
    SpriteMode pairMode = SpriteMode::UI;   // 2D版: 重ねた2つの描き方(5キーでUI → 2D → 3Dの順に切り替える)

public:
    explicit SpriteDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 動かすスプライトを足す(CreateSpriteDemo()から)
    void AddSprite(GameObject* object, SpriteRenderer* sprite, int effects, const std::wstring& label);

    // 2D: UIモードとWorldモードの違いを見せる、重ねた2つ
    void SetOverlapPair(SpriteRenderer* nearOne, SpriteRenderer* farOne) { nearSprite = nearOne; farSprite = farOne; }

    void Update(float dt) override;
    void Draw() override;
};
