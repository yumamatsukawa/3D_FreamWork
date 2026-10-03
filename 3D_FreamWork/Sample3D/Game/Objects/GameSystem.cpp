#include "GameSystem.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../Components/GameManager.h"

GameManager* CreateGameSystem(Scene& scene) {
    GameObject* obj = scene.CreateObject("GameSystem");
    // スコア・ゲームオーバーの管理(見た目は無く、管理役のComponentだけを持つ)
    return obj->AddComponent<GameManager>();
}
