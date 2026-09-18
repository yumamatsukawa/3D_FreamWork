#include "Scene.h"
#include <algorithm>

void Scene::Update(float dt) {
    isUpdating = true;  // この間のCreateObjectはpendingObjects行きにする

    for (auto& obj : objects)
        if (obj->GetIsActive()) obj->Update(dt);

    isUpdating = false;

    // Destroy()された(非アクティブなだけ、ではない)オブジェクトだけを実際に破棄する。
    // 破棄前にUninit()を呼び、テクスチャ解放やCollider登録解除などの後片付けをする
    for (auto& obj : objects)
        if (obj->GetIsDestroyed()) obj->Uninit();

    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [](const auto& obj) { return obj->GetIsDestroyed(); }),
        objects.end());

    // Update中に生成されたオブジェクトを、ここでまとめてobjectsへ移す
    for (auto& obj : pendingObjects)
        objects.push_back(std::move(obj));
    pendingObjects.clear();
}

void Scene::Draw() {
    for (auto& obj : objects)
        if (obj->GetIsActive()) obj->Draw();
}

void Scene::Uninit() {
    for (auto& obj : objects)
        obj->Uninit();
    objects.clear();

    // 通常はUpdate末尾で空になるはずだが、念のため
    for (auto& obj : pendingObjects)
        obj->Uninit();
    pendingObjects.clear();
}
