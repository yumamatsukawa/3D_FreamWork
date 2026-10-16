#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 周りを照らしながら回るランタンを作る(点光源=LightComponent の使用例。centerの周りを回る)
GameObject* CreateLantern(Scene& scene, DirectX::XMFLOAT3 center);
