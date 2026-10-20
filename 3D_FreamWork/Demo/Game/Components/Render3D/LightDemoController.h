#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>

class GameObject;

// マテリアルとライトのデモの操作を担当する(太陽の向き・光のオン/オフ。点光源とスポットライトの動きは、それぞれのオブジェクトのTweenが担当)
class LightDemoController : public Component {
private:
    GameObject* pointLight = nullptr;   // 点光源のオブジェクト(オン/オフを切り替える)
    GameObject* spotLight = nullptr;    // スポットライトのオブジェクト(オン/オフを切り替える)
    float sunYaw = 0.f;                 // 太陽の水平の向き(度)
    float sunElevation = 63.f;          // 太陽の高さ(度。90で真上、小さいほど低い)
    bool sunOn = true;                  // 平行光源が点いているか
    std::wstring fontName;              // 文字のフォント(InitでText::LoadFontした名前)

    void ApplySun();   // 太陽の向きと明るさを、Image::GetLight() に入れる

public:
    // 切り替える光のオブジェクトを、CreateLightDemo()から渡してもらう
    void Setup(GameObject* point, GameObject* spot) { pointLight = point; spotLight = spot; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
