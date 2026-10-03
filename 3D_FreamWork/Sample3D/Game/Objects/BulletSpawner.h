#pragma once

class Scene;
class BulletManager;

// 弾の発射役を作る("FireBullet"が発行されると弾を出す)
BulletManager* CreateBulletSpawner(Scene& scene);
