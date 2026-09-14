#pragma once
#include "../../Engine/GameObject.h"
#include "../../Engine/Image.h"

class Title : public GameObject
{
private:
    unsigned int texID = UINT_MAX;
    float speed;
public:
    Title();
    Title(std::string tag);

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    void Uninit() override;
};