#pragma once
#include "SceneTransition.h"
#include <DirectXMath.h>

// 暗転 → シーンを切り替える → 明転、のシーン切り替え(画面全体を1色で塗る)。
//
// 使い方:
//   SceneManager::Get().ChangeScene<GameScene>(FadeTransition());                    // 黒で、0.5秒で暗転・0.5秒で明転
//   SceneManager::Get().ChangeScene<GameScene>(FadeTransition({ 1.f, 1.f, 1.f, 1.f }, 1.f));  // 白で、1秒ずつ
//
// - 色は color の x,y,z だけを使う(w は使わない。切り替える時は必ず画面全体を塗りつぶす)
// - duration は覆う時間と開く時間(どちらも同じ秒数)。0以下なら一瞬で切り替わる
class FadeTransition : public SceneTransition {
public:
    explicit FadeTransition(DirectX::XMFLOAT4 color = { 0.f, 0.f, 0.f, 1.f }, float duration = 0.5f);
    ~FadeTransition() override;

    // コピーする時は色と時間だけを写す(ブラシは写さない。同じブラシを2回解放しないように)
    FadeTransition(const FadeTransition& other);
    FadeTransition& operator=(const FadeTransition&) = delete;

protected:
    float GetCoverDuration() const override { return duration; }
    float GetOpenDuration() const override { return duration; }
    void DrawCover(ID2D1RenderTarget* target, float progress) override;
    void DrawOpen(ID2D1RenderTarget* target, float progress) override;

private:
    DirectX::XMFLOAT3 color;   // 塗る色
    float duration;            // 覆う時間・開く時間(秒)

    ID2D1RenderTarget* brushTarget = nullptr;  // ブラシを作った描画先(違う描画先では使えないため覚えておく)
    ID2D1SolidColorBrush* brush = nullptr;     // 塗りつぶし用のブラシ(最初に描く時に作り、デストラクタで解放)

    void Fill(ID2D1RenderTarget* target, float alpha);  // 画面全体を alpha の濃さで塗る
    void ReleaseBrush();
};
