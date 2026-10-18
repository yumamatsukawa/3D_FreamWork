#pragma once
#include <memory>
#include <functional>
#include <type_traits>
#include "Scene.h"
#include "../Transition/SceneTransition.h"
#include "../Transition/FadeTransition.h"    // このヘッダーだけで、演出の種類もすぐ使えるように
#include "../Transition/SpriteTransition.h"

// シーンの切り替えを担当する(アプリ全体で1つ)。
//
// 使い方:
//   SceneManager::Get().ChangeScene<GameScene>();                    // すぐに切り替える(次のSceneManager::Updateの最初で)
//   SceneManager::Get().ChangeScene<GameScene>(FadeTransition());    // 0.5秒で暗転 → 切り替え → 0.5秒で明転
//   SceneManager::Get().ChangeScene<TitleScene>(SpriteTransition(L"Assets/transition_circle.png", 8, 2, 0.04f));
//
// - 演出付きの切り替えは「覆う → シーンを切り替える → 開く」の順に進む。演出は文字・ScreenEffectも含めて画面の一番上に描く
// - 演出の間(覆っている間・開いている間)は、ゲームの時間を止める(Time の倍率0。演出の時間はUnscaledで進む)。
//   開き終わったら、切り替える前の倍率に戻す(演出中にゲーム側が SetTimeScale した時は、その値になる。Time.h 参照)
// - 切り替えを頼んでから(演出が無い時はシーンが切り替わるまで、ある時は開き終わるまで)の間に呼ばれた ChangeScene は無視する
//   (ボタンの二重押しなどで2回切り替わらないように。出力ウィンドウに知らせる)
class SceneManager {
public:
    static SceneManager& Get() {
        static SceneManager instance;
        return instance;
    }

    // 演出なしで切り替える
    template<typename T>
    void ChangeScene() {
        RequestChange([]() -> Scene* { return new T(); }, nullptr);
    }

    // 演出付きで切り替える(transition には FadeTransition / SpriteTransition などを渡す)
    template<typename T, typename TransitionType>
    void ChangeScene(TransitionType transition) {
        static_assert(std::is_base_of<SceneTransition, TransitionType>::value,
            "ChangeScene の引数には FadeTransition / SpriteTransition など、SceneTransition の子クラスを渡す");
        RequestChange([]() -> Scene* { return new T(); }, std::make_unique<TransitionType>(transition));
    }

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    void Update(float dt);
    void Draw();
    void DrawTransition();   // 切り替えの演出を描く(ScreenEffect::Draw の後、Text::EndDraw の前。一番上に出す)
    void Uninit();

private:
    SceneManager() = default;

    void RequestChange(std::function<Scene* ()> factory, std::unique_ptr<SceneTransition> newTransition);
    void SwitchScene();   // 古いシーンを片付けて、新しいシーンを始める

    std::unique_ptr<Scene>           currentScene;
    std::function<Scene* ()>         nextScene = nullptr;  // 次のシーンを作る関数(切り替えを頼まれていない時は nullptr)
    std::unique_ptr<SceneTransition> transition;           // 今の切り替えの演出(演出していない時は nullptr)
};
