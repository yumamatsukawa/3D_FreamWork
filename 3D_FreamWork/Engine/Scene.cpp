#include "Scene.h"
#include <algorithm>

void Scene::Update(float dt) {
    for (auto& obj : objects)
        if (obj->GetIsActive()) obj->Update(dt);

    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [](const auto& obj) { return !obj->GetIsActive(); }),
        objects.end());
}

void Scene::Draw() {
    for (auto& obj : objects)
        if (obj->GetIsActive()) obj->Draw();
}

void Scene::Uninit() {
    for (auto& obj : objects)
        obj->Uninit();
    objects.clear();
}
