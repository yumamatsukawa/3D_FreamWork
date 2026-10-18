#include "Title.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../Components/TitleController.h"

GameObject* CreateTitle(Scene& scene) {
    GameObject* title = scene.CreateObject("Title");
    // 表示とBGM
    title->AddComponent<TitleController>();
    return title;
}
