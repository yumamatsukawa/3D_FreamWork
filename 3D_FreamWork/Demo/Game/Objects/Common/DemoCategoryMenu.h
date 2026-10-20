#pragma once

class Scene;
class GameObject;

// 機能一覧のボタンと説明を作る(今のカテゴリの機能1つにつき、ボタン1つと説明1行。今の側の版が無い機能は暗くなる)
GameObject* CreateDemoCategoryMenu(Scene& scene);
