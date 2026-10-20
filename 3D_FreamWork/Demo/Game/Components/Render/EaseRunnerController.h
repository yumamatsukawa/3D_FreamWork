#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Component.h"
#include "../../../../Engine/Animation/Ease.h"
#include <DirectXMath.h>
#include <string>

class TweenComponent;
class SpriteRenderer;
class MeshRenderer;

// 補間デモの1本分(2D: 丸が左右に動く / 3D: 立方体が奥へ動く)を担当する。同じ時間をかけて、決まったEaseで動き、今の進み具合(0〜1)を文字で出す
class EaseRunnerController : public Component {
public:
    static constexpr float startZ = 0.f;     // 3D版: 動き始める位置(手前)
    static constexpr float endZ = 700.f;     // 3D版: 動く先の位置(奥)

private:
    std::wstring label;            // Easeの名前(2Dは行の左、3Dはレーンの手前に出す)
    EaseFunction ease;             // 動きの速さの変化
    DemoMode mode;                 // 2D版か3D版か
    int row;                       // 何番目か(3D版の立方体の色を決める)
    float progress = 0.f;          // 今の進み具合(TweenValueが毎フレーム入れる。OutBackなどは0〜1をはみ出す)
    TweenComponent* tween = nullptr;     // 動かす役(同じオブジェクトに付いている)
    SpriteRenderer* sprite = nullptr;    // 2D版: 丸の見た目(色を変える)
    MeshRenderer* mesh = nullptr;        // 3D版: 立方体の見た目(色を変える)
    DirectX::XMFLOAT3 baseScale{ 1.f, 1.f, 1.f };   // 3D版: 元の大きさ(やり直す時に戻す)
    std::wstring fontName;         // 文字のフォント(InitでText::LoadFontした名前)

    void Restart2D();   // 2D版: 左端から動きを最初からやり直す
    void Restart3D();   // 3D版: 手前から動きを最初からやり直す
    void Draw2D();
    void Draw3D();

public:
    EaseRunnerController(const std::wstring& easeName, EaseFunction easeFunction, DemoMode runnerMode, int runnerRow)
        : label(easeName), ease(easeFunction), mode(runnerMode), row(runnerRow) {}

    void Restart();   // 動きを最初からやり直す

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
};
