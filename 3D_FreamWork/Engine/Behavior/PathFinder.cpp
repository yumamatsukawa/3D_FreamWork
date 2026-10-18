#include "PathFinder.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
    constexpr float diagonalCost = 1.41421356f;   // 斜め1マスの道のり(マス単位)

    // 8方向(上下左右が先、斜めが後)
    constexpr int directionX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
    constexpr int directionZ[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

    // 順番待ちを「見込みが小さい順(同じなら進んだ道のりが長い順)」にする比較(heapは大きい物が先頭なので逆)
    bool IsLater(const float estimateA, const float costA, const float estimateB, const float costB) {
        if (estimateA != estimateB) return estimateA > estimateB;
        return costA < costB;
    }
}

std::vector<XMFLOAT3> PathFinder::FindPath(XMFLOAT3 start, XMFLOAT3 goal, const PathOptions& options) {
    lastExpandedCount = 0;
    std::vector<XMFLOAT3> result;
    if (!grid) return result;

    // 始点と終点のマス(格子の外は経路なし)
    int startX, startZ, goalX, goalZ;
    if (!grid->WorldToCell(start, startX, startZ) || !grid->WorldToCell(goal, goalX, goalZ)) return result;

    // 通れないマスの中なら、近くの通れるマスに寄せる
    bool startMoved = !grid->IsWalkable(startX, startZ);
    bool goalMoved = !grid->IsWalkable(goalX, goalZ);
    XMFLOAT3 startPoint = start;   // 滑らかにする時の出発点
    XMFLOAT3 goalPoint = goal;     // 最後の点
    XMFLOAT3 exactGoal = goal;     // goal の位置(高さは格子が覚えた地面の高さ)
    if (startMoved) {
        if (!grid->FindNearestWalkable(start, startPoint) || !grid->WorldToCell(startPoint, startX, startZ)) return result;
    }
    exactGoal = grid->CellToWorld(goalX, goalZ);   // 高さ(XZ)は格子が覚えた地面の高さ。平面の位置は goal のまま
    exactGoal.x = goal.x;
    if (grid->GetSettings().plane == MovePlane::XY) exactGoal.y = goal.y;
    else exactGoal.z = goal.z;
    if (goalMoved) {
        if (!grid->FindNearestWalkable(goal, goalPoint) || !grid->WorldToCell(goalPoint, goalX, goalZ)) return result;
    }
    else {
        goalPoint = exactGoal;
    }

    // 同じマスなら、そのまま向かうだけ
    if (startX == goalX && startZ == goalZ) {
        // どちらも通れないマスの中で、寄せた先が同じ(壁際で目的地のすぐそば)なら、寄せずに目的地へまっすぐ
        if (startMoved && goalMoved) {
            result.push_back(exactGoal);
            return result;
        }
        if (startMoved) result.push_back(startPoint);
        result.push_back(goalPoint);
        return result;
    }

    // まっすぐ行けるなら探さない(広い場所ではほとんどこれで済む)
    if (options.smooth && !startMoved && grid->HasLineOfWalk(start, goalPoint)) {
        result.push_back(goalPoint);
        return result;
    }

    // ─── A* ───
    int countX = grid->GetCellCountX();
    size_t cellCount = (size_t)countX * grid->GetCellCountZ();
    if (nodes.size() != cellCount) {
        nodes.assign(cellCount, Node{});
        searchStamp = 0;
    }
    searchStamp++;
    open.clear();

    auto toIndex = [countX](int x, int z) { return z * countX + x; };
    // ゴールまでの見込みの道のり(斜めも使える時の最短。実際より大きくならない)
    auto estimate = [&](int x, int z) {
        float dx = (float)abs(x - goalX), dz = (float)abs(z - goalZ);
        if (!options.allowDiagonal) return dx + dz;
        return (dx + dz) + (diagonalCost - 2.f) * (std::min)(dx, dz);
    };
    auto isLater = [](const OpenEntry& a, const OpenEntry& b) { return IsLater(a.estimate, a.cost, b.estimate, b.cost); };

    int startIndex = toIndex(startX, startZ), goalIndex = toIndex(goalX, goalZ);
    nodes[startIndex] = Node{ 0.f, -1, searchStamp, false };
    open.push_back({ estimate(startX, startZ), 0.f, startIndex });

    bool found = false;
    int directionCount = options.allowDiagonal ? 8 : 4;
    while (!open.empty()) {
        std::pop_heap(open.begin(), open.end(), isLater);
        OpenEntry current = open.back();
        open.pop_back();
        Node& node = nodes[current.index];
        if (node.closed) continue;   // 同じマスの古い順番待ち
        node.closed = true;

        if (current.index == goalIndex) { found = true; break; }
        if (++lastExpandedCount > options.maxExpandedCells) break;   // 調べすぎ: 諦める

        int x = current.index % countX, z = current.index / countX;
        for (int d = 0; d < directionCount; d++) {
            int nx = x + directionX[d], nz = z + directionZ[d];
            if (!grid->IsWalkable(nx, nz)) continue;
            bool diagonal = (directionX[d] != 0 && directionZ[d] != 0);
            // 斜めは、隣り合う2マスの両方が通れる時だけ(壁の角を抜けない)
            if (diagonal && (!grid->IsWalkable(x + directionX[d], z) || !grid->IsWalkable(x, z + directionZ[d]))) continue;

            int neighborIndex = toIndex(nx, nz);
            Node& neighbor = nodes[neighborIndex];
            if (neighbor.stamp == searchStamp && neighbor.closed) continue;

            float newCost = node.cost + (diagonal ? diagonalCost : 1.f);
            if (neighbor.stamp == searchStamp && newCost >= neighbor.cost) continue;   // もっと短い道を見つけている
            neighbor = Node{ newCost, current.index, searchStamp, false };
            open.push_back({ newCost + estimate(nx, nz), newCost, neighborIndex });
            std::push_heap(open.begin(), open.end(), isLater);
        }
    }
    if (!found) return result;

    // ─── 経路を取り出す(ゴールから親をたどって、逆にする) ───
    std::vector<int> cells;
    for (int index = goalIndex; index != -1; index = nodes[index].parent) cells.push_back(index);
    std::reverse(cells.begin(), cells.end());   // 先頭が start のマス

    // 点の列: 出発点(実際の start。寄せた時はそのマスの中心)、各マスの中心、最後は goalPoint
    std::vector<XMFLOAT3> points;
    points.push_back(startMoved ? startPoint : start);
    for (size_t i = 1; i + 1 < cells.size(); i++) points.push_back(grid->CellToWorld(cells[i] % countX, cells[i] / countX));
    points.push_back(goalPoint);

    // 直線化: 今の点から、まっすぐ行ける限り先の点へ飛ばす
    if (startMoved) result.push_back(startPoint);   // 通れない所にいる時は、まず通れるマスの中心へ出る
    size_t anchor = 0;
    while (anchor + 1 < points.size()) {
        size_t next = anchor + 1;
        if (options.smooth) {
            while (next + 1 < points.size() && grid->HasLineOfWalk(points[anchor], points[next + 1])) next++;
        }
        result.push_back(points[next]);   // 直線化しない時は、マスの中心を1つずつ
        anchor = next;
    }
    return result;
}
