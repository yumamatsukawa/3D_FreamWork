#pragma once
#include <DirectXMath.h>
#include "../../../Engine/Component.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/Timer.h"

class GameObject;

// 敵の出現を担当する(一定間隔でtargetの周りの地面の上に出し、Playerが倒れたら止める)
class EnemyManager : public Component {
private:
    ObjectPool pool;                  // 敵のプール
    GameObject* target = nullptr;     // 敵が追いかける相手

    static constexpr float spawnInterval = 2.5f;      // 出現間隔(秒)
    static constexpr float firstSpawnDelay = 1.0f;    // 最初の1体が出るまでの時間(秒)
    Timer spawnTimer{ spawnInterval, true };          // 出現間隔を数える(最初だけInitで短くする)
    static constexpr float spawnDistance = 700.0f;    // targetからどれだけ離れた場所に出すか(XZ平面上)
    static constexpr float spawnHeight = 10.0f;       // 出現する高さ(敵の中心。地面の上面-50より少し上に出して、重力で着地させる)
    static constexpr float enemySpeed = 60.0f;        // 敵が追いかける速さ

    bool stopped = false;             // 出現を止めたか

public:
    // 敵が追いかける相手(Player)を、CreateEnemySpawner()から渡してもらう
    void Setup(GameObject* targetObject) { target = targetObject; }

    void Init() override;
    void Update(float dt) override;

    // 1体出現させ、targetを追いかけさせる
    GameObject* Spawn(XMFLOAT3 position);
};
