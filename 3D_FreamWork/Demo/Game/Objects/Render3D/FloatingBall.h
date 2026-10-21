#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;
class WaterRenderer;

// 物理(Dynamic)で、浮力で水に浮かぶ赤い球を作る。position: 最初の位置(水の上から落とせる)、diameter: 球の直径
GameObject* CreateFloatingBall(Scene& scene, WaterRenderer* water, DirectX::XMFLOAT3 position, float diameter);
