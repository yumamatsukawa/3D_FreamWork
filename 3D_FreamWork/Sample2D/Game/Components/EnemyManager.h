#pragma once
#include <DirectXMath.h>
#include "../../../Engine/Component.h"
#include "../../../Engine/ObjectPool.h"

class GameObject;

// 敵の出現を担当する(一定間隔でtargetの周囲に出し、Playerが倒れたら止める)
class EnemyManager : public Component {
private:
    ObjectPool pool;                  // 敵のプール
    GameObject* target = nullptr;     // 敵が追いかける相手

    float spawnTimer = 1.0f;                          // 次の出現までの残り時間(最初だけ少し待つ)
    static constexpr float spawnInterval = 2.5f;      // 出現間隔(秒)
    static constexpr float spawnDistance = 700.0f;    // targetからどれだけ離れた場所に出すか

    bool stopped = false;             // 出現を止めたか

public:
    // 敵が追いかける相手(Player)を、CreateEnemySpawner()から渡してもらう
    void Setup(GameObject* targetObject) { target = targetObject; }

    void Init() override;
    void Update(float dt) override;

    // 1体出現させ、targetを追いかけさせる
    GameObject* Spawn(XMFLOAT3 position);
};
