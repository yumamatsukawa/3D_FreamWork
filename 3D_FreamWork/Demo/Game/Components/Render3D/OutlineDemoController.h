#pragma once
#include "../../../../Engine/Core/Component.h"

class MeshRenderer;

// Outlineデモの操作役。枠線(outline)・壁越しの枠線(seeThrough)・縁の光(rim)をオン/オフし、太さや強さを変える。
// 3つの物は CreateOutlineDemo() から渡してもらい、名前と状態の一覧もここで描く
class OutlineDemoController : public Component {
private:
    MeshRenderer* outlined = nullptr;       // 枠線を付ける立方体
    MeshRenderer* hidden = nullptr;         // 壁の後ろに隠れた球(壁越しの枠線を見せる)
    MeshRenderer* rimmed = nullptr;         // 縁の光を付ける球

    bool outlineOn = true;                  // 枠線を付けるか(1キー)
    bool seeThroughOn = true;               // 壁越しにも枠線を出すか(2キー)
    bool rimOn = true;                      // 縁の光を付けるか(3キー)
    float outlineWidth = 4.f;               // 枠線の太さ(ピクセル。上下キー)
    float rimStrength = 1.5f;               // 縁の光の強さ(左右キー)

    void ApplySettings();                   // 状態を3つの物のマテリアルに入れる

public:
    // 3つの物を、CreateOutlineDemo()から渡してもらう
    void Setup(MeshRenderer* outlinedMesh, MeshRenderer* hiddenMesh, MeshRenderer* rimmedMesh) {
        outlined = outlinedMesh;
        hidden = hiddenMesh;
        rimmed = rimmedMesh;
    }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
