#include "ParticleDemoController.h"
#include "../Common/DemoMouseGround.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Render/Effect.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/UI/Text.h"
#include "../../../../Engine/UI/Anchor.h"
#include <cstdio>
#include <vector>

namespace {
    // 爆発の種類1つ分(名前・一度に出す粒の数・粒の設定)
    struct Preset {
        const wchar_t* name;
        int count;
        ParticleSettings settings;
    };

    constexpr float slowTimeScale = 0.2f;   // SPACEを押している間の時間の倍率
    constexpr float groundY = -50.f;        // 3D版: 爆発させる地面の高さ(DemoGroundの上面)
    constexpr float particleScale3D = 3.f;  // 3D版: 粒の速さ・大きさの倍率

    // 爆発の種類の一覧(1〜4キーで選ぶ。種類を増やす時は、ここに足して、キーの数も増やす)
    const std::vector<Preset>& GetPresets() {
        static const std::vector<Preset> presets = [] {
            std::vector<Preset> list;

            // 1 爆発: 全方向へ飛び散る光の粒(加算合成=重なるほど明るい)。だんだん小さく・暗くなる(色の変化)
            ParticleSettings explosion;
            explosion.lifetimeMin = 0.3f;
            explosion.lifetimeMax = 0.7f;
            explosion.speedMin = 100.f;
            explosion.speedMax = 300.f;
            explosion.spreadAngle = 360.f;
            explosion.startSize = 50.f;
            explosion.endSize = 0.f;
            explosion.startColor = { 1.0f, 0.7f, 0.2f, 1.0f };
            explosion.endColor = { 1.0f, 0.2f, 0.0f, 0.0f };
            explosion.additive = true;
            list.push_back({ L"爆発(加算合成)", 30, explosion });

            // 2 火花: 上へ飛んで、重力(gravity)で落ちてくる細かい粒
            ParticleSettings sparks;
            sparks.lifetimeMin = 0.6f;
            sparks.lifetimeMax = 1.2f;
            sparks.speedMin = 250.f;
            sparks.speedMax = 550.f;
            sparks.direction = { 0.f, 1.f, 0.f };
            sparks.spreadAngle = 120.f;
            sparks.gravity = { 0.f, -900.f, 0.f };
            sparks.startSize = 14.f;
            sparks.endSize = 2.f;
            sparks.startColor = { 1.0f, 1.0f, 0.6f, 1.0f };
            sparks.endColor = { 1.0f, 0.4f, 0.0f, 0.0f };
            sparks.additive = true;
            list.push_back({ L"火花(重力)", 40, sparks });

            // 3 煙: 加算合成にしない(ふわっと重なる普通の合成)。ゆっくり上がって、大きくなりながら消える
            ParticleSettings smoke;
            smoke.lifetimeMin = 1.2f;
            smoke.lifetimeMax = 2.0f;
            smoke.speedMin = 30.f;
            smoke.speedMax = 70.f;
            smoke.direction = { 0.f, 1.f, 0.f };
            smoke.spreadAngle = 40.f;
            smoke.gravity = { 0.f, 40.f, 0.f };
            smoke.startSize = 30.f;
            smoke.endSize = 110.f;
            smoke.startColor = { 0.7f, 0.7f, 0.7f, 0.8f };
            smoke.endColor = { 0.2f, 0.2f, 0.2f, 0.0f };
            smoke.additive = false;
            list.push_back({ L"煙(普通の合成)", 12, smoke });

            // 4 花火: 全方向へ広がって、水色からピンクへ色が変わり、少し落ちる
            ParticleSettings firework;
            firework.lifetimeMin = 0.8f;
            firework.lifetimeMax = 1.4f;
            firework.speedMin = 150.f;
            firework.speedMax = 350.f;
            firework.spreadAngle = 360.f;
            firework.gravity = { 0.f, -150.f, 0.f };
            firework.startSize = 24.f;
            firework.endSize = 0.f;
            firework.startColor = { 0.3f, 0.8f, 1.0f, 1.0f };
            firework.endColor = { 1.0f, 0.2f, 0.8f, 0.0f };
            firework.additive = true;
            list.push_back({ L"花火(色の変化)", 80, firework });

            return list;
        }();
        return presets;
    }

    // 選ぶキー(presetsと同じ順)
    constexpr int selectKeys[] = { KEY_1, KEY_2, KEY_3, KEY_4 };

    // 文字の位置(左上からのずらし量。下へずらす時はマイナス)
    constexpr DirectX::XMFLOAT2 listOffset = { 40.f, -170.f };   // 種類の一覧の1行目
    constexpr float listGap = 32.f;                              // 一覧の行の間隔
}

float GetParticleScale(DemoMode mode) {
    return (mode == DemoMode::Mode3D) ? particleScale3D : 1.f;
}

ParticleSettings AdaptParticleSettings(const ParticleSettings& settings, DemoMode mode) {
    ParticleSettings adapted = settings;
    if (mode != DemoMode::Mode3D) return adapted;

    // 3D空間に出す(粒は常にカメラの方を向く板になる)。画面に対して小さく見えるので、速さ・重力・大きさを大きくする
    const float scale = GetParticleScale(mode);
    adapted.use3D = true;
    adapted.speedMin *= scale;
    adapted.speedMax *= scale;
    adapted.gravity = { adapted.gravity.x * scale, adapted.gravity.y * scale, adapted.gravity.z * scale };
    adapted.startSize *= scale;
    adapted.endSize *= scale;
    return adapted;
}

void ParticleDemoController::Init() {
    // フォントの読み込み(同じファイルは2回読まないので、何回呼んでもよい)
    fontName = Text::LoadFont(L"Assets/Fonts/NotoSansJP[wght].ttf");
}

bool ParticleDemoController::GetClickPosition(DirectX::XMFLOAT3& position) const {
    // 2D: マウスの下のワールド座標
    if (mode == DemoMode::Mode2D) {
        DirectX::XMFLOAT2 mouse = Input::GetMouseWorldPosition();
        position = { mouse.x, mouse.y, 0.f };
        return true;
    }

    // 3D: マウスの線が、地面(y=groundY)に当たる場所(地面に当たらなければfalse)。粒が地面に埋まらないよう少し上に出す
    DirectX::XMFLOAT3 ground;
    if (!GetMouseGroundPoint(groundY, ground)) return false;
    position = { ground.x, groundY + 10.f, ground.z };
    return true;
}

void ParticleDemoController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    const std::vector<Preset>& presets = GetPresets();

    // 1〜4キーで種類を選ぶ
    for (int i = 0; i < (int)presets.size() && i < (int)(sizeof(selectKeys) / sizeof(selectKeys[0])); i++) {
        if (Input::GetKeyDown(selectKeys[i])) selected = i;
    }

    // SPACEを押している間はスローモーション(離すと戻る。画面を出る時は、DemoGuideControllerが倍率を戻す)
    Time::SetTimeScale(Input::GetKeyPress(KEY_SPACE) ? slowTimeScale : 1.f);

    // クリックした場所で爆発させる(作ったらすぐBurstする。粒が消えたらEffectが自動で消える)
    DirectX::XMFLOAT3 clickPosition;
    if (Input::GetKeyDown(MOUSE_LEFT) && GetClickPosition(clickPosition)) {
        const Preset& preset = presets[selected];
        GameObject* effect = CreateEffect(*GetOwner()->GetScene(), AdaptParticleSettings(preset.settings, mode), clickPosition);
        effect->GetComponent<ParticleRenderer>()->Burst(preset.count);
    }
}

void ParticleDemoController::Draw() {
    // 文字の見た目: 黒い縁取り、左寄せ(xが文字の左端)
    TextStyle style;
    style.fontName = fontName;
    style.outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    style.align = TextAlign::Left;
    style.size = 24.f;

    // 爆発の種類の一覧(選んでいる種類は黄色で、先頭に > を付ける)
    const std::vector<Preset>& presets = GetPresets();
    DirectX::XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, listOffset);
    wchar_t buf[96];
    for (int i = 0; i < (int)presets.size(); i++) {
        swprintf_s(buf, L"%ls[%d] %ls", (i == selected) ? L"> " : L"   ", i + 1, presets[i].name);
        style.color = (i == selected) ? DirectX::XMFLOAT4(1.0f, 0.9f, 0.4f, 1.0f) : DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        Text::Draw(buf, pos.x, pos.y - listGap * (float)i, style);
    }

    // 焚き火(出しっぱなし)の粒の数
    if (campfire) {
        swprintf_s(buf, L"焚き火の粒の数: %d(最大 %d)", campfire->GetParticleCount(), ParticleRenderer::maxParticles);
        style.size = 20.f;
        style.color = { 0.7f, 0.9f, 1.0f, 1.0f };
        Text::Draw(buf, pos.x, pos.y - listGap * (float)presets.size() - 10.f, style);
    }

    // スロー中の表示(画面の中央の上)
    if (!Time::IsPaused() && Time::GetTimeScale() < 1.f) {
        style.align = TextAlign::Center;
        style.size = 32.f;
        style.color = { 1.0f, 0.5f, 0.5f, 1.0f };
        DirectX::XMFLOAT2 center = GetAnchoredPosition(Anchor::Top, { 0.f, -190.f });
        Text::Draw(L"SLOW", center.x, center.y, style);
    }
}
