#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;
class ParticleRenderer;

// パーティクルデモの操作役(クリックで爆発・キーで種類の切り替え)を作る。campfire: 粒の数を表示する焚き火のパーティクル、mode: 2D版か3D版か
GameObject* CreateParticleDemo(Scene& scene, ParticleRenderer* campfire, DemoMode mode);
