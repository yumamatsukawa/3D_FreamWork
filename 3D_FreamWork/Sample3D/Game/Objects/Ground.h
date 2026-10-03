#pragma once

class Scene;
class GameObject;

// 地面(カメラの周りに敷き詰めるタイルの見た目と、見えない大きな床の当たり判定)を作る
GameObject* CreateGround(Scene& scene);
