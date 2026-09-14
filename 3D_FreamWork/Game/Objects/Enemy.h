#pragma once
#include "../../Engine/GameObject.h"
#include "../../Engine/Image.h"

class Enemy : public GameObject
{
private:
    unsigned int texID = UINT_MAX;
    float speed;

    void OnTriggerEnter2D(Collider2D* other) override;
public:
    Enemy();
    Enemy(std::string tag);

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};