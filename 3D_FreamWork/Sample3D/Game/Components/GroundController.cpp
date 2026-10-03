#include "GroundController.h"
#include "../../../Engine/Image.h"
#include <cmath>

void GroundController::Init() {
    // タイルの形(単位立方体)とテクスチャの読み込み
    mesh.Init(Mesh::CreateCube());
    texID = Image::LoadTexture(texturePath);
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

    // 範囲内のタイルを並べて描く(四角い範囲の角は描画距離の外なので、円の内側だけにして枚数を減らす)
    Transform tile;
    tile.scale = { tileSize, tileThickness, tileSize };
    for (int z = minZ; z <= maxZ; z++) {
        for (int x = minX; x <= maxX; x++) {
            float centerX = (x + 0.5f) * tileSize;
            float centerZ = (z + 0.5f) * tileSize;
            float dx = centerX - camera.position.x;
            float dz = centerZ - camera.position.z;
            if (dx * dx + dz * dz > range * range) continue;

            tile.position = { centerX, topY - tileThickness * 0.5f, centerZ };
            mesh.Draw(tile, camera, { 1.f, 1.f, 1.f, 1.f }, Image::GetLight());
        }
    }
}

void GroundController::Uninit() {
    // 形とテクスチャの解放
    mesh.Uninit();
    Image::ReleaseTexture(texID);
}
