#pragma once
#include "../../../Engine/Component.h"
#include "../../../Engine/Mesh.h"
#include <string>
#include <climits>

// 地面のタイルを、3Dカメラの周り(カメラの描画距離farZまで)に敷き詰めて描く。
// 1枚の大きな板にするとテクスチャが引き伸ばされるので、一定サイズのタイルを並べている
class GroundController : public Component {
private:
    std::wstring texturePath;          // タイルの画像
    float tileSize;                    // タイル1枚の大きさ(XZ方向)
    float topY;                        // 地面の上面の高さ
    static constexpr float tileThickness = 1.0f;   // タイルの厚み(薄い立方体で描く)

    Mesh mesh;                         // タイル1枚の形(全タイルで使い回す)
    unsigned int texID = UINT_MAX;     // 読み込んだテクスチャ

public:
    GroundController(const std::wstring& texturePath, float tileSize, float topY)
        : texturePath(texturePath), tileSize(tileSize), topY(topY) {}

    void Init() override;
    void Draw() override;
    void Uninit() override;
};
