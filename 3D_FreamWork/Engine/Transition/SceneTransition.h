#pragma once
#include <d2d1.h>

// シーン切り替えの演出の親クラス。どの種類も「画面を覆う → シーンを切り替える → 開く」の順に進む。
// 覆い方・開き方は種類ごとに override する(FadeTransition・SpriteTransition)。
// ゲーム側は SceneManager::Get().ChangeScene<GameScene>(FadeTransition()); のように作って渡すだけで、このクラスの関数は呼ばない
class SceneTransition {
public:
    virtual ~SceneTransition() = default;

    // ─── Engine内部用(SceneManagerだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    void Begin();                   // 覆い始める(ChangeSceneが呼ばれた時)
    void Update(float unscaledDt);  // 毎フレーム1回、時間を進める
    void BeginOpen();               // シーンを切り替えた後: 開き始める
    void Draw();                    // 今の様子を、文字も含めて画面の一番上に描く(Text::BeginDraw〜EndDrawの間)
    bool IsCovered() const;         // 覆い終わって、シーンの切り替え待ちか
    bool IsFinished() const;        // 開き終わったか

protected:
    virtual float GetCoverDuration() const = 0;  // 覆うのにかかる時間(秒)
    virtual float GetOpenDuration() const = 0;   // 開くのにかかる時間(秒)
    virtual void OnBegin() {}                    // 覆い始める時に1回呼ばれる(画像の読み込みなど)

    // 覆っている途中を描く。progress: 0=まだ何も覆っていない → 1=画面全体を覆った
    virtual void DrawCover(ID2D1RenderTarget* target, float progress) = 0;
    // 開いている途中を描く。progress: 0=画面全体を覆っている → 1=全部見えている
    virtual void DrawOpen(ID2D1RenderTarget* target, float progress) = 0;

private:
    enum class Phase { None, Covering, Covered, Opening, Finished };

    Phase phase = Phase::None;   // 今どの段階か
    float elapsed = 0.f;         // 今の段階に入ってからの時間(秒)
    bool skipNextUpdate = false; // 次のUpdateで時間を進めないか(シーンの読み込みで長くなったフレームの分を飛ばすため)

    float GetProgress(float duration) const;  // 今の段階の進み具合(0〜1)
};
