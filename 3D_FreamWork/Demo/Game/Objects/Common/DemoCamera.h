#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 3Dデモ用のカメラを作る(右ドラッグで向き、Q/Eでズーム)。target: 見る点、yawDegrees: 向き(度)、pitchDegrees: 上下(マイナスで見下ろす)、distance: 距離
GameObject* CreateDemoCamera(Scene& scene, DirectX::XMFLOAT3 target, float yawDegrees, float pitchDegrees, float distance);
