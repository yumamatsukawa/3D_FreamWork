#include "NavGrid.h"
#include "../Physics/Physics.h"
#include "../Render3D/TerrainRenderer.h"
#include "../Core/MathUtil.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <windows.h>
#include <cstdio>

namespace {
    constexpr int blockSize = 8;               // 物理を調べる時、まず blockSize x blockSize マスをまとめて調べて、何も無ければ飛ばす
    constexpr float thickness2D = 10.f;        // 2D(XY)で壁を探す箱の厚み(Z方向。薄くしておけば、敵のいる Z=0 の壁だけが見つかる)

    // 平面の座標 u(X)・v(XYならY、XZならZ)
    float GetU(XMFLOAT3 p) { return p.x; }
    float GetV(XMFLOAT3 p, MovePlane plane) { return (plane == MovePlane::XY) ? p.y : p.z; }
}

NavGrid::NavGrid(const NavGridSettings& newSettings) : settings(newSettings) {
    // 使えない値は安全な値にする
    if (!(settings.cellSize > 0.f)) settings.cellSize = 50.f;
    if (!(settings.size.x > 0.f)) settings.size.x = settings.cellSize;
    if (!(settings.size.y > 0.f)) settings.size.y = settings.cellSize;
    if (!(settings.agentRadius > 0.f)) settings.agentRadius = 0.f;

    // マスの数。上限(maxCells)を超えるなら、cellSize を大きくして収める
    double countX = std::ceil((double)settings.size.x / settings.cellSize);
    double countZ = std::ceil((double)settings.size.y / settings.cellSize);
    if (countX * countZ > (double)maxCells) {
        float oldCellSize = settings.cellSize;
        while (countX * countZ > (double)maxCells) {
            settings.cellSize = (float)(settings.cellSize * std::sqrt(countX * countZ / (double)maxCells) * 1.01);
            countX = std::ceil((double)settings.size.x / settings.cellSize);
            countZ = std::ceil((double)settings.size.y / settings.cellSize);
        }
        char message[256];
        sprintf_s(message, "★ NavGrid: マスが多すぎる(上限 %d)ので、cellSize を %.1f から %.1f にしました\n", maxCells, oldCellSize, settings.cellSize);
        OutputDebugStringA(message);
    }
    cellCountX = (std::max)(1, (int)countX);
    cellCountZ = (std::max)(1, (int)countZ);

    blocked.assign((size_t)cellCountX * cellCountZ, 0);
    if (settings.plane == MovePlane::XZ) heights.assign((size_t)cellCountX * cellCountZ, settings.baseHeight);
}

// ─── 障害物を取り込む ─────────────────────────────
bool NavGrid::SetFlag(int index, uint8_t kind, bool on) {
    uint8_t before = blocked[index];
    if (on) blocked[index] |= kind;
    else blocked[index] &= (uint8_t)~kind;
    return blocked[index] != before;
}

void NavGrid::Clear() {
    std::fill(blocked.begin(), blocked.end(), (uint8_t)0);
    std::fill(heights.begin(), heights.end(), settings.baseHeight);
    version++;
}

void NavGrid::SetBlocked(int cellX, int cellZ, bool isBlocked) {
    if (!IsInside(cellX, cellZ)) return;
    if (SetFlag(ToIndex(cellX, cellZ), blockedManual, isBlocked)) version++;
}

void NavGrid::SetBlockedBox(XMFLOAT3 center, XMFLOAT3 size, bool isBlocked) {
    // 箱の範囲を半径ぶん太らせて、重なるマスを塞ぐ(高さは見ない)
    float pad = settings.agentRadius;
    float halfU = size.x * 0.5f;
    float halfV = ((settings.plane == MovePlane::XY) ? size.y : size.z) * 0.5f;
    float centerU = GetU(center), centerV = GetV(center, settings.plane);
    int x0, z0, x1, z1;
    if (!GetCellRange(centerU - halfU - pad, centerU + halfU + pad, centerV - halfV - pad, centerV + halfV + pad, x0, z0, x1, z1)) return;

    bool changed = false;
    for (int z = z0; z <= z1; z++) {
        for (int x = x0; x <= x1; x++) changed |= SetFlag(ToIndex(x, z), blockedManual, isBlocked);
    }
    if (changed) version++;
}

bool NavGrid::HasPhysicsObstacle(int x0, int z0, int x1, int z1) const {
    // 範囲内のマスの「太らせた箱」をすべて含む大きな箱を作る
    float cs = settings.cellSize;
    float pad = settings.agentRadius;
    float originV = (settings.plane == MovePlane::XY) ? settings.origin.y : settings.origin.z;
    float minU = settings.origin.x + x0 * cs - pad;
    float maxU = settings.origin.x + (x1 + 1) * cs + pad;
    float minV = originV + z0 * cs - pad;
    float maxV = originV + (z1 + 1) * cs + pad;

    XMFLOAT3 center, size;
    if (settings.plane == MovePlane::XY) {
        center = { (minU + maxU) * 0.5f, (minV + maxV) * 0.5f, settings.origin.z };
        size = { maxU - minU, maxV - minV, thickness2D };
    }
    else {
        // 高さの窓: 地面の高さ + 登れる段差 から 地面の高さ + 背の高さ まで(地面そのものや低い段差は障害物にしない)
        float lowestGround = heights[ToIndex(x0, z0)], highestGround = lowestGround;
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) {
                float h = heights[ToIndex(x, z)];
                lowestGround = (std::min)(lowestGround, h);
                highestGround = (std::max)(highestGround, h);
            }
        }
        float low = lowestGround + settings.maxStepHeight;
        float high = highestGround + settings.agentHeight;
        center = { (minU + maxU) * 0.5f, (low + high) * 0.5f, (minV + maxV) * 0.5f };
        size = { maxU - minU, (std::max)(high - low, 1.f), maxV - minV };
    }
    return !Physics::OverlapBox(center, size, settings.obstacleTag).empty();
}

bool NavGrid::BakePhysicsRange(int x0, int z0, int x1, int z1) {
    bool changed = false;

    // 障害物のタグが無ければ、物理の障害物は無し
    if (settings.obstacleTag.empty()) {
        for (int z = z0; z <= z1; z++) {
            for (int x = x0; x <= x1; x++) changed |= SetFlag(ToIndex(x, z), blockedPhysics, false);
        }
        return changed;
    }

    // blockSize x blockSize マスのまとまりごとに、まず大きな箱で調べる(何も無ければ、そのまとまりの問い合わせは1回で済む)
    for (int bz = z0; bz <= z1; bz += blockSize) {
        for (int bx = x0; bx <= x1; bx += blockSize) {
            int ex = (std::min)(bx + blockSize - 1, x1);
            int ez = (std::min)(bz + blockSize - 1, z1);
            bool any = HasPhysicsObstacle(bx, bz, ex, ez);
            bool single = (bx == ex && bz == ez);

            for (int z = bz; z <= ez; z++) {
                for (int x = bx; x <= ex; x++) {
                    bool hit = any && (single || HasPhysicsObstacle(x, z, x, z));
                    changed |= SetFlag(ToIndex(x, z), blockedPhysics, hit);
                }
            }
        }
    }
    return changed;
}

void NavGrid::BakeFromPhysics() {
    if (BakePhysicsRange(0, 0, cellCountX - 1, cellCountZ - 1)) version++;
}

void NavGrid::BakeRegionFromPhysics(XMFLOAT3 center, XMFLOAT3 size) {
    // 範囲の中で変わった物が影響するのは、「太らせた箱」が範囲に重なるマス
    float pad = settings.agentRadius;
    float halfU = size.x * 0.5f;
    float halfV = ((settings.plane == MovePlane::XY) ? size.y : size.z) * 0.5f;
    float centerU = GetU(center), centerV = GetV(center, settings.plane);
    int x0, z0, x1, z1;
    if (!GetCellRange(centerU - halfU - pad, centerU + halfU + pad, centerV - halfV - pad, centerV + halfV + pad, x0, z0, x1, z1)) return;
    if (BakePhysicsRange(x0, z0, x1, z1)) version++;
}

void NavGrid::BakeFromTerrain(const TerrainRenderer& terrain) {
    if (settings.plane != MovePlane::XZ) {
        OutputDebugStringA("★ NavGrid: BakeFromTerrain は XZ平面(3D)の格子でだけ使えます\n");
        return;
    }
    bool changed = false;
    float cs = settings.cellSize;

    // 各マスの地面の高さ(地形の外は baseHeight)
    for (int z = 0; z < cellCountZ; z++) {
        for (int x = 0; x < cellCountX; x++) {
            float worldX = settings.origin.x + (x + 0.5f) * cs;
            float worldZ = settings.origin.z + (z + 0.5f) * cs;
            float h = terrain.Contains(worldX, worldZ) ? terrain.GetHeightAt(worldX, worldZ) : settings.baseHeight;
            if (heights[ToIndex(x, z)] != h) {
                heights[ToIndex(x, z)] = h;
                changed = true;
            }
        }
    }

    // 急な坂・大きな段差のマスを塞ぐ
    float slopeDegrees = Math::Clamp(settings.maxSlopeDegrees, 0.f, 89.9f);
    float cosSlope = cosf(XMConvertToRadians(slopeDegrees));
    float tanSlope = tanf(XMConvertToRadians(slopeDegrees));
    for (int z = 0; z < cellCountZ; z++) {
        for (int x = 0; x < cellCountX; x++) {
            bool steep = false;
            float worldX = settings.origin.x + (x + 0.5f) * cs;
            float worldZ = settings.origin.z + (z + 0.5f) * cs;

            // 傾き: マスの中心と4つの角の面の向きを調べる(中心1点だと、細い崖を見逃す)
            const float offsets[5][2] = { { 0.f, 0.f }, { -0.5f, -0.5f }, { 0.5f, -0.5f }, { -0.5f, 0.5f }, { 0.5f, 0.5f } };
            for (const auto& offset : offsets) {
                float px = worldX + offset[0] * cs, pz = worldZ + offset[1] * cs;
                if (!terrain.Contains(px, pz)) continue;
                if (terrain.GetNormalAt(px, pz).y < cosSlope) { steep = true; break; }
            }

            // 段差: 隣のマスとの高さの差が、登れる段差か坂の傾きで説明できる高さを超えていたら塞ぐ
            float h = heights[ToIndex(x, z)];
            for (int dz = -1; dz <= 1 && !steep; dz++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if ((dx == 0 && dz == 0) || !IsInside(x + dx, z + dz)) continue;
                    float distance = cs * ((dx != 0 && dz != 0) ? 1.4142f : 1.f);
                    float allowed = (std::max)(settings.maxStepHeight, tanSlope * distance);
                    if (fabsf(h - heights[ToIndex(x + dx, z + dz)]) > allowed) { steep = true; break; }
                }
            }
            changed |= SetFlag(ToIndex(x, z), blockedTerrain, steep);
        }
    }
    if (changed) version++;
}

// ─── 調べる ─────────────────────────────
XMFLOAT2 NavGrid::ToCellSpace(float u, float v) const {
    float originV = (settings.plane == MovePlane::XY) ? settings.origin.y : settings.origin.z;
    return { (u - settings.origin.x) / settings.cellSize, (v - originV) / settings.cellSize };
}

bool NavGrid::GetCellRange(float minU, float maxU, float minV, float maxV, int& x0, int& z0, int& x1, int& z1) const {
    XMFLOAT2 lo = ToCellSpace(minU, minV);
    XMFLOAT2 hi = ToCellSpace(maxU, maxV);
    if (!(hi.x > 0.f && hi.y > 0.f && lo.x < (float)cellCountX && lo.y < (float)cellCountZ)) return false;   // 格子と重ならない(NaNもここで弾く)

    x0 = (std::max)(0, (int)floorf(lo.x));
    z0 = (std::max)(0, (int)floorf(lo.y));
    x1 = (std::min)(cellCountX - 1, (int)ceilf(hi.x) - 1);
    z1 = (std::min)(cellCountZ - 1, (int)ceilf(hi.y) - 1);
    x1 = (std::max)(x1, x0);
    z1 = (std::max)(z1, z0);
    return true;
}

bool NavGrid::WorldToCell(XMFLOAT3 position, int& cellX, int& cellZ) const {
    XMFLOAT2 c = ToCellSpace(GetU(position), GetV(position, settings.plane));
    if (!(c.x >= 0.f && c.y >= 0.f && c.x < (float)cellCountX && c.y < (float)cellCountZ)) return false;   // 格子の外(NaNもここで弾く)
    cellX = (int)floorf(c.x);
    cellZ = (int)floorf(c.y);
    return true;
}

float NavGrid::GetCellHeight(int cellX, int cellZ) const {
    return heights.empty() ? 0.f : heights[ToIndex(cellX, cellZ)];
}

XMFLOAT3 NavGrid::CellToWorld(int cellX, int cellZ) const {
    float cs = settings.cellSize;
    float u = settings.origin.x + (cellX + 0.5f) * cs;
    if (settings.plane == MovePlane::XY) return { u, settings.origin.y + (cellZ + 0.5f) * cs, settings.origin.z };
    return { u, GetCellHeight(cellX, cellZ), settings.origin.z + (cellZ + 0.5f) * cs };
}

XMFLOAT3 NavGrid::ClampToGrid(XMFLOAT3 position) const {
    float half = settings.cellSize * 0.5f;
    float originV = (settings.plane == MovePlane::XY) ? settings.origin.y : settings.origin.z;
    position.x = Math::Clamp(position.x, settings.origin.x + half, settings.origin.x + cellCountX * settings.cellSize - half);
    float v = Math::Clamp(GetV(position, settings.plane), originV + half, originV + cellCountZ * settings.cellSize - half);
    if (settings.plane == MovePlane::XY) position.y = v;
    else position.z = v;
    return position;
}

bool NavGrid::IsInside(int cellX, int cellZ) const {
    return cellX >= 0 && cellZ >= 0 && cellX < cellCountX && cellZ < cellCountZ;
}

bool NavGrid::IsWalkable(int cellX, int cellZ) const {
    return IsInside(cellX, cellZ) && blocked[ToIndex(cellX, cellZ)] == 0;
}

bool NavGrid::IsWalkable(XMFLOAT3 position) const {
    int cellX, cellZ;
    return WorldToCell(position, cellX, cellZ) && blocked[ToIndex(cellX, cellZ)] == 0;
}

bool NavGrid::HasLineOfWalk(XMFLOAT3 from, XMFLOAT3 to) const {
    // 平面の上で、線が通るマスを順にたどる(どちらかの端が格子の外なら行けない)
    XMFLOAT2 a = ToCellSpace(GetU(from), GetV(from, settings.plane));
    XMFLOAT2 b = ToCellSpace(GetU(to), GetV(to, settings.plane));
    int x, z, endX, endZ;
    if (!WorldToCell(from, x, z) || !WorldToCell(to, endX, endZ)) return false;
    if (!IsWalkable(x, z)) return false;

    const float infinity = 1e30f;
    float dx = b.x - a.x, dz = b.y - a.y;
    int stepX = (dx > 0.f) ? 1 : -1;
    int stepZ = (dz > 0.f) ? 1 : -1;
    float deltaX = (dx != 0.f) ? 1.f / fabsf(dx) : infinity;   // マス1つぶん進むのに必要な、線の進み具合(0〜1)
    float deltaZ = (dz != 0.f) ? 1.f / fabsf(dz) : infinity;
    float maxX = (dx > 0.f) ? (x + 1 - a.x) / dx : (dx < 0.f) ? (a.x - x) / -dx : infinity;   // 次にマスの境目を越える進み具合
    float maxZ = (dz > 0.f) ? (z + 1 - a.y) / dz : (dz < 0.f) ? (a.y - z) / -dz : infinity;

    int guard = cellCountX + cellCountZ + 4;
    while ((x != endX || z != endZ) && guard-- > 0) {
        bool moveX, moveZ;
        if (x == endX) { moveX = false; moveZ = true; }
        else if (z == endZ) { moveX = true; moveZ = false; }
        else if (fabsf(maxX - maxZ) < 1e-5f) { moveX = true; moveZ = true; }   // マスの角をちょうど通る
        else { moveX = maxX < maxZ; moveZ = !moveX; }

        // 角を通る時は、両脇のマスも通れなければ不可(壁の角をかすめない)
        if (moveX && moveZ && (!IsWalkable(x + stepX, z) || !IsWalkable(x, z + stepZ))) return false;
        if (moveX) { x += stepX; maxX += deltaX; }
        if (moveZ) { z += stepZ; maxZ += deltaZ; }
        if (!IsWalkable(x, z)) return false;
    }
    return true;
}

bool NavGrid::FindNearestWalkable(XMFLOAT3 position, XMFLOAT3& result, int maxRing) const {
    int cellX, cellZ;
    if (!WorldToCell(position, cellX, cellZ)) return false;
    if (IsWalkable(cellX, cellZ)) {
        result = CellToWorld(cellX, cellZ);
        return true;
    }

    // 自分のマスのまわりを、1マスずつ外へ広げながら探す。最初に見つかった輪の、次の輪まで調べて一番近い物を選ぶ
    float bestDistance = 1e30f;
    bool found = false;
    int foundRing = -1;
    for (int ring = 1; ring <= maxRing; ring++) {
        for (int dz = -ring; dz <= ring; dz++) {
            for (int dx = -ring; dx <= ring; dx++) {
                if (abs(dx) != ring && abs(dz) != ring) continue;   // 輪の上のマスだけ
                if (!IsWalkable(cellX + dx, cellZ + dz)) continue;
                XMFLOAT3 center = CellToWorld(cellX + dx, cellZ + dz);
                float du = center.x - GetU(position), dv = GetV(center, settings.plane) - GetV(position, settings.plane);
                float distance = du * du + dv * dv;
                if (distance < bestDistance) {
                    bestDistance = distance;
                    result = center;
                    found = true;
                }
            }
        }
        if (found) {
            if (foundRing < 0) foundRing = ring;
            else break;
        }
    }
    return found;
}
