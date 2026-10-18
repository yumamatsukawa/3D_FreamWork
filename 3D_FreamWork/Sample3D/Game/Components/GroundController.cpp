#include "GroundController.h"
#include "../../../Engine/Render/Image.h"
#include "../../../Engine/Core/Resources.h"
#include "../../../Engine/Render/Visibility.h"
#include <cmath>

void GroundController::Init() {
    // タイルの形(単位立方体)と、テクスチャを借りる(読み込み済みなら使い回す)
    mesh.Init(Mesh::CreateCube());
    texID = Resources::LoadTexture(texturePath);
    mesh.SetTexture(Image::GetTextureView(texID));
}

void GroundController::Draw() {
    // カメラのXZ位置を中心に、描画距離に余裕を持たせた範囲に、何番目から何番目のタイルが入るかを求める
    const Camera& camera = Image::GetCamera3D();
    // farZは視線方向の距離なので、画面の左右の端では斜めに約1.24倍遠くまで見える。余裕を持って1.3倍にする
    float range = camera.farZ * 1.3f + tileSize;
    int minX = (int)floorf((camera.position.x - range) / tileSize);
    int maxX = (int)floorf((camera.position.x + range) / tileSize);
    int minZ = (int)floorf((camera.position.z - range) / tileSize);
    int maxZ = (int)floorf((camera.position.z + range) / tileSize);

    // 地面は影を受ける(Shadow が有効な時、Playerや壁の影が落ちる。無効なら今まで通りの見た目)
    Material material;
    material.receiveShadow = true;
    // 画像(bg_tile.png)がとても暗い(平均RGB約23,28,68)ので、色を掛けて明るい青灰色にする(1より大きくてよい。暗いままだと影が見えない)
    material.color = { 5.0f, 4.0f, 2.0f, 1.0f };

    // 範囲内のタイルを並べて描く(カメラに映らないタイル=後ろや横、描画距離の外は描かない)
    Transform tile;
    tile.scale = { tileSize, tileThickness, tileSize };
    for (int z = minZ; z <= maxZ; z++) {
        for (int x = minX; x <= maxX; x++) {
            float centerX = (x + 0.5f) * tileSize;
            float centerZ = (z + 0.5f) * tileSize;
            XMFLOAT3 center = { centerX, topY - tileThickness * 0.5f, centerZ };
            if (!Visibility::IsSphereInView3D(center, tileSize * 0.8f)) continue;   // 0.8 = 正方形の対角線の半分(約0.71)に余裕を足した値

            tile.position = center;
            mesh.Draw(tile, camera, material, Image::GetLight());
        }
    }
}

void GroundController::Uninit() {
    // 形の解放と、テクスチャを返す
    mesh.Uninit();
    mesh.SetTexture(nullptr);
    Resources::ReleaseTexture(texID);
    texID = UINT_MAX;
}
