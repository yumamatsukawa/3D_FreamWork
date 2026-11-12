#pragma once

class Scene;
class GameObject;

// 大群の中のロボットを1体作る(Assets/Models/robot.fbx。床の上の (x, z) に置き、ランダムに歩き回る)
// physics: trueなら、カプセルの剛体(Dynamic。倒れないよう回転ロック)を付けて、ほかのロボットとぶつかる
GameObject* CreateCrowdRobot(Scene& scene, float x, float z, bool physics);
