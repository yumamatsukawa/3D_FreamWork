#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 高さの画像(Assets/Terrain/heightmap.png)から作った、起伏のある地形(なだらかな丘と谷)を置く。
// corner: 地形の左手前の角(X・Zが最小の角)の位置。見た目と同じデータの当たり判定(TerrainRigidbodyComponent)も付く
GameObject* CreateHills(Scene& scene, DirectX::XMFLOAT3 corner);
