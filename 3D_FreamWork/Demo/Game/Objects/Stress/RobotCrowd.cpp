#include "RobotCrowd.h"
#include "../../Components/Stress/RobotCrowdManager.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateRobotCrowd(Scene& scene) {
    // 見た目の無い管理役。ロボットは、この Component が体数に合わせて作る・消す
    GameObject* crowd = scene.CreateObject("RobotCrowd");
    crowd->AddComponent<RobotCrowdManager>();
    return crowd;
}
