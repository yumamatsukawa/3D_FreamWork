#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include <string>
#include <vector>

class GameObject;
class TextRenderer;

// WorldUIデモの操作役。矢印キーで主人公を動かし、1〜3キーで敵にダメージを与え、SPACEで全回復し、Mキーで主人公の名前の大きさの決め方を切り替える。
// 敵のHPの持ち主はここ(ゲージは読むだけ)。名前の板は、敵の頭上の位置を画面の位置に直して描く
class WorldUiDemoController : public Component {
private:
    // 敵1体分
    struct Enemy {
        GameObject* object;     // 敵のオブジェクト
        std::wstring name;      // 頭の上に出す名前
        float hp;               // 今のHP
        float maxHp;            // 最大のHP
    };

    static constexpr float heroSpeed = 300.f;       // 主人公の速さ(1秒あたり)
    static constexpr float damageAmount = 15.f;     // 1回のダメージ
    static constexpr float alertRange = 220.f;      // 主人公がこの距離より近づくと、名前の板が赤くなる
    static constexpr float nameOffsetY = 110.f;     // 敵の位置から、名前の板までの高さ(ワールド)

    DemoMode mode;                    // 2D版か3D版か
    GameObject* hero;                 // 主人公(矢印キーで動かす)
    TextRenderer* heroText;           // 主人公の頭上の名前(Mキーで大きさの決め方を切り替える)
    std::vector<Enemy> enemies;       // 敵

public:
    WorldUiDemoController(DemoMode demoMode, GameObject* heroObject, TextRenderer* heroNameText) : mode(demoMode), hero(heroObject), heroText(heroNameText) {}

    int AddEnemy(GameObject* object, const wchar_t* name, float maxHp);   // 敵を登録する(番号を返す)
    float GetEnemyHp(int index) const { return enemies[index].hp; }       // ゲージの値の持ち主はここ
    float GetEnemyMaxHp(int index) const { return enemies[index].maxHp; }

    void Update(float dt) override;
    void Draw() override;
};
