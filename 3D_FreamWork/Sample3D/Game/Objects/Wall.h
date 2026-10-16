#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 壁を作る(動かない箱。タグ"Wall"なので、見張り役(VisionComponent)の視線をさえぎる)
GameObject* CreateWall(Scene& scene, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 size);