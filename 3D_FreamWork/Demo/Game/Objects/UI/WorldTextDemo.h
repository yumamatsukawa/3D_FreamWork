#pragma once

class Scene;
class GameObject;

// WorldTextデモの本体を作る。壁と、壁に貼った看板(題名・カウント・柱に隠れる看板・小さな看板60枚)、傾く看板、ビルボードの看板と、操作役
GameObject* CreateWorldTextDemo(Scene& scene);
