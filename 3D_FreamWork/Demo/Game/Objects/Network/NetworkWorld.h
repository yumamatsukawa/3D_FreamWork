#pragma once
#include <functional>

class Scene;
class GameObject;

// 同期する物を作る関数 build を、役割(ホスト/クライアント)に合わせて作り直す役を置く。
// 置いた時に1回 build が呼ばれ、その後は役割が変わる度に、古い物を片付けてから呼び直される
GameObject* CreateNetworkWorld(Scene& scene, std::function<void()> build);