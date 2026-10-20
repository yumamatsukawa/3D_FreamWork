#pragma once
#include <string>
#include <functional>

class Scene;
class GameObject;

// カテゴリ一覧のボタンと説明を作る(GetCategoryList() の1件につき、ボタン1つと説明1行。デモが無いカテゴリは押せない)
GameObject* CreateDemoMenu(Scene& scene);

// 一覧の1行(ボタンと、その右の説明)を作る。row: 上から何行目か(0から数える)、rowCount: 全部で何行か(多いほど間隔を縮める)。
// 戻り値のオブジェクトに、ButtonComponent と TextRenderer(説明)が付いている
GameObject* CreateDemoMenuRow(Scene& scene, const std::wstring& name, const std::wstring& description,
    int row, int rowCount, std::function<void()> onClick);
