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
    // z座標が小さい(マイナス側 = カメラに近い)ものから順に描画する。
    // objects自体の並び順(Update順や生成順)は変えたくないので、
    // 描画用の一時リストだけ作ってソートする
    std::vector<GameObject*> drawOrder;
    drawOrder.reserve(objects.size());
    for (auto& obj : objects)
        if (obj->GetIsActive()) drawOrder.push_back(obj.get());

    std::sort(drawOrder.begin(), drawOrder.end(),
        [](GameObject* a, GameObject* b) {
            return a->transform.position.z < b->transform.position.z;
        });

    for (auto* obj : drawOrder)
        obj->Draw();
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
