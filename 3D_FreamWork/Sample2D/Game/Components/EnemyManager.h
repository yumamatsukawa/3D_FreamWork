#pragma once
#include <DirectXMath.h>
#include "../../../Engine/Core/Component.h"
#include "../../../Engine/Core/ObjectPool.h"
#include "../../../Engine/Core/Timer.h"
#include "../../../Engine/Data/Csv.h"
#include <vector>

class GameObject;

// 敵の出現を担当する(実行中にCSVを保存すると、そのまま反映される。Waves.csvの表のとおりにtargetの周囲に出し、Playerが倒れたら止める。敵の速さはEnemies.csv)
class EnemyManager : public Component {
private:
    // 出現ウェーブ(Waves.csvの1行)。Time秒後から、Interval秒ごとにCount体ずつ出す
    struct Wave {
        float time = 0.0f;       // 始まる時間(秒。シーンが始まってから)
        int count = 1;           // 1回に出す数
        float interval = 0.0f;   // 出す間隔(秒)。0以下なら1回だけ
        int repeat = 0;          // 出す回数。0なら、次のウェーブが始まるまで(最後のウェーブなら、ずっと)続ける
    };

    ObjectPool pool;                  // 敵のプール
    GameObject* target = nullptr;     // 敵が追いかける相手

    // 表が読めなかった時に使う値(今までの動き: 1秒後に最初の1体、その後2.5秒ごとに1体)
    static constexpr float defaultFirstSpawnDelay = 1.0f;   // 最初の1体が出るまでの時間(秒)
    static constexpr float defaultSpawnInterval = 2.5f;     // 出現間隔(秒)
    static constexpr float defaultEnemySpeed = 60.0f;       // 敵が追いかける速さ
    static constexpr float spawnDistance = 700.0f;          // targetからどれだけ離れた場所に出すか

    static constexpr float reloadCheckInterval = 0.5f;      // CSVが更新されたか調べる間隔(秒)。毎フレームは調べない

    Csv enemiesCsv;                         // Enemies.csv(更新されたらReloadする)
    Csv wavesCsv;                           // Waves.csv(更新されたらReloadする)
    Timer reloadTimer{ reloadCheckInterval, true };   // 更新の確認用タイマー

    float enemySpeed = defaultEnemySpeed;   // 敵が追いかける速さ(Enemies.csvのSpeed)
    std::vector<Wave> waves;                // 出現ウェーブ(始まる時間の順)
    size_t waveIndex = 0;                   // 今のウェーブ(waves.size()なら、もう出さない)
    int spawnedCount = 0;                   // 今のウェーブで、何回出したか
    float elapsedTime = 0.0f;               // シーンが始まってからの時間(秒。止まっている間は進めない)
    float nextSpawnTime = 0.0f;             // 次に出す時間(秒)

    bool stopped = false;             // 出現を止めたか

    void LoadEnemies();   // enemiesCsvから敵の速さを取り出す
    void LoadWaves();     // wavesCsvから出現ウェーブを取り出す
    void CheckReload();   // CSVが更新されていたら読み直して反映する
    // 表が変わった後、今の経過時間のまま「今どのウェーブの何回目か」を数え直す(出した分を二重に出さず、飛ばさない)
    void RescheduleWaves();
    void ApplySpeedToEnemies();   // 出ている敵すべてに、今の敵の速さを渡す
    // 今のウェーブの次に出す時間を決める(今のウェーブが終わっていれば、次のウェーブに移る)
    void ScheduleNext();

public:
    // 敵が追いかける相手(Player)を、CreateEnemySpawner()から渡してもらう
    void Setup(GameObject* targetObject) { target = targetObject; }

    void Init() override;
    void Update(float dt) override;

    // 1体出現させ、targetを追いかけさせる
    GameObject* Spawn(XMFLOAT3 position);
};
