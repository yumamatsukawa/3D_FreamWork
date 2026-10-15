#pragma once
#include <DirectXMath.h>
#include <functional>

// 画面全体に色の演出(フラッシュ・ビネット・フェード)を重ねる。
// 文字(Text::Draw)も含めて、画面の一番上に描く(暗転すると文字も隠れる)。Direct2Dで描くので画像ファイルは使わない
//
// 使い方:
//   ScreenEffect::Flash({ 1.f, 1.f, 1.f, 0.8f }, 0.3f);           // 白く光って0.3秒で消える
//   ScreenEffect::Vignette({ 1.f, 0.f, 0.f, 0.6f }, 0.5f, 2);     // 画面の端が赤く2回点滅する(被弾)
//   ScreenEffect::Fade({ 0.f, 0.f, 0.f, 1.f }, 1.f, 0.5f, [] {    // 0.5秒で暗転して、暗転し終わったらシーンを切り替える
//       SceneManager::Get().ChangeScene<GameScene>();
//   });
//   ScreenEffect::Fade({ 0.f, 0.f, 0.f, 1.f }, 0.f, 0.5f);        // 0.5秒で明転する(次のシーンのComponentのInitで)
//
// - 3つのエフェクトは別々に動くので、同時に出せる。重なる順番は下から Vignette → Flash → Fade(Fadeが一番上)
// - 時間はポーズ中も進む(Time::GetUnscaledDeltaTime())
// - 同じエフェクトを途中でまた呼んだ時は、新しい方に置き換える(最初から数え直す)
// - シーンを切り替えると、Flash と Vignette は消える。Fade の濃さと進み具合は残る(暗転したまま次のシーンへ行き、そこで明転できる)。
//   ただし Fade の onComplete は、まだ呼ばれていなければ捨てる(消えたシーンのオブジェクトを触らないように)
namespace ScreenEffect {
    // 画面全体が一瞬光る。color.w の濃さから、duration 秒で0になる(だんだん弱まる)。duration が0以下なら何も出さない(出ている途中なら消す)
    void Flash(DirectX::XMFLOAT4 color, float duration);

    // 画面の端だけが色付きで点滅する(中央は透明)。duration 秒の間に blinkCount 回点滅して消える。
    // color.w が一番濃い時の濃さ。duration が0以下なら何も出さない(出ている途中なら消す)。blinkCount が1未満なら1回
    void Vignette(DirectX::XMFLOAT4 color, float duration, int blinkCount);

    // 暗転・明転。今の濃さから toAlpha(0〜1)へ、duration 秒で変える(色は color の x,y,z。color.w は使わない)。
    // toAlpha=1 で塗りつぶしたまま止まり、その後 toAlpha=0 で消える。
    // onComplete(省略可): 変わり終わった時に1回だけ呼ぶ。ScreenEffect::Update(シーンのUpdateの前)の中で呼ぶので、
    //   中で SceneManager::Get().ChangeScene() してよい(同じフレームのうちにシーンが切り替わる)。
    //   変わり終わる前に次の Fade を呼んだ時や、シーンが切り替わった時は、前の onComplete は呼ばれずに捨てられる
    // duration が0以下なら、すぐに toAlpha にして、onComplete もその場で呼ぶ
    void Fade(DirectX::XMFLOAT4 color, float toAlpha, float duration, std::function<void()> onComplete = nullptr);

    // Fade の濃さが変わっている途中か(暗転し終わって止まっている間は false)
    bool IsFading();

    // 今出ている Flash と Vignette をすぐに消す。Fade は消さない(今の濃さも onComplete もそのまま)
    void Stop();

    // ─── Engine内部用(GameEngine・SceneManagerだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    void Update(float unscaledDt);  // 毎フレーム1回、エフェクトの時間を進める(シーンのUpdateの前)
    void Draw();                    // シーンと文字を描いた後、Text::EndDraw の前に呼ぶ(一番上に描く)
    void OnSceneChanged();          // シーンの切り替え時(古いシーンのUninitの後・新しいシーンのInitの前): Stop して、Fade の onComplete を捨てる
    void Uninit();                  // Direct2Dのブラシを解放する(Text::Uninit より前に呼ぶ)
}
