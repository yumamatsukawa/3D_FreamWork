#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 2Dの壁(動かない四角い板)を1枚作る。center: 中心の位置、size: 幅と高さ。見た目は2Dモードのスプライト(物より奥)
GameObject* CreateDemoWall2D(Scene& scene, DirectX::XMFLOAT2 center, DirectX::XMFLOAT2 size);

// 2Dの壁で、内側の範囲(min〜max)を4枚で囲む(壁は範囲の外側に置く)
void CreateDemoArena2D(Scene& scene, DirectX::XMFLOAT2 min, DirectX::XMFLOAT2 max, float thickness = 30.f);

// 3Dの壁(動かない大きな箱)を1つ作る。center: 中心の位置、size: 幅・高さ・奥行き
GameObject* CreateDemoWall3D(Scene& scene, DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 size);
