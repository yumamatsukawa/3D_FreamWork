#include "WallField.h"
#include "Wall.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render3D/MeshRenderer.h"

void CreateWallField(Scene& scene, DirectX::XMFLOAT3 center, int countPerSide) {
    const float spacing = 300.0f;                              // 柱と柱の間隔
    const DirectX::XMFLOAT3 pillarSize = { 80.0f, 300.0f, 80.0f };
    const float groundTopY = -50.0f;                           // 地面の上面(Groundと同じ)

    // 距離で省く設定(カメラから柱の表面までの距離。遠いほど軽くする)
    LodSettings lod;
    lod.drawDistance = 6000.0f;      // これより遠い柱は描かない(farZ=10000よりずっと手前)
    lod.shadowDistance = 2500.0f;    // 影を落とすのは近い柱だけ(影の範囲 Shadow::distance=3000 に合わせた)
    lod.outlineDistance = 1500.0f;   // 枠線を描くのは近い柱だけ

    // 中心(center)のまわりに、countPerSide x countPerSide 本を並べる
    float start = -spacing * (countPerSide - 1) * 0.5f;
    for (int row = 0; row < countPerSide; row++) {
        for (int col = 0; col < countPerSide; col++) {
            DirectX::XMFLOAT3 position = {
                center.x + start + spacing * col,
                groundTopY + pillarSize.y * 0.5f,
                center.z + start + spacing * row };
            GameObject* pillar = CreateWall(scene, position, pillarSize);
            auto* meshRenderer = pillar->GetComponent<MeshRenderer>();
            meshRenderer->SetOutline({ 0.05f, 0.05f, 0.1f, 1.0f }, 2.0f);
            meshRenderer->SetLod(lod);
        }
    }
}
