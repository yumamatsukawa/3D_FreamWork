#pragma once

class Scene;
class GameObject;

// Inputデモの中身(キーボード・マウス・ゲームパッドの状態を出す表示役)を作る
GameObject* CreateInputDemo(Scene& scene);
