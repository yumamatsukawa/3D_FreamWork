#pragma once

class Scene;
class GameObject;

// 1回だけ曲がる腕を置く場所のx(作り直す時も同じ場所)
constexpr float onceArmX = 420.f;

// 骨で曲がる腕(Assets/Models/TestSkin.fbx)を1本置いて、クリップ "Bend" を再生する(loop=falseなら1回だけ)
GameObject* CreateSkinArm(Scene& scene, float x, bool loop);

// SkinAnimデモの本体を作る。左=箱(TestAnim.fbx。Spin と Bob)、真ん中=ずっと曲がる腕、右=1回だけ曲がる腕と、操作役
GameObject* CreateSkinAnimDemo(Scene& scene);
