#pragma once
#include "../../../../Engine/Core/Component.h"
#include <string>
#include <vector>

class GameObject;

// Shapesデモの操作役。形(Mesh)とマテリアル(Material)の見本を3列に並べ、列ごとの表示のオン/オフと、形の列を回す動きを担当する。
// 見本の上に出す名前も、ここで描く(WorldToScreen3Dで画面の位置に直す)
class ShapesDemoController : public Component {
public:
    // 見本の列(奥行きごとの3列)
    enum class Row { Shapes, Spheres, Cubes };

private:
    // 見本1つ分
    struct Sample {
        GameObject* object;        // 見本の物
        Row row;                   // どの列か
        std::wstring label;        // 名前(日本語)
        std::wstring detail;       // 設定の名前(英字)
        float labelHeight;         // 物の中心から、名前を出す高さ
    };

    std::vector<Sample> samples;   // 見本の一覧
    bool rowVisible[3] = { true, true, true };   // 列ごとの表示(1・2・3キー)
    bool spinning = true;          // 形の列を回しているか(4キー)

    static int ToIndex(Row row) { return static_cast<int>(row); }

public:
    // 見本を足す(CreateShapesDemo()から)
    void AddSample(GameObject* object, Row row, const std::wstring& label, const std::wstring& detail, float labelHeight);

    void Update(float dt) override;
    void Draw() override;
};
