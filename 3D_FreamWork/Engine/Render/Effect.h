#pragma once
#include "ParticleRenderer.h"

class Scene;
class GameObject;

// 使い捨てのパーティクルのエフェクトを作る(爆発など)。
// 戻り値のオブジェクトには ParticleRenderer が付いていて、粒を出していない状態で始まる。
// 続けて GetComponent<ParticleRenderer>()->Burst(個数) で粒を出す。粒がすべて消えたら、オブジェクトは自動でDestroyされる。
//   例: GameObject* effect = CreateEffect(scene, explosion, position);
//       effect->GetComponent<ParticleRenderer>()->Burst(30);
// ・Burstの前にUpdateが回ると(粒が0のまま)すぐ消えるので、作ったらすぐBurstする
// ・Play() で出し続けた場合は、Stop() して粒がすべて消えるまで残る(Stopしないと消えない)
// ・手前に描かれるよう、描画順(drawPriority)を大きくしてある
GameObject* CreateEffect(Scene& scene, const ParticleSettings& settings, DirectX::XMFLOAT3 position);
