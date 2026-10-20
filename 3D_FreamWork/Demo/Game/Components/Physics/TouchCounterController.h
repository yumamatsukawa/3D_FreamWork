#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

// 当たり判定のコールバック(OnTrigger○○2D / OnCollision○○2D)が呼ばれた回数を数える。Triggerデモが使う。
// 付けた物に、tag のタグを持つ物が当たった時だけ数える。
// トリガー(すり抜ける)は Enter(入った瞬間)・Stay(中にいる間は毎フレーム)・Exit(出た瞬間)、
// 通常の当たり判定(ぶつかる)は CollisionEnter・Stay・Exit が呼ばれる。その回数を、別々に数える
class TouchCounterController : public Component {
public:
    // 呼ばれた回数
    struct Counts {
        int enter = 0;
        int stay = 0;
        int exit = 0;
    };

private:
    std::string targetTag;      // 数える相手のタグ
    Counts trigger;             // トリガーのコールバックの回数
    Counts collision;           // ぶつかるコールバックの回数
    int touching = 0;           // 今トリガーの中にいる数(Enter - Exit)

public:
    explicit TouchCounterController(const std::string& tag) : targetTag(tag) {}

    const Counts& GetTrigger() const { return trigger; }
    const Counts& GetCollision() const { return collision; }
    bool IsTouching() const { return touching > 0; }      // 今トリガーの中に相手がいるか
    void ResetCounts() { trigger = Counts(); collision = Counts(); }   // 回数を0に戻す(今中にいるかは変えない)

    void OnTriggerEnter2D(Collider2D* other) override;
    void OnTriggerStay2D(Collider2D* other) override;
    void OnTriggerExit2D(Collider2D* other) override;
    void OnCollisionEnter2D(CollisionInfo info) override;
    void OnCollisionStay2D(CollisionInfo info) override;
    void OnCollisionExit2D(CollisionInfo info) override;
};
