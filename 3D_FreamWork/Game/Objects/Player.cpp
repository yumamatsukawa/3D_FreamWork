#include "Player.h"

#include "../../Engine/Input.h"

Player::Player()
{
}

Player::Player(std::string tag) : GameObject(tag)
{
}

void Player::Init()
{
    transform.position = {   0.0f,   0.0f , 0.0f };
    transform.scale    = { 100.0f, 100.0f , 1.0f };
    transform.rotate   = {   0.0f,   0.0f , 0.0f };
    speed = 200.0f;
    
    // テクスチャの設定
    texID = Image::LoadTexture(L"Assets/player.png", 8, 2);

    // スプライトシートの位置指定
    Image::SetSpriteIndex(texID, 5);

    // コライダーのtag設定
    AddCollider<CircleCollider2D>(true, 50.0f);
}

void Player::Update(float dt)
{
    Image::SetColor(texID, 1.0f, 1.0f, 1.0f, 1.0f);
    DirectX::XMFLOAT2 mousePos = Input::GetMousePosition();

    if (Input::GetKeyPress(MOUSE_LEFT))
    {
        if (IsPointInBox( mousePos, transform))
        {
            Image::SetColor(texID, 1.0f, 0.0f, 0.0f, 0.5f );
        }
    }

    // 移動処理
    if (Input::GetKeyPress(KEY_W)) transform.position.y -= speed * dt;
    if (Input::GetKeyPress(KEY_S)) transform.position.y += speed * dt;
    if (Input::GetKeyPress(KEY_A)) transform.position.x -= speed * dt;
    if (Input::GetKeyPress(KEY_D)) transform.position.x += speed * dt;
    if (Input::GetKeyPress(KEY_Q)) transform.rotate.z -= speed * dt;
    if (Input::GetKeyPress(KEY_E)) transform.rotate.z += speed * dt;
}

void Player::Draw()
{
    // 描画
    Image::Draw(transform, texID);
}

void Player::Uninit()
{
    Image::ReleaseTexture(texID);
}

void Player::OnTriggerStay2D(Collider2D* other)
{
    if (other->GetTag() == "Enemy")
    {
        Image::SetColor(texID, 1.0f, 0.0f, 0.0f, 1.0f);
    }
}