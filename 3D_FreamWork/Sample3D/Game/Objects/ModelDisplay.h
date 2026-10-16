#pragma once

class Scene;
class GameObject;

// FBXモデルを地面の端に1つ置く。アニメーション付きの Assets/Models/TestAnim.fbx があればそれを再生し、
// 無ければ Assets/Models/TestParts.fbx(静止)を置く。どちらも無い時は何も作らずnullptrを返す
GameObject* CreateModelDisplay(Scene& scene);