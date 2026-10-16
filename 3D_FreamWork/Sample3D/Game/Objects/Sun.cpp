#include "Sun.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../Components/ShadowManager.h"

GameObject* CreateSun(Scene& scene) {
    GameObject* sun = scene.CreateObject("Sun");

    // 影の設定(有効にする・範囲を決める)は、ShadowManagerが担当する
    sun->AddComponent<ShadowManager>();

    return sun;
}
