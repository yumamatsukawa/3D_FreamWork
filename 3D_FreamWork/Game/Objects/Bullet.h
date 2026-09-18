#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 指定した位置から、指定方向に弾を1発発射する。
// 内部でObjectPoolを使い回すので、何度呼んでもnew/deleteはほとんど発生しない
GameObject* FireBullet(Scene& scene, DirectX::XMFLOAT3 position, DirectX::XMFLOAT2 direction);
