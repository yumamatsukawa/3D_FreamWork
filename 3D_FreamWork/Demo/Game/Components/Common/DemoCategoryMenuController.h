#pragma once
#include "../../../../Engine/Core/Component.h"
#include <vector>

struct DemoInfo;
class ButtonComponent;
class TextRenderer;

// 機能一覧の行の見た目を担当する(今の側に合わせて、ボタンを暗くしたり、説明の文字を変えたりする。Tabで側が変わっても追従する)
class DemoCategoryMenuController : public Component {
private:
    // 1行分(機能と、その行のボタン・説明の文字)
    struct Row {
        const DemoInfo* demo = nullptr;
        ButtonComponent* button = nullptr;
        TextRenderer* text = nullptr;
        int state = -1;   // 前に反映した状態(側と、押せるか。-1=まだ反映していない)
    };
    std::vector<Row> rows;

public:
    // 行を足す(CreateDemoCategoryMenu()が、行を作るたびに呼ぶ)
    void AddRow(const DemoInfo* demo, ButtonComponent* button, TextRenderer* text);

    void Update(float dt) override;
};
