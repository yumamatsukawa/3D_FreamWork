#pragma once
#include "../Core/Component.h"
#include "../Render3D/Mesh.h"
#include "NavGrid.h"
#include "PathFinder.h"
#include <utility>
#include <vector>

class TerrainRenderer;

// 格子(NavGrid)と探索役(PathFinder)を持つComponent。シーンに1つ置いて、PathFollowComponent に渡す。
// 障害物(タグ "Wall" の Static な物)は、置くだけで自動で読み込まれる。
//
// 決まり:
// - 障害物を読む(焼く)のは、2回目のUpdate。物理に出したばかりの物は次のフレームから見つかるため、Initでは読めない。
//   焼き終わるまでは IsBaked() が false で、歩く側は待つ
// - 動く壁(Kinematic)があるなら、SetAutoRebakeInterval で定期的に焼き直すか、動かした側が RequestRebake / RebakeRegion を呼ぶ。
//   どちらも次のフレームの Update で焼く(Kinematic の壁は、動かしたフレームの次の Physics::Update で物理の位置が変わるため。同じフレームに焼くと古い位置を読む)
// - 探索の回数は1フレームに maxSearchesPerFrame まで(TryBeginSearch)。敵が多くても、探索が同じフレームに集中して重くならない
// - 焼いた時間は GetLastBakeMilliseconds() で分かる(出力ウィンドウにも出す)
class NavGridComponent : public Component {
private:
    NavGrid grid;
    PathFinder pathFinder;
    TerrainRenderer* terrain = nullptr;      // あれば、焼く時に地形の高さ・傾きを取り込む

    int updateCount = 0;                     // Update が呼ばれた回数(最初の焼きを遅らせるため)
    bool baked = false;                      // 最初の焼きが済んだか
    bool rebakeRequested = false;            // 次のフレームで全体を焼き直す
    std::vector<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>> pendingRegions;   // 次のフレームで焼き直す範囲(中心, 大きさ)
    float autoRebakeInterval = 0.f;          // 自動で焼き直す間隔(秒。0なら焼き直さない)
    float autoRebakeTimer = 0.f;             // 次の焼き直しまでの残り時間
    float lastBakeMilliseconds = 0.f;        // 直前の焼きにかかった時間(ミリ秒)

    int maxSearchesPerFrame = 8;             // 1フレームに探索してよい回数
    int searchesThisFrame = 0;               // このフレームに許可した探索の数

    // デバッグ表示(通れないマスの輪郭の線)
    Mesh blockedMesh;
    bool blockedMeshReady = false;
    int blockedMeshVersion = -1;             // 今の blockedMesh を作った時の格子の version
    static bool debugDrawEnabled;

    void Bake();                             // 全体を焼く(地形 → 物理の順)
    void RebuildDebugMesh();                 // 通れないマスの輪郭の線を作り直す

public:
    explicit NavGridComponent(const NavGridSettings& settings);

    NavGrid& GetGrid() { return grid; }
    PathFinder& GetPathFinder() { return pathFinder; }

    void SetTerrain(TerrainRenderer* newTerrain);              // 地形の高さ・傾きを取り込む(焼く前に渡す。焼いた後なら焼き直す)
    void SetAutoRebakeInterval(float seconds);                 // 0=自動で焼き直さない(既定)。動く壁がある時だけ設定する
    void RequestRebake() { rebakeRequested = true; }           // 次のフレームで全体を焼き直す
    void RebakeRegion(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 size);   // 範囲だけ、次のフレームのUpdateで焼き直す(動く壁を動かした側が呼ぶ。全体より軽い)
    void SetMaxSearchesPerFrame(int count) { maxSearchesPerFrame = (count > 1) ? count : 1; }   // 1フレームの探索回数の上限(既定8)

    bool IsBaked() const { return baked; }
    float GetLastBakeMilliseconds() const { return lastBakeMilliseconds; }

    // 探索してよいか(PathFollowComponent が探索の前に呼ぶ)。上限に達していたら false で、次のフレームにまわす
    bool TryBeginSearch();

    // trueにすると、通れないマスの輪郭を赤い線で描く(RigidbodyComponent の SetDebugDrawEnabled と同じ使い方)
    static void SetDebugDrawEnabled(bool enabled) { debugDrawEnabled = enabled; }

    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
