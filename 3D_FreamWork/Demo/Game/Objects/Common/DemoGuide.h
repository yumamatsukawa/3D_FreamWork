#pragma once
#include "../../Components/Common/DemoGuideController.h"

class Scene;
class GameObject;

// デモ画面の説明(左上の4行)・戻るボタン(左下。Escキーと同じ)・2D/3D切り替えボタン(右下。Tabキーと同じ)を作る。
// screen: どの画面か(デモ本体は既定のままでよい。カテゴリ一覧は Top、機能一覧は List)。戻る先は画面で決まる
GameObject* CreateDemoGuide(Scene& scene, const DemoGuideInfo& info, DemoScreen screen = DemoScreen::Demo);
