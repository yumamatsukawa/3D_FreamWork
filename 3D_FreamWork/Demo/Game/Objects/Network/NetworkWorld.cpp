#include "NetworkWorld.h"
#include "../../Components/Network/NetworkWorldController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateNetworkWorld(Scene& scene, std::function<void()> build) {
    GameObject* world = scene.CreateObject("NetworkWorld");
    world->AddComponent<NetworkWorldController>(std::move(build));
    return world;
}