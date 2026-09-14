#pragma once
#include "../../Engine/GameObject.h"
#include "../../Engine/Image.h"

class Player : public GameObject
{
private:
    unsigned int texID = UINT_MAX;
    float speed;

    void OnTriggerStay2D(Collider2D* other) override;
public:
    Player();
    Player(std::string tag);

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};