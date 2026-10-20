#include "NavGridComponent.h"
#include "../Render/Image.h"
#include "../Render3D/TerrainRenderer.h"
#include <chrono>
#include <windows.h>
#include <cstdio>

bool NavGridComponent::debugDrawEnabled = false;

NavGridComponent::NavGridComponent(const NavGridSettings& settings) : grid(settings), pathFinder(&grid) {
}

void NavGridComponent::SetTerrain(TerrainRenderer* newTerrain) {
    terrain = newTerrain;
    if (baked) rebakeRequested = true;   // 焼いた後に渡された時は、地形を取り込み直す
}

void NavGridComponent::SetAutoRebakeInterval(float seconds) {
    autoRebakeInterval = (seconds > 0.f) ? seconds : 0.f;
    autoRebakeTimer = autoRebakeInterval;
}

void NavGridComponent::RebakeRegion(XMFLOAT3 center, XMFLOAT3 size) {
    if (!baked) return;   // 最初の焼きの時に全体が読まれる
    pendingRegions.push_back({ center, size });
}

bool NavGridComponent::TryBeginSearch() {
    if (searchesThisFrame >= maxSearchesPerFrame) return false;
    searchesThisFrame++;
    return true;
}

void NavGridComponent::Bake() {
    auto startTime = std::chrono::steady_clock::now();

    // 地形(高さ・傾き)を先に取り込む。物理の壁を探す高さの窓が、各マスの地面の高さで決まるため
    if (terrain) grid.BakeFromTerrain(*terrain);
    grid.BakeFromPhysics();

    auto endTime = std::chrono::steady_clock::now();
    lastBakeMilliseconds = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    baked = true;

    char message[256];
    sprintf_s(message, "★ NavGridComponent: 格子 %d x %d マス(cellSize %.1f)を焼きました(%.2f ミリ秒)\n",
        grid.GetCellCountX(), grid.GetCellCountZ(), grid.GetSettings().cellSize, lastBakeMilliseconds);
    OutputDebugStringA(message);
}

void NavGridComponent::Update(float dt) {
    searchesThisFrame = 0;   // 探索の回数を数え直す

    // 最初の焼き: 出したばかりの壁は次の物理の更新から見つかるので、2回目のUpdateで焼く
    updateCount++;
    if (!baked) {
        if (updateCount >= 2) Bake();
        return;
    }

    // 自動の焼き直し(動く壁用)
    if (autoRebakeInterval > 0.f) {
        autoRebakeTimer -= dt;
        if (autoRebakeTimer <= 0.f) {
            autoRebakeTimer = autoRebakeInterval;
            rebakeRequested = true;
        }
    }
    if (rebakeRequested) {
        rebakeRequested = false;
        pendingRegions.clear();   // 全体を焼き直すので、範囲は不要
        Bake();
    }

    // 範囲だけの焼き直し
    for (const auto& region : pendingRegions) grid.BakeRegionFromPhysics(region.first, region.second);
    pendingRegions.clear();
}

void NavGridComponent::RebuildDebugMesh() {
    if (blockedMeshReady) blockedMesh.Uninit();
    blockedMeshReady = false;
    blockedMeshVersion = grid.GetVersion();

    // 通れないマスと通れるマスの境目だけを線にする(1本=2頂点)
    float half = grid.GetSettings().cellSize * 0.5f;
    bool plane2D = (grid.GetSettings().plane == MovePlane::XY);
    std::vector<MeshVertex> lines;
    auto addLine = [&](XMFLOAT3 a, XMFLOAT3 b) {
        MeshVertex v{};
        v.Normal = { 0.f, 1.f, 0.f };
        v.Color = { 1.f, 1.f, 1.f, 1.f };
        v.Position = a;
        lines.push_back(v);
        v.Position = b;
        lines.push_back(v);
    };
    for (int z = 0; z < grid.GetCellCountZ(); z++) {
        for (int x = 0; x < grid.GetCellCountX(); x++) {
            if (grid.IsWalkable(x, z)) continue;
            XMFLOAT3 c = grid.CellToWorld(x, z);
            if (!plane2D) c.y += 3.f;   // 地面にめり込まないよう少し浮かせる

            // 角の座標(平面の上で、左手前・右手前・左奥・右奥)
            auto corner = [&](float sx, float sz) {
                return plane2D ? XMFLOAT3{ c.x + sx * half, c.y + sz * half, c.z } : XMFLOAT3{ c.x + sx * half, c.y, c.z + sz * half };
            };
            XMFLOAT3 c00 = corner(-1.f, -1.f), c10 = corner(1.f, -1.f), c01 = corner(-1.f, 1.f), c11 = corner(1.f, 1.f);
            if (grid.IsInside(x, z - 1) && grid.IsWalkable(x, z - 1)) addLine(c00, c10);
            if (grid.IsInside(x, z + 1) && grid.IsWalkable(x, z + 1)) addLine(c01, c11);
            if (grid.IsInside(x - 1, z) && grid.IsWalkable(x - 1, z)) addLine(c00, c01);
            if (grid.IsInside(x + 1, z) && grid.IsWalkable(x + 1, z)) addLine(c10, c11);
        }
    }
    if (lines.empty()) return;
    blockedMeshReady = blockedMesh.Init(lines, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}

void NavGridComponent::Draw() {
    if (!debugDrawEnabled || !baked) return;
    if (blockedMeshVersion != grid.GetVersion()) RebuildDebugMesh();
    if (!blockedMeshReady) return;

    // 光の影響を受けない一定の赤。頂点はワールド座標で作ってあるので、Transformは何も変えない
    static const Light fullBright{ {0.f,-1.f,0.f}, {0.f,0.f,0.f}, {1.f,1.f,1.f} };
    Transform identity;
    // 2D(XY平面)は、2Dスプライトと同じ2D(正射影)で描く。3D(XZ平面)は3Dカメラの透視
    if (grid.GetSettings().plane == MovePlane::XY) blockedMesh.DrawLines2D(identity, Image::GetCamera(), XMFLOAT4{ 1.f, 0.f, 0.f, 1.f }, fullBright);
    else blockedMesh.Draw(identity, Image::GetCamera3D(), XMFLOAT4{ 1.f, 0.f, 0.f, 1.f }, fullBright);
}

void NavGridComponent::Uninit() {
    if (blockedMeshReady) blockedMesh.Uninit();
    blockedMeshReady = false;
}
