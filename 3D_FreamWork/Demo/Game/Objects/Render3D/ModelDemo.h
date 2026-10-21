#pragma once

class Scene;
class GameObject;

// Modelデモの本体を作る。同じFBX(Assets/Models/TestParts.fbx)を2体置いて(1体は動かす、1体は見本)、試しに読むロボット(robot.fbx)も置き、操作役を付ける
GameObject* CreateModelDemo(Scene& scene);
