#include "TitleController.h"
#include "../../Engine/Input.h"
#include "../../Engine/Text.h"
#include "../../Engine/SceneManager.h"
#include "../Scenes/GameScene.h"

void TitleController::Update(float dt) {
    if (Input::GetKeyPress(KEY_SPACE)) {
        SceneManager::Get().ChangeScene<GameScene>();
    }
}

void TitleController::Draw() {
    Text::Draw(L"タイトル", 0, 0, 100);
}
