#pragma once
#include <vector>
#include <functional>
#include "GameObject.h"
#include "Scene.h"

// 同じ種類のGameObjectを使い回すためのプール。
// 弾・エフェクトなど「頻繁に出しては消す」オブジェクトのnew/delete回数を減らす。
//
// 使い方:
//   ObjectPool bulletPool;
//
//   auto setupBullet = [](GameObject* obj) {
//       obj->AddComponent<SpriteRenderer>(L"Assets/bullet.png");
//   };
//
//   // シーン開始時にあらかじめ用意しておく(任意。無くてもRentが自動で作る)
//   bulletPool.Prewarm(scene, 20, setupBullet);
//
//   // 発射する時
//   GameObject* bullet = bulletPool.Rent(scene, setupBullet);
//   bullet->transform.position = player->transform.position;  // 位置などは毎回リセットすること
//
//   // 消えた時(画面外に出た、敵に当たった等)
//   bulletPool.Return(bullet);
//
// 注意: Rentで借りたGameObjectは前回使った時の値(position, colorなど)が残っている。
//       必要な値は借りた直後に毎回セットし直すこと。
class ObjectPool {
public:
    // GameObjectを1つ組み立てる処理(AddComponentなど)。新規作成時にだけ呼ばれる
    using Setup = std::function<void(GameObject*)>;

    // あらかじめcount個ぶんGameObjectを作って、非アクティブ(非表示)にしておく
    void Prewarm(Scene& scene, int count, const Setup& setup) {
        for (int i = 0; i < count; i++) {
            GameObject* obj = scene.CreateObject("Pooled");
            setup(obj);
            obj->SetActive(false);
            pool.push_back(obj);
        }
    }

    // プールから1個借りてアクティブにして返す。空きが無ければ新しく作って追加する
    GameObject* Rent(Scene& scene, const Setup& setup) {
        for (auto* obj : pool) {
            if (!obj->GetIsActive()) {
                obj->SetActive(true);
                return obj;
            }
        }

        // 空きが無かった → 新しく作って、そのままプールに加える(プールが自動で育つ)
        GameObject* obj = scene.CreateObject("Pooled");
        setup(obj);
        obj->SetActive(true);
        pool.push_back(obj);
        return obj;
    }

    // 使い終わったオブジェクトを非アクティブに戻す(Destroyはしない = また使うので生かしておく)
    void Return(GameObject* obj) {
        obj->SetActive(false);
    }

    int GetPoolSize() const { return (int)pool.size(); }

private:
    std::vector<GameObject*> pool;
};
