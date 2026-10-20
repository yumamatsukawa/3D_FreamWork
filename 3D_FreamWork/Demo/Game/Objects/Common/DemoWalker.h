#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;
class TerrainRenderer;

// 地形の上を歩く箱(WASDで動く。カメラが付いてくる)を作る。terrain: 歩く地形(スタート地点の高さを調べる。無ければ高さ100)、
// startXZ: スタート地点のワールドのXZ
GameObject* CreateDemoWalker(Scene& scene, TerrainRenderer* terrain, DirectX::XMFLOAT2 startXZ);
