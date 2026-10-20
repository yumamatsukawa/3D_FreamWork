#include "BackgroundController.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Core/Resources.h"
#include <cmath>

namespace {
    constexpr float backgroundZ = 2000.f;   // 背景のz(2Dモードの前後。デモの物(z=-250〜+500)より奥で、3Dカメラの描画範囲(nearZ〜farZ)の中)
}

void BackgroundController::Init() {
    // テクスチャを借りる(読み込み済みなら使い回す)
    texID = Resources::LoadTexture(texturePath);
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
            tile.position = { (x + 0.5f) * tileSize, (y + 0.5f) * tileSize, backgroundZ };
            // 2Dモードで、いちばん奥(z=+2000)に描く。UIで描くと、2D・3Dモードのスプライト(先に描く)を、後から上書きして隠してしまうため
            Image::Draw(tile, texID, XMFLOAT4(1.f, 1.f, 1.f, 1.f), SpriteSheet(), SpriteMode::World2D);
        }
    }
}

void BackgroundController::Uninit() {
    // テクスチャを返す
    Resources::ReleaseTexture(texID);
    texID = UINT_MAX;
}
