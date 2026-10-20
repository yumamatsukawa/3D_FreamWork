#pragma once
#include <DirectXMath.h>

class GameObject;

// 動く剛体(Dynamic)を、指定の位置へ戻す(向き・速さ・回る速さも0にする)。
// Transformを書き換えてもPhysXの位置で上書きされるため、PhysXの剛体とTransformの両方を直す
void ResetDemoBody(GameObject* body, DirectX::XMFLOAT3 position);
