#pragma once
#include <DirectXMath.h>

class Scene;
class GameObject;

// 3Dデモの共通の舞台(スカイボックス・地面・見る点の周りを回るカメラ)を作る。CreateDemoGuide より「後」に呼ぶ(ガイドが3Dカメラの設定を戻すため)
// target: 見る点、yawDegrees/pitchDegrees: 最初の向き、distance: 見る点からカメラまでの距離。戻り値はカメラのオブジェクト
GameObject* CreateDemoStage3D(Scene& scene, DirectX::XMFLOAT3 target, float yawDegrees, float pitchDegrees, float distance);
