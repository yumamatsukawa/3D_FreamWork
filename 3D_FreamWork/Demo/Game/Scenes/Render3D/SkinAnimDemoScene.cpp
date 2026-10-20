#include "SkinAnimDemoScene.h"
#include "../../Objects/Common/DemoStage3D.h"
#include "../../Objects/Common/DemoGuide.h"
#include "../../Objects/Render3D/SkinAnimDemo.h"

void SkinAnimDemoScene::Init()
{
    // オブジェクトを配置するだけ(動き・ルールは各Componentに書く)
    CreateDemoGuide(*this, { L"3Dアニメーションのデモ(SkinAnim)",
        L"FBXのクリップ(動き)を再生します。腕は骨で曲がります。切り替えは、すぐ(Play)かなめらか(CrossFade)です。",
        L"操作: 1・2 箱のクリップ / SPACE CrossFade / 3 腕を再生し直す / 4 枠線 / 上下 速さ / 右ドラッグ 視点 / Esc 戻る",
        L"使っているAPI: ModelAnimationComponent(Play, CrossFade, SetSpeed, IsFinished, GetClipNames)" });
    // 見る点を左へずらして、3つのモデルを画面の右半分に寄せる(左は状態の文字を出す)
    CreateDemoStage3D(*this, { -480.0f, 70.0f, 100.0f }, 0.0f, -15.0f, 1100.0f);
    CreateSkinAnimDemo(*this);
}

void SkinAnimDemoScene::Uninit()
{
    Scene::Uninit();
}
