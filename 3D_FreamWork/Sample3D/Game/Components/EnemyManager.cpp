#include "EnemyManager.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render/SpriteRenderer.h"
#include "../../../Engine/Animation/SpriteAnimationComponent.h"
#include "../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../Engine/Core/EventBus.h"
#include "../../../Engine/Core/MathUtil.h"
#include "../../../Engine/Behavior/ChaseComponent.h"
#include "../../../Engine/Data/Csv.h"
#include "EnemyController.h"
#include <algorithm>
#include <cmath>
#include <windows.h>

namespace {
    // 敵の速さの表と、出現ウェーブの表(Excelで編集できる。パスはここだけに書く)
    constexpr const wchar_t* enemiesCsvPath = L"Assets/Data/Enemies.csv";   // 列: Name, Speed
    constexpr const wchar_t* wavesCsvPath = L"Assets/Data/Waves.csv";       // 列: Time, Count, Interval, Repeat
    constexpr const wchar_t* enemyName = L"Slime";                          // Enemies.csvのどの行の敵か(Name列)

    void Log(const std::wstring& message) {
        OutputDebugStringW((L"★ EnemyManager: " + message + L"\n").c_str());
    }

    // プールに新しい敵を追加する時に1回だけ呼ばれる組み立て処理
    void SetupEnemy(GameObject* obj) {
        obj->SetTag("Enemy");
        obj->transform.scale = { 100.0f, 100.0f, 100.0f };

        // 見た目(3D空間に立つ2Dの画像)
        auto* sprite = obj->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
        sprite->SetColor(1.0f, 0.0f, 1.0f, 1.0f);
        sprite->SetWorldSpace(true);
        // X/Zの傾きは固定し、水平方向(Y軸)だけカメラの方を向く「立て看板」にする。
        // ※SpriteRendererのWorldモードはtransform.rotate.yを使わないので、
        //   rotate.yで向きを変える代わりに、こちらで画像の表面を見る側に向けている
        sprite->SetBillboardLock(true, false, true);
        sprite->SetFrustumCulling(true);   // 画面の外にいる間は描かない(物理と動きは止まらない)

        // 歩きアニメーション: コマ4と5を0.3秒ごとに(何体いても足踏みがそろわないように、始まるコマはランダム)
        // 再利用された時は、SpriteAnimationComponentが自分で最初からやり直す
        auto* anim = obj->AddComponent<SpriteAnimationComponent>();
        anim->AddClip("Walk", { 4, 5 }, 0.3f, true);
        anim->SetRandomStart(true);
        anim->Play("Walk");

        // 当たり判定+物理演算(PhysX): 立った形のカプセル(半径40・高さ100)。箱と違い、角が段差に引っかからない
        auto* rigidbody = obj->AddComponent<CapsuleRigidbodyComponent>(BodyType::Dynamic, 40.0f, 100.0f, 1.0f);
        rigidbody->SetUseGravity(true);
        rigidbody->SetFreezeRotation(true);

        // 動き: 地面(XZ平面)の上を追いかける。Yの速度(重力で落ちる速さ)はChaseComponentがそのまま残す
        // (追いかける相手と速さはSpawnで渡す)
        auto* chase = obj->AddComponent<ChaseComponent>();
        chase->SetPlane(MovePlane::XZ);

        // 弾に当たった時の処理
        obj->AddComponent<EnemyController>();
    }
}

void EnemyManager::LoadEnemies() {
    // 敵の速さ: Name列がenemyNameの行のSpeed(行が無い・読めなければ、読めなかった時の値)
    enemySpeed = defaultEnemySpeed;
    int row = enemiesCsv.FindRow("Name", enemyName);
    if (row >= 0) {
        enemySpeed = enemiesCsv.GetFloat(row, "Speed", defaultEnemySpeed);
    }
    else if (enemiesCsv.IsValid()) {
        Log(std::wstring(enemiesCsvPath) + L" に Name が " + enemyName + L" の行がないので、速さは " + std::to_wstring(defaultEnemySpeed) + L" にします");
    }
}

void EnemyManager::LoadWaves() {
    // 出現ウェーブ: 1行が1ウェーブ(Time秒後から、Interval秒ごとにCount体ずつ、Repeat回。Repeatが0なら次のウェーブまで)
    waves.clear();
    const Csv& table = wavesCsv;
    for (int row = 0; row < table.GetRowCount(); row++) {
        Wave wave;
        wave.time = table.GetFloat(row, "Time", 0.0f);
        wave.count = table.GetInt(row, "Count", 0);
        wave.interval = table.GetFloat(row, "Interval", 0.0f);
        wave.repeat = table.GetInt(row, "Repeat", 0);
        if (wave.time < 0.0f) wave.time = 0.0f;
        if (wave.repeat < 0) wave.repeat = 0;
        if (wave.count <= 0) {
            Log(std::wstring(wavesCsvPath) + L" の row " + std::to_wstring(row) + L" は Count が1以上ではないので飛ばします");
            continue;
        }
        waves.push_back(wave);
    }
    // 表の行が時間の順でなくても動くように、始まる時間の順に並べる(同じ時間なら表の順)
    std::stable_sort(waves.begin(), waves.end(), [](const Wave& a, const Wave& b) { return a.time < b.time; });

    // 使えるウェーブが無ければ、今までの出現(最初の1体が1秒後、その後2.5秒ごとに1体)にする
    if (waves.empty()) {
        Log(std::wstring(wavesCsvPath) + L" が使えないので、今までの出現(1秒後から2.5秒ごとに1体)にします");
        Wave wave;
        wave.time = defaultFirstSpawnDelay;
        wave.count = 1;
        wave.interval = defaultSpawnInterval;
        waves.push_back(wave);
    }
}

void EnemyManager::ScheduleNext() {
    while (waveIndex < waves.size()) {
        const Wave& wave = waves[waveIndex];
        float nextTime = wave.time + spawnedCount * wave.interval;
        bool hasNextWave = waveIndex + 1 < waves.size();
        bool finished =
            (wave.repeat > 0 && spawnedCount >= wave.repeat) ||                          // 決めた回数を出した
            (spawnedCount > 0 && wave.interval <= 0.0f) ||                               // 間隔が無いので1回だけ
            (hasNextWave && spawnedCount > 0 && nextTime >= waves[waveIndex + 1].time);  // 次のウェーブが始まる
        if (!finished) {
            nextSpawnTime = nextTime;
            return;
        }
        waveIndex++;
        spawnedCount = 0;
    }
}

void EnemyManager::RescheduleWaves() {
    // 表が変わった時の数え直し。経過時間(elapsedTime)はそのままにして、
    // 新しい表で「今の経過時間より前に出るはずだった回」はすべて「出した扱い」にし、次に出す時間だけを決め直す。
    // こうすると、すでに出した分を二重に出さず、これから出す分を飛ばさない
    // (回数を1回ずつ数えると、Intervalが極端に小さい時に終わらないので、ウェーブごとに計算で数える)
    waveIndex = 0;
    spawnedCount = 0;
    for (int pass = 0; pass < 1000; pass++) {   // 毎回必ず進むので、ウェーブの数ぶんで終わる(念のための上限)
        ScheduleNext();
        if (waveIndex >= waves.size() || nextSpawnTime > elapsedTime) return;

        // このウェーブで、経過時間までに出るはずだった回数(0回目からの個数)
        const Wave& wave = waves[waveIndex];
        double count = 1.0;
        if (wave.interval > 0.0f) {
            count = std::floor(((double)elapsedTime - wave.time) / wave.interval) + 1.0;
            if (wave.repeat > 0) count = (std::min)(count, (double)wave.repeat);
            if (waveIndex + 1 < waves.size()) {
                // 次のウェーブが始まる前までの回数が上限(ScheduleNextの終わり方と同じ。最初の1回は必ず出る)
                double untilNext = std::ceil(((double)waves[waveIndex + 1].time - wave.time) / wave.interval);
                count = (std::min)(count, (std::max)(untilNext, 1.0));
            }
        }
        count = (std::min)(count, 1.0e9);   // intに入る大きさにする
        spawnedCount = (std::max)((int)count, spawnedCount + 1);   // 最低1つは進める(計算の誤差で止まらないように)
    }
}

void EnemyManager::ApplySpeedToEnemies() {
    // 出ている敵(プールの中のアクティブな敵)に、新しい速さを渡す。出ていない敵は、Spawnの時に渡される
    for (GameObject* enemy : pool.GetObjects()) {
        if (!enemy->GetIsActive()) continue;
        ChaseComponent* chase = enemy->GetComponent<ChaseComponent>();
        if (chase) chase->SetSpeed(enemySpeed);
    }
}

void EnemyManager::CheckReload() {
    // 更新されたCSVだけ読み直す。読み直しに失敗したら(保存の途中など)、今の値のまま動いて、次の確認でまた試す
    if (enemiesCsv.IsFileUpdated() && enemiesCsv.Reload()) {
        LoadEnemies();
        ApplySpeedToEnemies();
        Log(L"Enemies.csv が更新されたので、読み直しました(速さ " + std::to_wstring(enemySpeed) + L")");
    }
    if (wavesCsv.IsFileUpdated() && wavesCsv.Reload()) {
        LoadWaves();
        RescheduleWaves();
        Log(L"Waves.csv が更新されたので、読み直しました(" + std::to_wstring(waves.size()) + L"ウェーブ)");
    }
}

void EnemyManager::Init() {
    // 表を読む(Init時に読み、その後は実行中の更新をCheckReloadが0.5秒ごとに確かめる)
    enemiesCsv = Csv::Load(enemiesCsvPath);
    wavesCsv = Csv::Load(wavesCsvPath);
    reloadTimer.Reset();
    LoadEnemies();
    LoadWaves();
    waveIndex = 0;
    spawnedCount = 0;
    elapsedTime = 0.0f;
    ScheduleNext();

    // Playerが倒れたら出現を止める
    // ※ラムダがthisを使うので、シーン終了時にGameScene::Uninit()で登録を消している
    EventBus::Get().Subscribe("PlayerDied", [this]() {
        stopped = true;
    });
}

void EnemyManager::Update(float dt) {
    if (stopped || !target) return;

    // 表の更新の確認(0.5秒ごと)
    if (reloadTimer.Update(dt)) CheckReload();

    // 表の時間になったら出現(1フレームに1回まで。遅れた分は次のフレームで出す)
    elapsedTime += dt;
    if (waveIndex < waves.size() && elapsedTime >= nextSpawnTime) {
        for (int i = 0; i < waves[waveIndex].count; i++) {
            // targetを中心に、XZ平面上のランダムな方向・一定距離離れた地面の上に出現させる
            // (円周上の点のx,yを、地面のx,zとして使う)
            XMFLOAT2 offset = Math::RandomOnCircle(spawnDistance);
            XMFLOAT3 pos = {
                target->transform.position.x + offset.x,
                spawnHeight,
                target->transform.position.z + offset.y
            };
            Spawn(pos);
        }
        spawnedCount++;
        ScheduleNext();
    }
}

GameObject* EnemyManager::Spawn(XMFLOAT3 position) {
    // プールから借りて、位置・向き・色をリセット(前回使った時の値が残っているため)
    GameObject* enemy = pool.Rent(*GetOwner()->GetScene(), SetupEnemy);
    enemy->transform.position = position;
    enemy->transform.rotate = { 0.f, 0.f, 0.f };
    SpriteRenderer* sprite = enemy->GetComponent<SpriteRenderer>();
    if (sprite) sprite->SetColor(1.0f, 0.0f, 1.0f, 1.0f);

    // 追いかける相手と速さを渡す
    ChaseComponent* chase = enemy->GetComponent<ChaseComponent>();
    if (chase) {
        chase->SetTarget(target);
        chase->SetSpeed(enemySpeed);
    }

    // 倒された時の返却先を渡す
    EnemyController* controller = enemy->GetComponent<EnemyController>();
    if (controller) controller->Spawn(&pool);

    return enemy;
}
