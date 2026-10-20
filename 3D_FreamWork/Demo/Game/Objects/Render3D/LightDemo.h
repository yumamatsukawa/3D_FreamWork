#pragma once

class Scene;
class GameObject;

// マテリアルとライトのデモの操作役(太陽の向き・光のオン/オフ)を作る。point / spot: オン/オフを切り替える光のオブジェクト
GameObject* CreateLightDemo(Scene& scene, GameObject* point, GameObject* spot);
