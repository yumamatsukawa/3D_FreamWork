#pragma once

class Scene;
class GameObject;

// 見張り役を作る(ゆっくり回って、視野にtargetが入ると赤くなる。壁(タグ"Wall")の陰は見えない)
GameObject* CreateGuard(Scene& scene, GameObject* target);