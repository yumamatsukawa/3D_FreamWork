#include "NetworkArena.h"
#include "../Common/DemoWall.h"

void CreateNetworkArena(Scene& scene, DemoMode mode) {
    if (mode == DemoMode::Mode2D) {
        // 上は説明と操作の板なので、下半分に壁で囲んだ場所を作る
        CreateDemoArena2D(scene, { -560.f, -330.f }, { 560.f, -60.f });
    }
    else {
        CreateDemoWall3D(scene, { 0.f, 0.f, 520.f }, { 1100.f, 100.f, 40.f });
        CreateDemoWall3D(scene, { 0.f, 0.f, -520.f }, { 1100.f, 100.f, 40.f });
        CreateDemoWall3D(scene, { 570.f, 0.f, 0.f }, { 40.f, 100.f, 1000.f });
        CreateDemoWall3D(scene, { -570.f, 0.f, 0.f }, { 40.f, 100.f, 1000.f });
    }
}