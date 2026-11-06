#include "NetworkInputDemo.h"
#include "NetworkBody.h"
#include "../../Components/Network/NetworkPlayerInputController.h"
#include "../../Components/Network/NetworkBoxController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../../Engine/Network/Network.h"

namespace {
    constexpr int firstBoxId = 20;   // 箱の NetworkId(20〜23。プレイヤー番号 + 20)

    // プレイヤーごとの色(0=赤、1=緑、2=青、3=黄)
    const DirectX::XMFLOAT4 playerColors[4] = {
        { 1.f, 0.3f, 0.3f, 1.f }, { 0.3f, 1.f, 0.4f, 1.f }, { 0.4f, 0.5f, 1.f, 1.f }, { 1.f, 0.9f, 0.3f, 1.f }
    };
}

void CreateNetworkInputDemo(Scene& scene, DemoMode mode) {
    const bool is3D = (mode == DemoMode::Mode3D);

    // プレイヤーの人数ぶん箱を置く(いない人の箱は暗く表示される)
    for (int player = 0; player < Network::maxPlayers; player++) {
        DemoBodyDesc desc;
        desc.shape = DemoShape::Block;
        desc.type = BodyType::Dynamic;
        desc.size = is3D ? 90.f : 70.f;
        desc.color = playerColors[player];
        desc.name = "NetworkPlayer";
        desc.tag = "Player";
        const float x = -300.f + 200.f * (float)player;
        desc.position = is3D ? DirectX::XMFLOAT3{ x, 0.f, 0.f } : DirectX::XMFLOAT3{ x, -195.f, 0.f };
        GameObject* box = CreateNetworkBody(scene, mode, firstBoxId + player, desc);

        // この箱の持ち主(操作するプレイヤー)
        box->GetComponent<NetworkIdentityComponent>()->SetOwnerPlayerIndex(player);

        // 3Dの箱は倒れないよう回転を固定する(Dynamicの時だけ効く)
        if (is3D) {
            if (RigidbodyComponent* rigidbody = box->GetComponent<RigidbodyComponent>()) rigidbody->SetFreezeRotation(true);
        }

        // 入力を集める役(自分の箱でだけ働く)→ 箱を動かす役(ホストだけ)の順に付ける(同じフレームで入力が届くように)
        box->AddComponent<NetworkPlayerInputController>();
        box->AddComponent<NetworkBoxController>(is3D, desc.color);
    }
}