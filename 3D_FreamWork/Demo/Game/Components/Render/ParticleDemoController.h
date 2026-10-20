#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Render/ParticleRenderer.h"
#include <string>

// 2D版の設定を、今の側に合わせる(3Dは、速さ・重力・大きさを GetParticleScale 倍にして、use3D を true にする)
ParticleSettings AdaptParticleSettings(const ParticleSettings& settings, DemoMode mode);
float GetParticleScale(DemoMode mode);   // 2Dは1、3Dは3(3Dの方が画面に対して小さく見えるので大きくする)

// パーティクルデモの操作を担当する(キーで種類を選び、クリックした場所で爆発させる。SPACEを押している間はスロー)
class ParticleDemoController : public Component {
private:
    DemoMode mode;                            // 2D版か3D版か
    int selected = 0;                         // 選んでいる種類(0から数える)
    ParticleRenderer* campfire = nullptr;     // 出しっぱなしの焚き火(粒の数を表示するために読むだけ)
    std::wstring fontName;                    // 文字のフォント(InitでText::LoadFontした名前)

    bool GetClickPosition(DirectX::XMFLOAT3& position) const;   // クリックした場所のワールド座標(3Dは、マウスの線と地面の交点。地面に当たらなければfalse)

public:
    explicit ParticleDemoController(DemoMode demoMode) : mode(demoMode) {}

    // 粒の数を表示する、焚き火のパーティクルを、CreateParticleDemo()から渡してもらう
    void Setup(ParticleRenderer* campfireParticles) { campfire = campfireParticles; }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
