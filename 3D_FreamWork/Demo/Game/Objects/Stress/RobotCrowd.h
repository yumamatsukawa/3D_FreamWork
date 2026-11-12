#pragma once

class Scene;
class GameObject;

// 負荷テストの本体を作る。ロボットの大群を出して管理する操作役(体数・物理・描画オプションの切り替えと、FPS・描画の数の表示)
GameObject* CreateRobotCrowd(Scene& scene);
