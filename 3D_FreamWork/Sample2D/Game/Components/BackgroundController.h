#pragma once
#include "../../../Engine/Component.h"
#include <string>
#include <climits>

// 背景のタイルを画面全体に敷き詰めて描く(カメラが動くとスクロールして見える)
class BackgroundController : public Component {
private:
    std::wstring texturePath;     // タイルの画像
    float tileSize;               // タイル1枚の大きさ
    unsigned int texID = UINT_MAX;   // 読み込んだテクスチャ

public:
    BackgroundController(const std::wstring& texturePath, float tileSize)
        : texturePath(texturePath), tileSize(tileSize) {}

    void Init() override;
    void Draw() override;
    void Uninit() override;
};
