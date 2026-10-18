#pragma once
#include "MoveComponent.h"
#include "../Render3D/Mesh.h"
#include <DirectXMath.h>
#include <vector>

class NavGridComponent;

// 壁・障害物を避けながら、目的地(相手・場所)へ一定の速さで向かうComponent。ChaseComponent の「まっすぐ向かう」を、経路探索つきにしたもの。
// 目的地・速さ・StopDistance・平面は MoveComponent と同じ(SetTarget / SetSpeed / SetPlane など。決まりはMoveComponentのコメントを参照)。
//
// 使い方:
//   auto* path = enemy->AddComponent<PathFollowComponent>();
//   path->SetTarget(player);
//   path->SetSpeed(60.f);
//   path->SetPlane(MovePlane::XZ);       // 3Dの地面の上を動く時(2Dなら不要)
//   path->SetNavigation(navigation);     // 格子(シーンに置いた NavGridComponent)
//
// 決まり:
// - 経路は NavGridComponent の格子の上で探す(A* + 直線化)。見つかった経路の点を順に目指して歩く
// - 経路が見つからない時は、止まる(格子の外にいる・目的地が格子の外・壁で囲まれていて行けない・格子がまだ焼けていない)。
//   止まっている間も探し直すので、目的地や障害物が変わって行けるようになったら、また動き出す
// - 探し直すのは、一定の間隔(SetRepathInterval。±20%ばらす)・目的地が1マス以上動いた時・経路が無い時の間隔・格子が変わった時・詰まった時
// - 詰まった(1秒間に 速さ x 0.2 x 1秒 より進めなかった)時も探し直す。敵どうしの押し合い・段差で止まった時の保険
// - 最後の点は目的地。目的地が通れないマスの中(壁際)なら、近くの通れるマスの中心まで行ってから、残りをまっすぐ向かう。途中の点は SetWaypointRadius の距離に近づいたら次へ進む
// - 動かし方は MoveComponent::ApplyVelocity のとおり(DynamicはSetVelocity、XZならYの速度は残る)
// - ObjectPoolで再利用された時(SetActive(true))に、経路・タイマーをリセットする(相手・速さ・格子の設定は残る)
class PathFollowComponent : public MoveComponent {
private:
    NavGridComponent* navigation = nullptr;           // 経路を探す格子
    float repathInterval = 0.5f;                      // 探し直す間隔(秒)
    float waypointRadius = -1.f;                      // この距離まで近づいたら次の点へ(負なら cellSize の半分)

    std::vector<DirectX::XMFLOAT3> path;              // 今の経路(先頭から順に向かう)
    size_t pathIndex = 0;                             // 今向かっている点
    bool finalApproach = false;                       // 最後の点に着いて、残りを目的地へまっすぐ向かっているか
    bool needsRepath = true;                          // 次のUpdateで探し直す
    float repathTimer = 0.f;                          // 次に探し直すまでの残り時間(秒)
    DirectX::XMFLOAT3 lastDestination{};              // 前に探した時の目的地
    int lastGridVersion = -1;                         // 前に探した時の格子の version
    float stuckTimer = 0.f;                           // 詰まりを調べる間隔のタイマー
    DirectX::XMFLOAT3 stuckStartPosition{};           // 詰まりを調べ始めた時の位置
    bool warnedNoNavigation = false;                  // 「格子が渡されていません」を出力したか

    static bool debugDrawEnabled;
    static int instanceCount;                         // 線のMeshを共有している数(0になったら解放)
    static Mesh lineMesh;                             // 長さ1の線(+Z向き)。点と点の間を Transform の長さと向きで描く
    static bool lineMeshReady;

    float GetWaypointRadius() const;
    void ResetPath();                                 // 経路・タイマーを初期状態に戻す
    void DrawLine(DirectX::XMFLOAT3 from, DirectX::XMFLOAT3 to);

public:
    void SetNavigation(NavGridComponent* newNavigation) { navigation = newNavigation; needsRepath = true; }   // どの格子で探すか(Spawnの度に渡す)
    void SetRepathInterval(float seconds) { repathInterval = (seconds > 0.1f) ? seconds : 0.1f; }              // 探し直す間隔(既定0.5秒。±20%ばらす)
    void SetWaypointRadius(float distance) { waypointRadius = distance; }                                      // 次の点へ進む距離(既定は cellSize の半分)

    bool HasPath() const { return !path.empty(); }                         // 経路が見つかっているか(止まっている時は false)
    const std::vector<DirectX::XMFLOAT3>& GetPath() const { return path; }   // 今の経路(確認・デバッグ用)

    // trueにすると、今の経路を緑の線で描く(目の前の点まで → 先の点へ)
    static void SetDebugDrawEnabled(bool enabled) { debugDrawEnabled = enabled; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
    void OnActiveChanged(bool active) override;
};
