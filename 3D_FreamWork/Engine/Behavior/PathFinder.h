#pragma once
#include "NavGrid.h"
#include <DirectXMath.h>
#include <vector>

// 経路探索のやり方の設定
struct PathOptions {
    bool smooth = true;             // 経路を直線化する(見通せる限り先の点へ飛ばす)
    bool allowDiagonal = true;      // 斜めの移動を使う(8方向)。壁の角を斜めに抜けるのは禁止する
    int maxExpandedCells = 20000;   // 1回の探索で調べるマスの上限(超えたら諦めて空の経路を返す)
};

// 格子(NavGrid)の上で、A*(エースター)で最短の経路を探す。
// 作業用の配列は PathFinder が1組だけ持って使い回す(敵ごとに持たない)。1つの PathFinder を同時に別々の探索には使えない(同じスレッドで順に呼ぶ)。
//
// 決まり:
// - 経路は、ワールド座標の点の並び。先頭は start の次の点、最後は goal。見つからなければ空
// - start / goal が通れないマス(太らせた障害物の中)なら、近くの通れるマスの中心に寄せてから探す。
//   寄せた goal の時は、最後の点がその中心になる。start が寄せられた時は、先頭がその中心になる(どちらも通れないマスで、寄せた先が同じマスなら [goal] だけ)
// - start / goal が格子の外、寄せられる場所が無い、つながっていない、maxExpandedCells を超えた時は、空の経路
// - 斜めの移動は、隣り合う2マスの両方が通れる時だけ(壁の角にめり込まない)
// - smooth の時は、まっすぐ行ける(NavGrid::HasLineOfWalk)なら探さずに [goal] だけを返す
class PathFinder {
public:
    explicit PathFinder(const NavGrid* newGrid) : grid(newGrid) {}

    std::vector<DirectX::XMFLOAT3> FindPath(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 goal, const PathOptions& options = PathOptions{});
    int GetLastExpandedCount() const { return lastExpandedCount; }   // 直前の探索で調べたマスの数(確認用)

private:
    // 探索中のマスの情報。stamp が今の探索の番号と違えば「まだ見ていない」(配列を毎回塗り直さないため)
    struct Node {
        float cost = 0.f;       // start からここまでの道のり(マス単位)
        int parent = -1;        // ひとつ前のマス
        int stamp = 0;          // 見た探索の番号
        bool closed = false;    // 調べ終わったか
    };
    // 調べる順番待ち(estimate が小さいものから)
    struct OpenEntry {
        float estimate;         // ここまでの道のり + ゴールまでの見込み
        float cost;
        int index;
    };

    const NavGrid* grid;
    std::vector<Node> nodes;
    std::vector<OpenEntry> open;
    int searchStamp = 0;
    int lastExpandedCount = 0;
};
