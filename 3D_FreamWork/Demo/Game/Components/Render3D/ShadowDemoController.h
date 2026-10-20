#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>
#include <vector>

class MeshRenderer;
class LightComponent;

// 影のデモで操作する物(CreateShadowDemo()が作って、渡す)
struct ShadowDemoTargets {
    MeshRenderer* castSphere = nullptr;       // 影を落とすかどうかを切り替える球
    MeshRenderer* receiveBox = nullptr;       // 影を受けるかどうかを切り替える箱
    LightComponent* spotLight = nullptr;      // 影を落とすかどうかを切り替えるスポットライト
    std::vector<MeshRenderer*> outlineTargets;   // 枠線をまとめて付け外しする物
};

// 影のデモの操作を担当する(影の有効化・太陽の向き・各種の切り替え。影の設定は全体で共通なので、終わったら既定に戻す)
class ShadowDemoController : public Component {
private:
    ShadowDemoTargets targets;       // 操作する物
    float sunYaw = 30.f;             // 太陽の水平の向き(度)
    float sunElevation = 45.f;       // 太陽の高さ(度。90で真上)
    bool autoSun = true;             // 太陽が自動で回っているか
    bool sphereCasts = true;         // 球が影を落とすか
    bool boxReceives = true;         // 箱が影を受けるか
    bool spotCasts = true;           // スポットライトが影を落とすか
    bool outlined = false;           // 枠線を付けているか
    std::wstring fontName;           // 文字のフォント(InitでText::LoadFontした名前)

    void ApplySettings();            // 切り替えた内容を、物と影の設定に反映する

public:
    // 操作する物を、CreateShadowDemo()から渡してもらう
    void Setup(const ShadowDemoTargets& newTargets) { targets = newTargets; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
