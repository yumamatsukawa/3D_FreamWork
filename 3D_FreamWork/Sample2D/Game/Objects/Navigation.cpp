#include "Navigation.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Behavior/NavGridComponent.h"

NavGridComponent* CreateNavigation(Scene& scene) {
    GameObject* navigation = scene.CreateObject("Navigation");

    // 格子の範囲: Playerが歩き回る範囲(原点のまわり6000四方)。外では敵は経路を探せず、止まる
    NavGridSettings settings;
    settings.origin = { -3000.0f, -3000.0f, 0.0f };
    settings.size = { 6000.0f, 6000.0f };
    settings.cellSize = 50.0f;           // 敵の大きさ(一辺100)の半分
    settings.plane = MovePlane::XY;
    settings.agentRadius = 50.0f;        // 敵は一辺100の四角
    return navigation->AddComponent<NavGridComponent>(settings);   // Wall(タグ)は最初のフレームの後に自動で読まれる
}
