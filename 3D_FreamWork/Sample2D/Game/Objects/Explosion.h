#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 敵を倒した位置に出る爆発のパーティクルを作る(粒が消えたら自分をDestroyする)
GameObject* CreateExplosion(Scene& scene, DirectX::XMFLOAT3 position);
