#pragma once
#include "../../../Engine/Component.h"
#include "../../../Engine/Timer.h"
#include <DirectXMath.h>

class ObjectPool;
class GameObject;

// 敵の動き(地面の上でtargetを追いかける)と、弾に当たった時の処理を担当する
class EnemyController : public Component {
private:
    GameObject* target = nullptr;   // 追いかける相手
    float speed = 60.0f;            // 移動速度
    ObjectPool* pool = nullptr;     // 自分が所属するプール(倒された時に返す)
    static constexpr float frameDuration = 0.3f;    // 歩きアニメーションの1コマを表示する時間(秒)
    Timer frameTimer{ frameDuration, true };        // 次のコマへ進むまでの時間を数える
    int frameIndex = 0;                             // 今表示しているコマ(walkFramesの何番目か)

public:
    // 出現する度に呼ばれ、状態をリセットする
    void Spawn(GameObject* chaseTarget, ObjectPool* ownerPool) {
        target = chaseTarget;
        pool = ownerPool;
        frameTimer.Reset();   // 歩きアニメーションを最初のコマから
        frameIndex = 0;
    }

    void Update(float dt) override;
    void OnTriggerEnter2D(Collider2D* other) override;
};
