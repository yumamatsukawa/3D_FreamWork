#pragma once

class Scene;
class GameObject;

// 太陽(平行光源)の影を使うための管理役を置く(Shadow の使用例。置くとこのシーンの間だけ、影が有効になる)
GameObject* CreateSun(Scene& scene);
