#pragma once
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/UI/Text.h"
#include <string>

class TextRenderer;

// Textデモの操作役。文字の大きさ・色・縁取り・太字・寄せ方・フォントの違いを並べ、キーで入力した文字をスタイルを変えて見せる。
// 動く物に付いた文字(TextRenderer)の中身も、ここから SetText で変える
class TextDemoController : public Component {
private:
    static constexpr size_t maxTyped = 14;      // 入力できる文字数
    static constexpr int styleCount = 4;        // 1〜4キーで選べるスタイルの数

    std::wstring typed = L"HELLO";              // キーで入力した文字
    int styleIndex = 0;                         // 今のスタイル(0〜3)
    TextRenderer* followerText;                 // 動く物に付いている文字(中身を毎フレーム変える)

    TextStyle GetTypedStyle() const;            // 今のスタイル番号に合う文字の見た目

public:
    explicit TextDemoController(TextRenderer* movingText) : followerText(movingText) {}

    void Update(float dt) override;
    void Draw() override;
};
