#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;
class GameObject;

// Queryデモの本体を作る(2D版と3D版で共有。違いは引数 mode)。
// 調べられる物(タグ Target と Decoy の、円・四角・カプセルなど。全部動かない)と、トリガー(黄色。タグ Target)を置く。
// 3D版は、地面の上に置く(線の高さは、物の真ん中)。操作役(問い合わせの選択・結果の表示)が付く
GameObject* CreateQueryDemo(Scene& scene, DemoMode mode);
