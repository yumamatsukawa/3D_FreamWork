#include "DemoRegistry.h"
#include "DemoMenuScene.h"
#include "DemoCategoryScene.h"
#include "../Core/ObjectDemo2DScene.h"
#include "../Core/ObjectDemo3DScene.h"
#include "../Core/TransformDemo2DScene.h"
#include "../Core/TransformDemo3DScene.h"
#include "../Core/TimeDemoScene.h"
#include "../Core/InputDemoScene.h"
#include "../Core/MathDemo2DScene.h"
#include "../Core/MathDemo3DScene.h"
#include "../Core/PoolDemoScene.h"
#include "../Core/EventBusDemoScene.h"
#include "../Core/ResourcesDemoScene.h"
#include "../Render/TweenDemo2DScene.h"
#include "../Render/TweenDemo3DScene.h"
#include "../Render/ParticleDemo2DScene.h"
#include "../Render/ParticleDemo3DScene.h"
#include "../Render/SpriteDemo2DScene.h"
#include "../Render/SpriteDemo3DScene.h"
#include "../Render/SpriteAnimDemo2DScene.h"
#include "../Render/SpriteAnimDemo3DScene.h"
#include "../Render/CameraDemo2DScene.h"
#include "../Render/CameraDemo3DScene.h"
#include "../Render/ScreenEffectDemoScene.h"
#include "../Render/TransitionDemoScene.h"
#include "../Render/CoordDemo2DScene.h"
#include "../Render/CoordDemo3DScene.h"
#include "../UI/UiDemoScene.h"
#include "../UI/RectDemoScene.h"
#include "../UI/TextDemoScene.h"
#include "../UI/WorldUiDemo2DScene.h"
#include "../UI/WorldUiDemo3DScene.h"
#include "../Render3D/ShapesDemoScene.h"
#include "../Render3D/LightDemoScene.h"
#include "../Render3D/OutlineDemoScene.h"
#include "../Render3D/ModelDemoScene.h"
#include "../Render3D/SkinAnimDemoScene.h"
#include "../Render3D/LodDemoScene.h"
#include "../Render3D/VisibilityDemo2DScene.h"
#include "../Render3D/VisibilityDemo3DScene.h"
#include "../Render3D/ShadowDemoScene.h"
#include "../Render3D/TerrainDemoScene.h"
#include "../Render3D/WaterDemoScene.h"
#include "../Physics/ColliderDemo2DScene.h"
#include "../Physics/ColliderDemo3DScene.h"
#include "../Physics/ForceDemo2DScene.h"
#include "../Physics/ForceDemo3DScene.h"
#include "../Physics/RigidbodySettingsDemo2DScene.h"
#include "../Physics/RigidbodySettingsDemo3DScene.h"
#include "../Physics/TriggerDemo2DScene.h"
#include "../Physics/TriggerDemo3DScene.h"
#include "../Physics/QueryDemo2DScene.h"
#include "../Physics/QueryDemo3DScene.h"
#include "../Physics/TerrainColliderDemoScene.h"
#include "../Behavior/MoveDemo2DScene.h"
#include "../Behavior/MoveDemo3DScene.h"
#include "../Behavior/HomingDemo2DScene.h"
#include "../Behavior/HomingDemo3DScene.h"
#include "../Behavior/VisionDemo2DScene.h"
#include "../Behavior/VisionDemo3DScene.h"
#include "../Behavior/PathDemo2DScene.h"
#include "../Behavior/PathDemo3DScene.h"
#include "../Behavior/MovingWallDemo2DScene.h"
#include "../Behavior/MovingWallDemo3DScene.h"
#include "../Behavior/TerrainPathDemoScene.h"
#include "../Audio/SoundDemoScene.h"
#include "../Audio/Sound3DDemo2DScene.h"
#include "../Audio/Sound3DDemo3DScene.h"
#include "../Data/CsvDemoScene.h"
#include "../Data/SaveDemoScene.h"
#include "../../../../Engine/Core/SceneManager.h"
#include "../../../../Engine/Core/Time.h"
#include <windows.h>

namespace {
    // 黒で暗転 → シーンTへ切り替え → 明転(ChangeSceneはテンプレートなので、シーンの型ごとにこの関数が作られる)
    template<typename T>
    void OpenScene() {
        SceneManager::Get().ChangeScene<T>(FadeTransition());
    }

    // 今の状態(Engineの Image などと同じ「名前空間+static」。Get()のシングルトンは作らない)
    DemoMode demoMode = DemoMode::Mode3D;                       // 今の側(起動した時は3D)
    DemoCategory currentCategory = DemoCategory::Core;          // 今のカテゴリ
    const DemoInfo* currentDemo = nullptr;                      // 最後に開いたデモ(デモ画面の「切り替え」「印」が読む。一度も開いていない時は nullptr)
}

const std::vector<DemoInfo>& GetDemoList() {
    static const std::vector<DemoInfo> demos = {
        // ★新しいデモは、ここに1行足す: { カテゴリ, ボタンの名前, 一言説明, 2D版を開く関数, 3D版を開く関数 }
        //   2D版/3D版が無い側は nullptr、2D・3Dの区別が無い機能は両方に同じ関数を入れる
        { DemoCategory::Core,     L"Object",            L"物に部品(Component)を付けたり止めたりする",    &OpenScene<ObjectDemo2DScene>,            &OpenScene<ObjectDemo3DScene> },
        { DemoCategory::Core,     L"Transform",         L"位置・回転・大きさと、親子でついてくる動き",   &OpenScene<TransformDemo2DScene>,         &OpenScene<TransformDemo3DScene> },
        { DemoCategory::Core,     L"Time",              L"時間のスロー・ポーズと、Timerの使い方",        &OpenScene<TimeDemoScene>,                &OpenScene<TimeDemoScene> },
        { DemoCategory::Core,     L"Input",             L"キー・マウス・ゲームパッドの今の状態",         &OpenScene<InputDemoScene>,               &OpenScene<InputDemoScene> },
        { DemoCategory::Core,     L"Math",              L"Lerp・Clamp・角度・内積・外積・乱数を見る",    &OpenScene<MathDemo2DScene>,              &OpenScene<MathDemo3DScene> },
        { DemoCategory::Core,     L"Pool",              L"弾を作り直さずに使い回す(ObjectPool)",         &OpenScene<PoolDemoScene>,                &OpenScene<PoolDemoScene> },
        { DemoCategory::Core,     L"EventBus",          L"互いを知らない物同士が名前で通知する",         &OpenScene<EventBusDemoScene>,            &OpenScene<EventBusDemoScene> },
        { DemoCategory::Core,     L"Resources",         L"同じ画像・音は1回だけ読み込んで共有する",      &OpenScene<ResourcesDemoScene>,           &OpenScene<ResourcesDemoScene> },
        { DemoCategory::Render,   L"Sprite",            L"画像のコマ・色・大きさ・回転。3Dはビルボード", &OpenScene<SpriteDemo2DScene>,            &OpenScene<SpriteDemo3DScene> },
        { DemoCategory::Render,   L"SpriteAnim",        L"コマ送りのアニメ(再生・停止・速さ)",           &OpenScene<SpriteAnimDemo2DScene>,        &OpenScene<SpriteAnimDemo3DScene> },
        { DemoCategory::Render,   L"Camera",            L"カメラの追従と揺れ。3Dは三人称・一人称",       &OpenScene<CameraDemo2DScene>,            &OpenScene<CameraDemo3DScene> },
        { DemoCategory::Render,   L"ScreenEffect",      L"画面全体の光・赤い縁・暗転",                   &OpenScene<ScreenEffectDemoScene>,        &OpenScene<ScreenEffectDemoScene> },
        { DemoCategory::Render,   L"Transition",        L"シーン切り替えの演出を見比べる",               &OpenScene<TransitionDemoScene>,          &OpenScene<TransitionDemoScene> },
        { DemoCategory::Render,   L"Particle",          L"爆発・火花・煙・花火。粒の設定を見る",         &OpenScene<ParticleDemo2DScene>,          &OpenScene<ParticleDemo3DScene> },
        { DemoCategory::Render,   L"Tween",             L"Easeの速さの違いを8種類並べて比べる",          &OpenScene<TweenDemo2DScene>,             &OpenScene<TweenDemo3DScene> },
        { DemoCategory::Render,   L"Coord",             L"画面とワールドの座標の変換・Raycast",          &OpenScene<CoordDemo2DScene>,             &OpenScene<CoordDemo3DScene> },
        { DemoCategory::UI,       L"UI",                L"ボタン・スライダー・ゲージ・TextRenderer",     &OpenScene<UiDemoScene>,                  &OpenScene<UiDemoScene> },
        { DemoCategory::UI,       L"Rect",              L"四角を描く。画面の9か所(Anchor)に置く",        &OpenScene<RectDemoScene>,                &OpenScene<RectDemoScene> },
        { DemoCategory::UI,       L"Text",              L"文字の大きさ・色・縁取り・寄せ方・フォント",   &OpenScene<TextDemoScene>,                &OpenScene<TextDemoScene> },
        { DemoCategory::UI,       L"WorldUI",           L"物の頭上に出るHPゲージと名前",                 &OpenScene<WorldUiDemo2DScene>,           &OpenScene<WorldUiDemo3DScene> },
        { DemoCategory::Render3D, L"Shapes",            L"図形の種類と、材質(Material)の違い",           nullptr,                                  &OpenScene<ShapesDemoScene> },
        { DemoCategory::Render3D, L"Light",             L"太陽・点光源・スポットライトを見比べる",       nullptr,                                  &OpenScene<LightDemoScene> },
        { DemoCategory::Render3D, L"Shadow",            L"太陽が回ると影が動く。影の設定を切り替える",   nullptr,                                  &OpenScene<ShadowDemoScene> },
        { DemoCategory::Render3D, L"Outline",           L"枠線・壁越しの枠線・縁の光",                   nullptr,                                  &OpenScene<OutlineDemoScene> },
        { DemoCategory::Render3D, L"Terrain",           L"画像から作った丘。高さと向きを調べる",         nullptr,                                  &OpenScene<TerrainDemoScene> },
        { DemoCategory::Render3D, L"Water",             L"池の水面。色・波の高さ・速さを変える",         nullptr,                                  &OpenScene<WaterDemoScene> },
        { DemoCategory::Render3D, L"Model",             L"FBXモデルを置いて、パーツを動かす",            nullptr,                                  &OpenScene<ModelDemoScene> },
        { DemoCategory::Render3D, L"SkinAnim",          L"骨で曲がるモデルのアニメ(Play・CrossFade)",    nullptr,                                  &OpenScene<SkinAnimDemoScene> },
        { DemoCategory::Render3D, L"LOD",               L"遠い物を省く距離と、描画の数の変化",           nullptr,                                  &OpenScene<LodDemoScene> },
        { DemoCategory::Render3D, L"Visibility",        L"画面に映っているかの判定と、描かない設定",     &OpenScene<VisibilityDemo2DScene>,        &OpenScene<VisibilityDemo3DScene> },
        { DemoCategory::Physics,  L"Collider",          L"円・四角・カプセルの当たり判定と枠線",         &OpenScene<ColliderDemo2DScene>,          &OpenScene<ColliderDemo3DScene> },
        { DemoCategory::Physics,  L"Force",             L"力と速度の違い(AddForceの4種類)",              &OpenScene<ForceDemo2DScene>,             &OpenScene<ForceDemo3DScene> },
        { DemoCategory::Physics,  L"RigidbodySettings", L"質量・抵抗・重力・固定で動きが変わる",         &OpenScene<RigidbodySettingsDemo2DScene>, &OpenScene<RigidbodySettingsDemo3DScene> },
        { DemoCategory::Physics,  L"Trigger",           L"すり抜けるトリガーと、ぶつかる判定の違い",     &OpenScene<TriggerDemo2DScene>,           &OpenScene<TriggerDemo3DScene> },
        { DemoCategory::Physics,  L"Query",             L"Raycast・Overlapの問い合わせを目で見る",       &OpenScene<QueryDemo2DScene>,             &OpenScene<QueryDemo3DScene> },
        { DemoCategory::Physics,  L"TerrainCollider",   L"丘を歩いて登る(地形の当たり判定)",             nullptr,                                  &OpenScene<TerrainColliderDemoScene> },
        { DemoCategory::Behavior, L"Move",              L"目標へまっすぐ向かう。速さと止まる距離",       &OpenScene<MoveDemo2DScene>,              &OpenScene<MoveDemo3DScene> },
        { DemoCategory::Behavior, L"Homing",            L"追いかけ方3種(一定・加速・曲がる)を比べる",    &OpenScene<HomingDemo2DScene>,            &OpenScene<HomingDemo3DScene> },
        { DemoCategory::Behavior, L"Vision",            L"視野(扇形)と壁で、見える・見えないを調べる",   &OpenScene<VisionDemo2DScene>,            &OpenScene<VisionDemo3DScene> },
        { DemoCategory::Behavior, L"Path",              L"壁を避けて歩く経路探索(NavGrid)",              &OpenScene<PathDemo2DScene>,              &OpenScene<PathDemo3DScene> },
        { DemoCategory::Behavior, L"MovingWall",        L"動く壁・柵で経路が変わる(焼き直し)",           &OpenScene<MovingWallDemo2DScene>,        &OpenScene<MovingWallDemo3DScene> },
        { DemoCategory::Behavior, L"TerrainPath",       L"丘の坂を取り込んだ経路探索",                   nullptr,                                  &OpenScene<TerrainPathDemoScene> },
        { DemoCategory::Audio,    L"Sound",             L"BGM・SEと、音量グループ・フェード",            &OpenScene<SoundDemoScene>,               &OpenScene<SoundDemoScene> },
        { DemoCategory::Audio,    L"Sound3D",           L"位置で聞こえ方が変わる音(遠い・左右)",         &OpenScene<Sound3DDemo2DScene>,           &OpenScene<Sound3DDemo3DScene> },
        { DemoCategory::Data,     L"Csv",               L"表(CSV)を読む。保存し直すと実行中に反映",      &OpenScene<CsvDemoScene>,                 &OpenScene<CsvDemoScene> },
        { DemoCategory::Data,     L"Save",              L"整数・小数・文字を保存して、開き直しても残す", &OpenScene<SaveDemoScene>,                &OpenScene<SaveDemoScene> },
    };
    return demos;
}

std::vector<const DemoInfo*> GetDemoListOf(DemoCategory category) {
    std::vector<const DemoInfo*> list;
    for (const DemoInfo& demo : GetDemoList()) {
        if (demo.category == category) list.push_back(&demo);
    }
    return list;
}

const std::vector<DemoCategoryInfo>& GetCategoryList() {
    static const std::vector<DemoCategoryInfo> categories = {
        { DemoCategory::Core,     L"Core",     L"基本の仕組み(Object・Time・Inputなど)" },
        { DemoCategory::Render,   L"Render",   L"2D描画と演出(スプライト・カメラ・画面効果など)" },
        { DemoCategory::UI,       L"UI",       L"UIと文字(ボタン・ゲージなど)" },
        { DemoCategory::Render3D, L"Render3D", L"3D描画(図形・ライト・影・モデル・LODなど)" },
        { DemoCategory::Physics,  L"Physics",  L"物理(当たり判定・力・地形など)" },
        { DemoCategory::Behavior, L"Behavior", L"動きとAI(追跡・視野・経路探索)" },
        { DemoCategory::Audio,    L"Audio",    L"音(BGM・SE・3Dの音)" },
        { DemoCategory::Data,     L"Data",     L"データ(CSV・セーブ)" },
    };
    return categories;
}

const DemoCategoryInfo& GetCategoryInfo(DemoCategory category) {
    for (const DemoCategoryInfo& info : GetCategoryList()) {
        if (info.category == category) return info;
    }
    return GetCategoryList().front();
}

DemoMode GetDemoMode() { return demoMode; }
void SetDemoMode(DemoMode mode) { demoMode = mode; }
void ToggleDemoMode() { demoMode = (demoMode == DemoMode::Mode2D) ? DemoMode::Mode3D : DemoMode::Mode2D; }
DemoCategory GetCurrentCategory() { return currentCategory; }
const DemoInfo* GetCurrentDemo() { return currentDemo; }

bool CanOpenDemo(const DemoInfo& demo, DemoMode mode) {
    return (mode == DemoMode::Mode2D ? demo.open2D : demo.open3D) != nullptr;
}

bool CanSwitchDemoMode() {
    // 逆の側の版があって、今の側と別の版(同じ関数なら「2D・3Dの区別が無い機能」なので切り替えない)
    if (!currentDemo) return false;
    return currentDemo->open2D && currentDemo->open3D && currentDemo->open2D != currentDemo->open3D;
}

const wchar_t* GetDemoKindTag(const DemoInfo& demo) {
    if (demo.open2D && demo.open3D) return (demo.open2D == demo.open3D) ? L"[ALL]" : L"[2D/3D]";
    return demo.open2D ? L"[2D]" : L"[3D]";
}

std::wstring GetCurrentModeTag() {
    if (!currentDemo) return L"";
    if (currentDemo->open2D == currentDemo->open3D) return L"[ALL]";
    return (demoMode == DemoMode::Mode2D) ? L"[2D]" : L"[3D]";
}

void OpenDemoMenu() {
    OpenScene<DemoMenuScene>();
}

void OpenCategory(DemoCategory category) {
    currentCategory = category;
    OpenScene<DemoCategoryScene>();
}

void OpenCurrentCategory() {
    OpenScene<DemoCategoryScene>();
}

void OpenDemo(const DemoInfo& demo) {
    void (*open)() = (demoMode == DemoMode::Mode2D) ? demo.open2D : demo.open3D;
    if (!open) return;
    currentDemo = &demo;
    currentCategory = demo.category;
    open();
}

void SwitchDemoMode() {
    // 切り替え中(暗転中)は受け付けない(側だけが入れ替わって、シーンが切り替わらないのを防ぐ)
    if (Time::IsPaused() || !CanSwitchDemoMode()) return;

    // 側を入れ替えて、逆の側の版を開く
    ToggleDemoMode();
    void (*open)() = (demoMode == DemoMode::Mode2D) ? currentDemo->open2D : currentDemo->open3D;
    open();
}

void QuitDemo() {
    PostQuitMessage(0);
}
