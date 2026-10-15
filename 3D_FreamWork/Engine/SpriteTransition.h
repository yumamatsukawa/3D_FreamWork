#pragma once
#include "SceneTransition.h"
#include <string>

// スプライトシート(1枚の画像に並べたコマ)を画面全体に引き伸ばして再生する、シーン切り替え。
// 普段は、覆う時はコマ0 → 最後のコマへ順に、開く時は最後のコマ → コマ0へ逆に再生する。
// SetSwitchFrame で「切り替えるコマ」を決めた時は、コマ0 → 最後のコマを順に1回だけ再生し、途中のそのコマでシーンを切り替える。
//
// 使い方:
//   // 8列×2行=16コマの画像を、1コマ0.04秒で再生する(覆う0.64秒 → 切り替え → 開く0.64秒)
//   SceneManager::Get().ChangeScene<TitleScene>(SpriteTransition(L"Assets/transition_circle.png", 8, 2, 0.04f));
//   // 開く時だけ別の画像にする(こちらも逆再生する)
//   SceneManager::Get().ChangeScene<TitleScene>(
//       SpriteTransition(L"Assets/close.png", 8, 2, 0.04f).SetOpenSheet(L"Assets/open.png", 4, 2, 0.05f));
//   // コマ0〜7で覆い、コマ7でシーンを切り替え、コマ8〜15で開く(順再生だけ。覆う0.32秒 → 切り替え → 開く0.32秒)
//   SceneManager::Get().ChangeScene<TitleScene>(SpriteTransition(L"Assets/transition_wipe.png", 8, 2, 0.04f).SetSwitchFrame(7));
//
// 画像の作り方:
// - コマは左上から右へ、行の終わりまで行ったら次の行へ、の順に並べる(SpriteRendererのコマ番号と同じ)
// - 最後のコマで、画面全体が覆われるように描く(最後のコマが出ている間にシーンを切り替えるため)。コマ0は全部透明にすると自然につながる
// - 開く時の画像も同じ決まりで作る(逆再生するので、最後のコマ=全部覆った絵から始まる)
// - SetSwitchFrame を使う時は、そのコマで画面全体が覆われるように描き、最後のコマは全部透明にする(コマ0も全部透明にすると自然につながる)
// - 各コマは画面全体に引き伸ばして描くので、1コマの縦横比は画面と同じ(16:9)にするとゆがまない
// - 画像は Direct2D で読み込み、同じファイルは使い回す(Engineの終了時に解放する)。読み込めなかった時は何も描かずに、時間どおりに切り替える
class SpriteTransition : public SceneTransition {
public:
    // path: スプライトシートの画像。columns×rows: コマの並び(列, 行)。frameDuration: 1コマを出す時間(秒)
    SpriteTransition(const std::wstring& path, int columns, int rows, float frameDuration);

    // 開く時だけ別の画像を使う(使わなければ、覆う時と同じ画像を逆再生する)。続けて書けるように自分を返す
    SpriteTransition& SetOpenSheet(const std::wstring& path, int columns, int rows, float frameDuration);

    // シーンを切り替えるコマを決める(覆う=コマ0〜frame、開く=コマframe+1〜最後のコマ。どちらも順再生)。続けて書けるように自分を返す
    // - frame は 0〜(コマ数-2) に収める(範囲外なら収めて、出力ウィンドウに知らせる。開く段階が1コマ以上になるように。1コマだけの画像は0)
    // - SetOpenSheet と一緒に使った時は、こちらを優先する(開く時の画像は使わない。出力ウィンドウに知らせる)
    SpriteTransition& SetSwitchFrame(int frame);

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    static void UnloadAll();   // 読み込んだ画像をすべて解放する(Text::Uninit より前に呼ぶ)

protected:
    float GetCoverDuration() const override;
    float GetOpenDuration() const override;
    void OnBegin() override;
    void DrawCover(ID2D1RenderTarget* target, float progress) override;
    void DrawOpen(ID2D1RenderTarget* target, float progress) override;

    // 進み具合(0〜1)の時に出すコマの番号(順再生。逆再生は frameCount - 1 から引く)
    static int GetFrameIndex(float progress, int frameCount);

private:
    // スプライトシート1枚分の情報
    struct Sheet {
        std::wstring path;          // 画像のファイル
        int columns = 1;            // 列の数
        int rows = 1;               // 行の数
        float frameDuration = 0.f;  // 1コマの時間(秒)

        int GetFrameCount() const { return columns * rows; }
        float GetDuration() const { return frameDuration * (float)GetFrameCount(); }
    };

    Sheet coverSheet;            // 覆う時の画像
    Sheet openSheet;             // 開く時の画像(SetOpenSheet で決める)
    bool hasOpenSheet = false;   // 開く時だけ別の画像を使うか
    int switchFrame = 0;         // シーンを切り替えるコマ(SetSwitchFrame で決める)
    bool hasSwitchFrame = false; // 切り替えるコマを決めたか(決めた時は順再生だけで1回流す)

    static Sheet MakeSheet(const std::wstring& path, int columns, int rows, float frameDuration);
    const Sheet& GetOpenSheet() const { return hasOpenSheet ? openSheet : coverSheet; }
    int GetCoverFrameCount() const { return switchFrame + 1; }                                 // SetSwitchFrame の時の、覆う段階のコマ数
    int GetOpenFrameCount() const { return coverSheet.GetFrameCount() - 1 - switchFrame; }    // SetSwitchFrame の時の、開く段階のコマ数
    static void DrawFrame(ID2D1RenderTarget* target, const Sheet& sheet, int index);  // 1コマを画面全体に引き伸ばして描く
};
