#include "BackgroundController.h"
#include "../../../Engine/Image.h"
#include <cmath>

void BackgroundController::Init() {
    // テクスチャの読み込み
    texID = Image::LoadTexture(texturePath);
}

void BackgroundController::Draw() {
    // 画面に映る範囲(+タイル1枚分の余白)に、何番目から何番目のタイルが入るかを求める
    const Camera& camera = Image::GetCamera();
    float halfW = Graphics::width / 2.f + tileSize;
    float halfH = Graphics::height / 2.f + tileSize;
    int minX = (int)floorf((camera.position.x - halfW) / tileSize);
    int maxX = (int)floorf((camera.position.x + halfW) / tileSize);
    int minY = (int)floorf((camera.position.y - halfH) / tileSize);
    int maxY = (int)floorf((camera.position.y + halfH) / tileSize);

    // 範囲内のタイルを並べて描く
    Transform tile;
    tile.scale = { tileSize, tileSize, 1.f };
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            tile.position = { (x + 0.5f) * tileSize, (y + 0.5f) * tileSize, 0.f };
            Image::Draw(tile, texID);
        }
    }
}

void BackgroundController::Uninit() {
    // テクスチャの解放
    Image::ReleaseTexture(texID);
}
