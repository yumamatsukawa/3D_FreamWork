#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

class TerrainRenderer;

// 地形デモの調べる玉の動きと表示を担当する(矢印キーで水平に動き、地面の上に乗る。足元の地面の高さと向きを文字で出す)
class TerrainDemoController : public Component {
private:
    TerrainRenderer* terrain = nullptr;   // 調べる地形
    std::wstring fontName;                // 文字のフォント(InitでText::LoadFontした名前)

    static constexpr float speed = 700.f;       // 玉の動く速さ
    static constexpr float limit = 1950.f;      // 動ける範囲(地形の中心から、XZそれぞれこの距離まで)

public:
    // 調べる地形を、CreateTerrainDemo()から渡してもらう
    void Setup(TerrainRenderer* terrainRenderer) { terrain = terrainRenderer; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
