#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

class WaterRenderer;

// 水面デモの操作を担当する(水の色・波の高さ・波の速さを変える)
class WaterDemoController : public Component {
private:
    WaterRenderer* water = nullptr;   // 水面(設定を変える)
    int colorIndex = 0;               // 選んでいる水の色(0から数える)
    std::wstring fontName;            // 文字のフォント(InitでText::LoadFontした名前)

public:
    // 設定を変える水面を、CreateWaterDemo()から渡してもらう
    void Setup(WaterRenderer* waterRenderer) { water = waterRenderer; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
