#include "DemoGuideController.h"
#include "../../Scenes/Common/DemoRegistry.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Audio/Audio.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render/ScreenEffect.h"
#include "../../../../Engine/Render/CameraShake.h"
#include "../../../../Engine/Render3D/Shadow.h"
#include "../../../../Engine/Render3D/Lod.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../../Engine/Behavior/VisionComponent.h"
#include "../../../../Engine/Behavior/NavGridComponent.h"
#include "../../../../Engine/Behavior/PathFollowComponent.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"

namespace {
    // 位置(画面の隅からのずらし量。画面座標は「中央が(0,0)、Y+=上」なので、下へずらす時はマイナス)
    constexpr DirectX::XMFLOAT2 panelOffset = { 12.f, -12.f };    // 左上から: 文字の後ろの板の左上の角
    constexpr DirectX::XMFLOAT2 panelSize = { 1240.f, 128.f };    // 板の大きさ
    constexpr DirectX::XMFLOAT2 textOffset = { 30.f, -34.f };     // 左上から: 1行目の文字の位置
    constexpr float lineGap = 36.f;                               // 2行目以降の行の間隔
}

void DemoGuideController::ResetSettings() {
    // 2Dカメラを原点に戻す(デモがカメラを動かしても、次の画面に持ち越さない)
    Image::GetCamera().position.x = 0.f;
    Image::GetCamera().position.y = 0.f;

    // 3Dカメラを既定に戻して、2Dカメラへの自動追従も戻す(デモのカメラは、この自動追従を止めて自分で動かすため)
    Image::SetCamera3DAutoSync(true);
    Image::GetCamera3D() = Camera();

    // 時間の倍率を戻す(スローにしたまま画面を切り替えても、次の画面が遅くならない)
    Time::SetTimeScale(1.f);

    // 平行光源(向き・色・環境光)・影・距離で省く設定を既定に戻す
    Image::GetLight() = Light();
    Shadow::SetSettings(Shadow::Settings());
    Lod::SetEnabled(true);
    Lod::SetGlobalScale(1.f);

    // 当たり判定・視野・経路探索の枠線の表示を消す
    RigidbodyComponent::SetDebugDrawEnabled(false);
    VisionComponent::SetDebugDrawEnabled(false);
    NavGridComponent::SetDebugDrawEnabled(false);
    PathFollowComponent::SetDebugDrawEnabled(false);

    // 画面エフェクトとカメラの揺れを止める
    ScreenEffect::Stop();
    CameraShake::Stop();

    // BGM/SEの音量を既定(最大)に戻す
    Audio::SetGroupVolume(AudioGroup::BGM, 1.f);
    Audio::SetGroupVolume(AudioGroup::SE, 1.f);

    // カーソルの固定を解く
    Input::SetCursorLocked(false);
}

void DemoGuideController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、シーンに入る度に呼んでよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");

    // 入る時に設定を戻す(前の画面の設定が残っていても、デモを同じ状態から始められる)
    ResetSettings();
}

void DemoGuideController::Update(float dt) {
    // Escキーで前の画面へ戻る(切り替え中にもう一度押されても、SceneManagerが無視する)
    if (Input::GetKeyDown(KEY_ESCAPE) && onBack) onBack();

    // Tabキーで2D/3D: デモ本体は逆の側の版へ切り替える、メニューは側を選ぶ(右下のボタンと同じ)
    if (Input::GetKeyDown(KEY_TAB)) {
        if (screen == DemoScreen::Demo) SwitchDemoMode();
        else ToggleDemoMode();
    }
}

void DemoGuideController::Draw() {
    // 文字の後ろの半透明の板(文字が読みやすいように。文字より先に描く)
    Image::DrawRect(GetAnchoredCenter(Anchor::TopLeft, panelOffset, panelSize), panelSize, { 0.0f, 0.0f, 0.0f, 0.55f });

    // 文字の見た目: 黒い縁取り、左寄せ(xが文字の左端)
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Left;

    // 1行目: デモの名前(デモ本体は、今の側の印 [2D] [3D] [ALL] を付ける)
    style.size = 28.f;
    style.color = { 1.0f, 0.9f, 0.4f, 1.0f };
    DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, textOffset);
    const std::wstring title = (screen == DemoScreen::Demo) ? info.name + L"  " + GetCurrentModeTag() : info.name;
    Text::Draw(title, pos.x, pos.y, style);

    // 2行目: 何のデモか、3行目: 操作方法、4行目: 使っているAPI
    style.size = 19.f;
    style.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pos.y -= lineGap - 6.f;
    Text::Draw(info.description, pos.x, pos.y, style);

    style.color = { 0.6f, 0.9f, 1.0f, 1.0f };
    pos.y -= lineGap - 8.f;
    Text::Draw(info.controls, pos.x, pos.y, style);

    style.color = { 0.7f, 1.0f, 0.7f, 1.0f };
    pos.y -= lineGap - 8.f;
    Text::Draw(info.api, pos.x, pos.y, style);
}

void DemoGuideController::Uninit() {
    // 出る時にも設定を戻す(別の画面へ移った時に、見た目が変わらないように)
    ResetSettings();
}
