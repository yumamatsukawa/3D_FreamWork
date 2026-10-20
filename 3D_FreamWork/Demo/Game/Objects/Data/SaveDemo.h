#pragma once

class Scene;
class GameObject;

// Saveデモの本体を作る(2Dと3Dの区別が無い機能)。
// 整数・小数・文字を SaveData に書いて保存・読み込みする操作役と、同じ操作のボタン(Z/X/C/S/L/D と スロット1〜3)を置く
GameObject* CreateSaveDemo(Scene& scene);