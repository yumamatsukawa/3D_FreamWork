#include "GameScene.h"
#include "../Objects/Player.h"
#include "../Objects/Enemy.h"
#include "../Objects/Cube.h"
#include "../Objects/Sphere.h"
#include "../Objects/Skybox.h"
#include "../Objects/Ground.h"
#include "../../Engine/EventBus.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/MeshRenderer.h"
#include <DirectXMath.h>

void GameScene::Init()
{
    // ★ box.pngは仮のテクスチャ。専用のスカイボックス用画像が用意でき次第差し替える
    CreateSkybox(*this);

    CreateGround(*this);
    CreatePlayer(*this);
    CreateEnemy(*this);
    CreateCube(*this);
    CreateSphere(*this);

    // 「FireBullet」イベントを購読する。誰か(PlayerControllerなど)が
    // PublishObject("FireBullet", shooter) を呼ぶと、ここが弾を実際に生成する。
    // ラムダの中でthis(GameScene自身)をキャプチャしているので、
    // シーンが終わったら必ずUninit()でClear()すること(下記参照)
    EventBus::Get().SubscribeObject("FireBullet", [this](GameObject* shooter) {
        if (!shooter) return;

        // rotate.z = 0 の時の正面を(0, -1)(画面の上方向)として、
        // 見た目の回転と同じ向きに回転させる(Sprite/Colliderと同じ符号)
        float rad = -shooter->transform.rotate.z * (DirectX::XM_PI / 180.f);
        DirectX::XMFLOAT2 dir = { sinf(rad), -cosf(rad) };

        bulletManager.Fire(*this, shooter->transform.position, dir);
    });
}

void GameScene::Uninit()
{
    // 上のラムダがthis(このGameScene)をキャプチャしたまま残らないよう、
    // 登録を消しておく(消さないと、次のシーンで誰かがFireBulletを発行した時に
    // 既に破棄されたthisを触ってしまう=dangling pointerのバグになる)
    EventBus::Get().Clear();

    Scene::Uninit();
}
