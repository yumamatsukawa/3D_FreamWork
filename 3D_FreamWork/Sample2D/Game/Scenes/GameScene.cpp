#include "GameScene.h"
#include "../Objects/Player.h"
#include "../Components/GameManager.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/RigidbodyComponent.h"
#include <DirectXMath.h>

void GameScene::Init()
{
    // Colliderの実際の大きさ・位置をワイヤーフレームで表示する(デバッグ用)。
    // 不要になったらこの行を削除するかfalseにすればよい
    RigidbodyComponent::SetDebugDrawEnabled(true);

    GameObject* player = CreatePlayer(*this);

    // スコア・HP・敵の出現・ゲームオーバーを管理する係。専用のGameObjectを1つ作って持たせる
    GameObject* gameManagerObj = CreateObject("GameManager");
    GameManager* gameManager = gameManagerObj->AddComponent<GameManager>();
    gameManager->Setup(&enemyManager, player);

    // 「FireBullet」イベントを購読する。誰か(PlayerControllerなど)が
    // PublishObject("FireBullet", shooter) を呼ぶと、ここが弾を実際に生成する。
    // ラムダの中でthis(GameScene自身)をキャプチャしているので、
    // シーンが終わったら必ずUninit()でClear()すること(下記参照)
    EventBus::Get().SubscribeObject("FireBullet", [this](GameObject* shooter) {
        if (!shooter) return;

        // rotate.z = 0 の時の正面を(0, 1)(Y+=上方向)として、
        // 見た目の回転と同じ向きに回転させる(Spriteの回転と同じ符号)
        float rad = -shooter->transform.rotate.z * (DirectX::XM_PI / 180.f);
        DirectX::XMFLOAT2 dir = { sinf(rad), cosf(rad) };

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
