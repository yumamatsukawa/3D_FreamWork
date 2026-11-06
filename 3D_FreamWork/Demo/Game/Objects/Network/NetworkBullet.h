#pragma once

class GameObject;

// 弾1つ分を組み立てる(ObjectPool::Rent の Setup に渡す。新しく作る時にだけ呼ばれる)。
// NetworkId と位置の同期、見た目、動きを付ける。2Dはスプライト、3Dは球
void SetupNetworkBullet(GameObject* bullet, bool is3D);

// 借りた弾の見た目を、撃った人の番号の色にする(プールの弾は前の値が残っているので、借りる度に呼ぶ)
void ApplyNetworkBulletLook(GameObject* bullet, bool is3D, int owner);