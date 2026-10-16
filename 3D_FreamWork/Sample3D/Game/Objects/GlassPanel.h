#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 半透明で光沢のあるガラスの板を作る(マテリアルの使用例。当たり判定は無いので通り抜けられる)
GameObject* CreateGlassPanel(Scene& scene, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 size);
