#pragma once
#include "../../../Engine/Component.h"
#include "../../../Engine/Timer.h"

class ObjectPool;

// 敵の歩きアニメーションと、弾に当たった時の処理を担当する(追いかける動きはChaseComponentが担当)
class EnemyController : public Component {
private:
    ObjectPool* pool = nullptr;     // 自分が所属するプール(倒された時に返す)
    static constexpr float frameDuration = 0.3f;    // 歩きアニメーションの1コマを表示する時間(秒)
    Timer frameTimer{ frameDuration, true };        // 次のコマへ進むまでの時間を数える
    int frameIndex = 0;                             // 今表示しているコマ(walkFramesの何番目か)

public:
    // 出現する度に呼ばれ、状態をリセットする
    void Spawn(ObjectPool* ownerPool) {
        pool = ownerPool;
        frameTimer.Reset();   // 歩きアニメーションを最初のコマから
        frameIndex = 0;
    }

    void Update(float dt) override;
    void OnTriggerEnter2D(Collider2D* other) override;
};
