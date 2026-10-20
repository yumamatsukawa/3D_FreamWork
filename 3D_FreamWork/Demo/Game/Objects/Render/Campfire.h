#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include <DirectXMath.h>

class Scene;
class GameObject;

// 出しっぱなしの焚き火(炎の粒。戻り値のオブジェクトに ParticleRenderer が付いている。煙は別のオブジェクトで出す)を作る。
// mode が3Dなら、粒を3D空間に出す(大きさも3D用に大きくする)
GameObject* CreateCampfire(Scene& scene, DirectX::XMFLOAT3 position, DemoMode mode);
