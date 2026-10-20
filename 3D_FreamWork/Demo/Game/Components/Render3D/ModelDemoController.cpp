#include "ModelDemoController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/ModelRenderer.h"
#include "../../../../Engine/UI/Text.h"
#include <cmath>
#include <cstdio>
#include <string>

namespace {
    constexpr float topSpinSpeed = 90.f;     // Topが回る速さ(度/秒)
    constexpr float bobHeight = 40.f;        // Topが上下に動く幅
    constexpr float pulseAmount = 0.5f;      // Topの大きさが変わる幅(元の大きさの何割か)
    constexpr float modelSpinSpeed = 45.f;   // モデル全体が回る速さ(度/秒)
    constexpr float scaleSteps[3] = { 1.f, 1.5f, 0.5f };   // モデル全体の大きさ(5キーで切り替え)
    constexpr float listX = -610.f;          // 状態の一覧の左端
    constexpr float listY = 192.f;           // 一覧の1行目(説明の板の下)
    constexpr float listGap = 24.f;          // 行の間隔

    // 英字だけの名前(パーツの名前)を、文字を描く関数に渡せる形にする
    std::wstring ToWide(const std::string& text) {
        return std::wstring(text.begin(), text.end());
    }

    // 色
    const DirectX::XMFLOAT4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
    const DirectX::XMFLOAT4 yellow = { 1.0f, 0.9f, 0.4f, 1.0f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.0f, 1.0f };
    const DirectX::XMFLOAT4 gray = { 0.8f, 0.8f, 0.8f, 1.0f };

    // モデルの上に名前を出す
    void DrawLabel(ModelRenderer* renderer, const wchar_t* label) {
        if (!renderer) return;
        DirectX::XMFLOAT3 head = renderer->GetOwner()->transform.position;
        head.y += 310.f;
        const DirectX::XMFLOAT3 screen = Image::WorldToScreen3D(head);
        if (screen.z <= 0.f) return;
        Text::Draw(label, screen.x, screen.y, MakeDemoTextStyle(18.f, yellow, TextAlign::Center));
    }
}

void ModelDemoController::Update(float dt) {
    if (!moving) return;
    time += dt;

    // Topパーツの元の位置と大きさを覚える(動かす前の値を基準にするため)。
    // Setup は Init より後に呼ばれるので、Init ではなく最初の Update で1回だけ行う
    if (!hasTop) {
        if (const Transform* top = moving->GetNodeTransform("Top")) {
            hasTop = true;
            topPosition = top->position;
            topScale = top->scale;
        }
    }

    // 1〜5キー: 動きの切り替え
    if (Input::GetKeyDown(KEY_1)) topSpin = !topSpin;
    if (Input::GetKeyDown(KEY_2)) topBob = !topBob;
    if (Input::GetKeyDown(KEY_3)) topPulse = !topPulse;
    if (Input::GetKeyDown(KEY_4)) modelSpin = !modelSpin;
    if (Input::GetKeyDown(KEY_5)) scaleIndex = (scaleIndex + 1) % 3;

    // Topパーツの動き(名前で探したパーツのTransformを書き換えると、そのパーツと子のパーツが動く)
    if (hasTop) {
        Transform* top = moving->GetNodeTransform("Top");
        if (top) {
            if (topSpin) top->rotate.y += topSpinSpeed * dt;
            top->position.y = topPosition.y + (topBob ? bobHeight * sinf(time * 3.f) : 0.f);
            const float pulse = topPulse ? 1.f + pulseAmount * sinf(time * 3.f) : 1.f;
            top->scale = { topScale.x * pulse, topScale.y * pulse, topScale.z * pulse };
        }
    }

    // モデル全体の動き(置いたオブジェクトのTransform)
    Transform& transform = moving->GetOwner()->transform;
    if (modelSpin) transform.rotate.y += modelSpinSpeed * dt;
    const float scale = scaleSteps[scaleIndex];
    transform.scale = { scale, scale, scale };
}

void ModelDemoController::Draw() {
    DrawLabel(moving, L"動かす方");
    DrawLabel(reference, L"動かさない見本");

    wchar_t buf[160];
    float y = listY;
    auto line = [&](const std::wstring& text, const DirectX::XMFLOAT4& color) {
        Text::Draw(text, listX, y, MakeDemoTextStyle(18.f, color));
        y -= listGap;
    };

    // 読み込んだモデルの情報(Model: みんなで共有するデータ)
    const std::shared_ptr<Model> model = moving ? moving->GetModel() : nullptr;
    if (!model) {
        line(L"モデルを読み込めませんでした(Assets/Models/TestParts.fbx)", yellow);
        return;
    }
    line(L"FBX: " + model->GetPath(), cyan);
    swprintf_s(buf, L"パーツ数 %d   半径 %.0f(GetBoundingRadius)", model->GetNodeCount(), model->GetBoundingRadius());
    line(buf, cyan);
    for (int i = 0; i < model->GetNodeCount(); i++) {
        const ModelNode& node = model->GetNode(i);
        const std::wstring parent = (node.parent >= 0) ? ToWide(model->GetNode(node.parent).name) : std::wstring(L"なし");
        swprintf_s(buf, L"  [%d] %ls   親: %ls   位置 (%.0f, %.0f, %.0f)", i, ToWide(node.name).c_str(), parent.c_str(),
            node.position.x, node.position.y, node.position.z);
        line(buf, cyan);
    }
    line(std::wstring(L"2体のモデルのデータ: ") + (reference && reference->GetModel() == model ? L"同じ物を共有" : L"別々"), gray);
    y -= 6.f;

    // 動きの状態
    line(std::wstring(L"[1] Top を回す: ") + (topSpin ? L"ON" : L"OFF"), white);
    line(std::wstring(L"[2] Top を上下に動かす: ") + (topBob ? L"ON" : L"OFF"), white);
    line(std::wstring(L"[3] Top の大きさを変える: ") + (topPulse ? L"ON" : L"OFF"), white);
    line(std::wstring(L"[4] モデル全体を回す: ") + (modelSpin ? L"ON" : L"OFF"), white);
    swprintf_s(buf, L"[5] モデル全体の大きさ: %.1f倍", scaleSteps[scaleIndex]);
    line(buf, white);
    line(hasTop ? L"GetNodeTransform(\"Top\") で動かす" : L"Topパーツが見つかりません", gray);
}
