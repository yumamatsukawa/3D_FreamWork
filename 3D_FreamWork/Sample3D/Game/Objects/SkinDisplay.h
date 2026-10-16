#pragma once

class Scene;
class GameObject;

// 骨で曲げるFBXモデル(Assets/Models/TestSkin.fbx。2本の骨で曲がる細長い箱)を地面の端に置き、アニメーション "Bend" を再生する。
// ファイルが無い時は何も作らずnullptrを返す
GameObject* CreateSkinDisplay(Scene& scene);