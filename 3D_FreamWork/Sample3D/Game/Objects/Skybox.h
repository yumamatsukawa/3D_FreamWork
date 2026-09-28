#pragma once
#include <string>

class Scene;
class GameObject;

// スカイボックス(カメラを中心に追従する、遠景用の巨大な立方体)を組み立てて、sceneに追加する
GameObject* CreateSkybox(Scene& scene);
