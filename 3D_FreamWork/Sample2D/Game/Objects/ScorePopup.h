#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 敵を倒した位置に出る「+10」の文字を作る(上に少し動きながら消えて、消えたら自分をDestroyする)
GameObject* CreateScorePopup(Scene& scene, DirectX::XMFLOAT3 position);
