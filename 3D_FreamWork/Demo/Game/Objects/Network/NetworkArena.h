#pragma once
#include "../../Scenes/Common/DemoRegistry.h"

class Scene;

// 位置・入力のデモの床と壁を作る(動かない物。ネットワークで同期しないので、ホストもクライアントも同じ物を置く)。
// 2Dは画面の下半分を壁で囲み、3Dは低い壁で四角く囲む
void CreateNetworkArena(Scene& scene, DemoMode mode);