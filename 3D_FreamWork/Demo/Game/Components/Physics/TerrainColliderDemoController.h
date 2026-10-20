#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

class GameObject;
class TerrainRenderer;

// 地形の当たり判定デモの表示を担当する(歩く箱の高さと、その下の地面の高さを文字で出す)
class TerrainColliderDemoController : public Component {
private:
    TerrainRenderer* terrain = nullptr;   // 地形(高さを調べる)
    GameObject* walker = nullptr;         // 歩く箱(今の位置を表示する)
    std::wstring fontName;                // 文字のフォント(InitでText::LoadFontした名前)

public:
    // 表示する物を、CreateTerrainColliderDemo()から渡してもらう
    void Setup(TerrainRenderer* terrainRenderer, GameObject* walkerObject) {
        terrain = terrainRenderer;
        walker = walkerObject;
    }

    void Init() override;
    void Draw() override;
};
