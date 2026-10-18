#pragma once

class Scene;
class NavGridComponent;

// 敵が壁を避けるための格子(経路探索の地図)を置く。壁(Wall)をすべて置いた後に呼ぶ(壁は最初のフレームの後に自動で読み込まれる)
NavGridComponent* CreateNavigation(Scene& scene);
