#pragma once

class Scene;
class GameObject;
class WaterRenderer;

// 水面デモの操作役(水の色・波の高さ・波の速さを変える)を作る。water: 設定を変える水面
GameObject* CreateWaterDemo(Scene& scene, WaterRenderer* water);
