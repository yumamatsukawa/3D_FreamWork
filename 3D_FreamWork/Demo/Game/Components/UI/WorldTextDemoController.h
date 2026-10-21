#pragma once
#include "../../../../Engine/Core/Component.h"
#include <vector>

class GameObject;
class WorldTextRenderer;

// WorldTextデモの操作役。B でビルボードの看板をオン/オフ、D で傾く看板の両面表示、左右キーで傾く看板を回し、
// C でカウントの看板の数字を増やし、1 で小さな看板60枚を「全部同じ文字(テクスチャを共有)」と「全部違う文字」に切り替える。
// 看板の数と共有テクスチャの数・メモリは、画面の左上に出す
class WorldTextDemoController : public Component {
private:
    static constexpr float turnSpeed = 90.f;     // 傾く看板を回す速さ(度/秒)

    WorldTextRenderer* billboardSign = nullptr;  // いつもカメラの方を向く看板(Bキー)
    WorldTextRenderer* board = nullptr;          // 傾く看板(Dキーで両面、左右キーで回す)
    WorldTextRenderer* counterSign = nullptr;    // 数字が増える看板(Cキー)
    GameObject* boardObject = nullptr;           // 傾く看板のオブジェクト
    std::vector<WorldTextRenderer*> allSigns;    // 全部の看板(数を数える)
    std::vector<WorldTextRenderer*> gridSigns;   // 小さな看板(1キーで文字を切り替える)

    bool billboardOn = true;                     // ビルボードのオン/オフ
    bool doubleSided = true;                     // 傾く看板を裏からも見えるか
    bool sameText = true;                        // 小さな看板を全部同じ文字にするか
    int count = 0;                               // カウントの数字
    float boardYaw = 25.f;                       // 傾く看板の向き(度)

    void ApplyGridText();                        // 小さな看板の文字を、今のモードに合わせる

public:
    // 看板を渡してもらう(CreateWorldTextDemo から)
    void Setup(WorldTextRenderer* billboard, WorldTextRenderer* boardSign, GameObject* boardOwner, WorldTextRenderer* counter) {
        billboardSign = billboard;
        board = boardSign;
        boardObject = boardOwner;
        counterSign = counter;
    }
    void AddSign(WorldTextRenderer* sign, bool grid = false) {   // 数える看板を登録する(grid=trueなら小さな看板)
        allSigns.push_back(sign);
        if (grid) gridSigns.push_back(sign);
    }

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
