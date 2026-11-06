#pragma once
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Core/ObjectPool.h"

// 弾を出す・消す操作を全員の画面に出す管理役(見た目の無い管理役)。
// - 弾の撃ち方は誰でも「FireRequest」の出来事をホストへ送る。ホストだけが Network::Spawn で弾を作る
// - 弾は ObjectPool で使い回す。消える時(Despawn)はプールに返す
class NetworkSpawnManager : public Component {
private:
    bool is3D;                  // 3Dか2Dか(弾の見た目と、出す位置)
    ObjectPool pool;            // 弾のプール(このシーンのオブジェクトに付けたComponentが持つ)
    bool wasClient = false;     // 前のフレームでクライアントだったか(役割が変わった時に、出ている弾を片付けるため)

    DirectX::XMFLOAT3 GetMuzzle(int player) const;      // プレイヤーごとの弾を出す位置
    GameObject* CreateBullet(Scene& scene, int owner);  // 弾をプールから借りて、見た目を付け直す

public:
    static constexpr int bulletPrefab = 1;   // 弾のプレハブ番号

    explicit NetworkSpawnManager(bool spawn3D) : is3D(spawn3D) {}

    void Fire();   // 弾を撃つ(ボタンやキーから呼ぶ。ホストへ頼む)

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};