#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;
class WaterRenderer;

// 水面に浮かんで、波に合わせて上下する赤い箱を作る。position: 水の上のXZ(Yは波に合わせて毎フレーム決まる)
GameObject* CreateBuoy(Scene& scene, WaterRenderer* water, DirectX::XMFLOAT3 position);
