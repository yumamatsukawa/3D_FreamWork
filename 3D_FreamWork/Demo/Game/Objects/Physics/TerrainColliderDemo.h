#pragma once

class Scene;
class GameObject;
class TerrainRenderer;

// 地形の当たり判定デモの表示役(歩く箱の高さと、足元の地面の高さを出す)を作る。terrain: 歩く地形、walker: 歩く箱
GameObject* CreateTerrainColliderDemo(Scene& scene, TerrainRenderer* terrain, GameObject* walker);
