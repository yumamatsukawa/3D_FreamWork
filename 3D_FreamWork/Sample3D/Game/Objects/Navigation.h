#pragma once

class Scene;
class GameObject;
class NavGridComponent;

// 敵が壁と急な坂を避けるための格子(経路探索の地図)を置く。壁(Wall)をすべて置いた後に呼ぶ(壁は最初のフレームの後に自動で読み込まれる)。
// hills: 起伏のある地形(CreateHillsの返り値)。高さと傾きを格子に取り込む(nullptrなら平らな地面として扱う)
NavGridComponent* CreateNavigation(Scene& scene, GameObject* hills);
