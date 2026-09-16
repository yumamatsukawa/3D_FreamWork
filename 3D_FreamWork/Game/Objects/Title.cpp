#include "Title.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../Components/TitleController.h"

GameObject* CreateTitle(Scene& scene) {
    GameObject* title = scene.CreateObject("Title");
    title->AddComponent<TitleController>();
    return title;
}
