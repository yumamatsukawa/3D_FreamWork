#pragma once

class Scene;
class GameObject;

// LODデモの本体を作る。柱が15x15本並ぶ場所(カプセル。遠い物を描かない・影を落とさない・枠線を描かない設定つき)と、操作役
GameObject* CreateLodDemo(Scene& scene);
