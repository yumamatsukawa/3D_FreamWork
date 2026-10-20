#pragma once

class Scene;
class GameObject;
class LightComponent;

// 影のデモの物(球・立方体・板と箱・壁)と、操作役を作る。spotLight: 影を落とすかどうかを切り替えるスポットライト
GameObject* CreateShadowDemo(Scene& scene, LightComponent* spotLight);
