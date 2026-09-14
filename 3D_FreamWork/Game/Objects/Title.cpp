#include "Title.h"
#include "../../Engine/Input.h"
#include "../../Engine/Texture.h"
#include "../../Engine/SceneManager.h"
#include "../Scenes/GameScene.h"
#include "../../Engine/Text.h"

Title::Title()
{

}

Title::Title(std::string tag) : GameObject(tag)
{

}

void Title::Init()
{
    transform.position = {   0.0f,   0.0f , 0.0f };
    transform.scale    = { 500.0f, 500.0f , 0.0f };
    transform.rotate   = {   0.0f,   0.0f , 0.0f };
    speed = 200.0f;
}

void Title::Update(float dt)
{
    if (Input::GetKeyPress(KEY_SPACE))SceneManager::Get().ChangeScene<GameScene>();
}

void Title::Draw()
{
    Text::Draw(L"タイトル", 0, 0, 100);
}

void Title::Uninit()
{
    Image::ReleaseTexture(texID);
}