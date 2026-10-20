#pragma once

class Scene;
class GameObject;

// UIデモの部品(スライダー+ゲージ・HPゲージとボタン・回数のボタンと文字・動く文字)をまとめて作る。戻り値は、値の持ち主のオブジェクト
GameObject* CreateUiDemo(Scene& scene);
