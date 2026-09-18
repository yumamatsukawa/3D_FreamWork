#pragma once
#include <vector>
#include <memory>
#include <string>
#include "GameObject.h"

// シーン = GameObjectを管理する入れ物(UnityのSceneに相当)。
// Scene自身は「何かを描画される存在」ではないので、GameObjectは継承しない。
class Scene
{
protected:
    std::vector<std::unique_ptr<GameObject>> objects;

    // Update中にCreateObjectされたものを、ここに一旦貯めておく。
    // Update中のobjectsへ直接push_backすると、今回っているループのイテレータが
    // 壊れて即クラッシュするため、ループの外側(Update末尾)でまとめてobjectsへ移す。
    std::vector<std::unique_ptr<GameObject>> pendingObjects;
    bool isUpdating = false;  // 今Update()の実行中かどうか

public:
    virtual ~Scene() = default;

    // シーンが始まる時に1回呼ばれる。TitleScene/GameSceneなど各シーンでoverrideして使う
    virtual void Init() {}
    // シーンが終わる時に1回呼ばれる。追加の後片付けをしたい場合はoverrideし、
    // 必ず最後にScene::Uninit()を呼んでGameObjectの後片付けをすること
    virtual void Uninit();

    void Update(float dt);
    void Draw();

    // シーンに新しいGameObjectを追加する。
    // 中身(見た目や動き)は、戻り値に対してAddComponentで組み立てる。
    // 例: auto* player = scene.CreateObject("Player", "Player");
    //     player->AddComponent<SpriteRenderer>();
    // Update中に呼ばれた場合はpendingObjectsに、それ以外(Init中など)はobjectsに
    // 直接追加する。戻り値のポインタは、どちらに居てもGameObject自体の実体は
    // 動かないので(unique_ptrの中身なので)、以後ずっと有効なまま使ってよい。
    GameObject* CreateObject(std::string name = "", std::string tag = "") {
        auto obj = std::make_unique<GameObject>(std::move(name), std::move(tag));
        GameObject* ptr = obj.get();
        ptr->SetScene(this);
        if (isUpdating) pendingObjects.push_back(std::move(obj));
        else objects.push_back(std::move(obj));
        return ptr;
    }
};
