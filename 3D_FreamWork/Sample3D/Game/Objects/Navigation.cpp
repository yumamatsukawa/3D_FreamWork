#include "Navigation.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Behavior/NavGridComponent.h"
#include "../../../Engine/Render3D/TerrainRenderer.h"

NavGridComponent* CreateNavigation(Scene& scene, GameObject* hills) {
    GameObject* navigation = scene.CreateObject("Navigation");

    // 格子の範囲: Playerが歩き回る範囲(壁と丘をおおう9000四方)。外では敵は経路を探せず、止まる
    NavGridSettings settings;
    settings.origin = { -3000.0f, 0.0f, -3000.0f };
    settings.size = { 9000.0f, 9000.0f };
    settings.cellSize = 50.0f;           // 敵の半径(40)くらい
    settings.plane = MovePlane::XZ;
    settings.agentRadius = 40.0f;        // 敵は半径40のカプセル
    settings.baseHeight = -50.0f;        // 地形の無い所の地面の高さ(Groundの上面)
    auto* nav = navigation->AddComponent<NavGridComponent>(settings);   // Wall(タグ)は最初のフレームの後に自動で読まれる

    // 丘の高さ・傾きを取り込む(急な斜面は通れないマスになる)
    if (hills) nav->SetTerrain(hills->GetComponent<TerrainRenderer>());
    return nav;
}
