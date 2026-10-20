#pragma once

class Scene;
class GameObject;
class TerrainRenderer;

// 地形デモの調べる玉(矢印キーで動かす。足元の地面の高さと向きを表示する)と、その操作役を作る。terrain: 調べる地形
GameObject* CreateTerrainDemo(Scene& scene, TerrainRenderer* terrain);
