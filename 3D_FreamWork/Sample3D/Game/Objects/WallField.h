#pragma once
#include <DirectXMath.h>

class Scene;

// 細い柱を格子状に何本も置く(LODと描画カウンタを測るための場所)。柱は Wall と同じ(動かない・影を落とす・枠線あり)で、遠いほど省く距離を設定してある
void CreateWallField(Scene& scene, DirectX::XMFLOAT3 center, int countPerSide);
