#include "NetworkPlayerInputController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Network/Network.h"

void NetworkPlayerInputController::Update(float dt) {
    // 自分が操作する箱にだけ働く(ほかのプレイヤーの箱は、その人のPCが入力を送る)
    NetworkIdentityComponent* identity = GetOwner()->GetComponent<NetworkIdentityComponent>();
    if (!identity || !identity->IsOwner()) return;
    if (Time::IsPaused()) return;   // シーン切り替え中は受け付けない

    // 入力: 移動 = WASD または矢印キー(Q・E・Tab・Esc はデモの共通キーなので使わない)
    NetworkInput input;
    if (Input::GetKeyPress(KEY_D) || Input::GetKeyPress(KEY_RIGHT)) input.moveX += 1.f;
    if (Input::GetKeyPress(KEY_A) || Input::GetKeyPress(KEY_LEFT))  input.moveX -= 1.f;
    if (Input::GetKeyPress(KEY_W) || Input::GetKeyPress(KEY_UP))    input.moveY += 1.f;
    if (Input::GetKeyPress(KEY_S) || Input::GetKeyPress(KEY_DOWN))  input.moveY -= 1.f;
    input.SetButton(jumpButton, Input::GetKeyPress(KEY_SPACE));

    // 自分の入力を渡す(押した瞬間は Network が自動で計算する)
    Network::SetLocalInput(input);
}