#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 波打つ半透明の水面(池)を置く。position が水面の中心で、Y が水位。当たり判定は無いので通り抜けられる(WaterRenderer。§35)
GameObject* CreateWater(Scene& scene, DirectX::XMFLOAT3 position);
