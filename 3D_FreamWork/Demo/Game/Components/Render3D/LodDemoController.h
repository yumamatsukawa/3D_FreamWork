#pragma once
#include "../../../../Engine/Core/Component.h"
#include <vector>

class MeshRenderer;

// LODデモの操作役。遠い物を省く設定(Lod)のオン/オフと距離の倍率、影・枠線のオン/オフを切り替え、
// 描画命令の数・三角形の数・距離で省いた数(RenderStats)を表示する。
// 全体の設定(Lod・Shadow)を変えるので、出る時(Uninit)に既定へ戻す
class LodDemoController : public Component {
private:
    std::vector<MeshRenderer*> pillars;     // 並べた柱(影・枠線のオン/オフをまとめて入れる)

    bool lodOn = true;                      // 距離で省く設定を使うか(1キー。Lod::SetEnabled)
    float scale = 1.f;                      // 距離の倍率(上下キー。Lod::SetGlobalScale)
    bool shadowOn = true;                   // 柱が影を落とすか(2キー)
    bool outlineOn = true;                  // 柱に枠線を付けるか(3キー)

    void ApplySettings();                   // 状態を Lod・Shadow・柱のマテリアルに入れる

public:
    // 柱を足す(CreateLodDemo()から)
    void AddPillar(MeshRenderer* pillar) { pillars.push_back(pillar); }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};
