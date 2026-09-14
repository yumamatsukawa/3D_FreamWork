#include "Enemy.h"

#include "../../Engine/Input.h"

Enemy::Enemy()
{
}

Enemy::Enemy(std::string tag) : GameObject(tag)
{
}

void Enemy::Init()
{
    transform.position = { 100.0f,   0.0f , 0.0f };
    transform.scale    = { 100.0f, 100.0f , 0.0f };
    transform.rotate   = {   0.0f,   0.0f , 0.0f };
    speed = 200.0f;
    
    // テクスチャの設定
    texID = Image::LoadTexture(L"Assets/player.png", 8, 2);

    // スプライトシートの位置指定
    Image::SetSpriteIndex(texID, 5);

    AddCollider<BoxCollider2D>(true, transform.scale);
}

void Enemy::Update(float dt)
{
    Image::SetColor(texID, 1.0f, 0.0f, 1.0f, 1.0f);
}

void Enemy::Draw()
{
    // 描画
    Image::Draw(transform, texID);
}

void Enemy::Uninit()
{
    Image::ReleaseTexture(texID);
}

void Enemy::OnTriggerEnter2D(Collider2D* other)
{

}