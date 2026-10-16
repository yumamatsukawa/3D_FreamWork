#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 壁を作る(動かない四角。タグ"Wall"なので、見張り役(VisionComponent)の視線をさえぎる)
GameObject* CreateWall(Scene& scene, DirectX::XMFLOAT2 position, DirectX::XMFLOAT2 size);