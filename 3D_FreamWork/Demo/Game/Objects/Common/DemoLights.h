#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 左右に往復する点光源(オレンジの光る玉。周り全体を照らす)を作る。center: 往復の中心の位置
GameObject* CreateDemoPointLight(Scene& scene, DirectX::XMFLOAT3 center);

// 左右に往復するスポットライト(白い光る玉。下の斜め前へ円すい形に照らす)を作る。center: 往復の中心の位置。
// castShadow: trueなら影を落とす(Shadow が有効な時に、影ができる)
GameObject* CreateDemoSpotLight(Scene& scene, DirectX::XMFLOAT3 center, bool castShadow);
