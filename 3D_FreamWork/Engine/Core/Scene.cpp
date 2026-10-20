#include "Scene.h"
#include "../Render/Image.h"
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
    // パス1: UIモードのスプライト以外(3Dメッシュ・Worldスプライト・Text・ゲージなど)を、
    // drawPriorityが小さいもの(スカイボックスなど)から順に、同じdrawPriority内ではz座標が小さいものから順に描画する。
    // UIモードのスプライトは、描かずにImageの「UIキュー」へ積んでおく(後のパス2で描く)。
    // objects自体の並び順(Update順や生成順)は変えたくないので、描画用の一時リストだけ作ってソートする
    struct DrawItem {
        GameObject* obj;
        int order;   // 作った順(objectsの中の位置)
    };
    std::vector<DrawItem> drawOrder;
    drawOrder.reserve(objects.size());
    for (size_t i = 0; i < objects.size(); i++)
        if (objects[i]->GetIsActive()) drawOrder.push_back({ objects[i].get(), (int)i });

    // 同じ並びのものは作った順のまま(stable_sort)
    std::stable_sort(drawOrder.begin(), drawOrder.end(),
        [](const DrawItem& a, const DrawItem& b) {
            if (a.obj->GetDrawPriority() != b.obj->GetDrawPriority())
                return a.obj->GetDrawPriority() < b.obj->GetDrawPriority();
            return a.obj->transform.position.z < b.obj->transform.position.z;
        });

    Image::BeginUiQueue();
    for (auto& item : drawOrder) {
        Image::SetDrawOrder(item.obj->GetDrawPriority(), item.order);   // UIモードの描画を積む時の並び(drawPriority → 作った順)
        item.obj->Draw();
    }

    // パス2: 積んだUIモードのスプライトを、drawPriority → 作った順(zは使わない)に、深度を使わず最前面に描く
    Image::FlushUiQueue();

    // パス3: Direct2Dで描く文字・四角(Text::Draw・Image::DrawRect)を、UIスプライトの後に、呼んだ順に描く
    Image::FlushOverlayQueue();
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
