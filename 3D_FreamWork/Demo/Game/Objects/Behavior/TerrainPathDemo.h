#pragma once

class Scene;
class GameObject;
class TerrainRenderer;
class NavGridComponent;

// 丘の高さ・坂を取り込んだ経路探索の格子(NavGridComponent)を作る。slopeDegrees: 登れる坂の角度(これより急なマスは通れない)。
// 丘(CreateDemoHills で作った地形)の全体を格子でおおう。地形の高さは SetTerrain で取り込まれる(最初のフレームの後に自動で読まれる)
NavGridComponent* CreateTerrainNavigation(Scene& scene, TerrainRenderer* terrain, float slopeDegrees);

// TerrainPathデモの本体を作る(3D版だけ)。丘の上を、PathFollowComponent で歩く緑の動く物を置く。
// hills: CreateDemoHills で作った丘(クリックの位置を Raycast で調べるため、タグ "Terrain" を付ける)。
// 格子・動く物・目的地の目印・操作役(目的地・登れる坂の角度・表示)を作る
GameObject* CreateTerrainPathDemo(Scene& scene, GameObject* hills);
