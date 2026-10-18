#include "ParticleRenderer.h"
#include "../Core/GameObject.h"
#include "../Core/Resources.h"
#include "Image.h"
#include "../Core/MathUtil.h"
#include <algorithm>

namespace {
    constexpr const wchar_t* defaultTexture = L"Assets/particle.png";   // textureが空の時の画像(柔らかい白い円)
    constexpr float minDrawSize = 0.01f;                               // これより小さい粒は描かない(大きさ0は「画像の大きさ」の意味になってしまうため)

    // 向きdirectionを中心に、全体でspreadAngle度の範囲にばらつかせた向き(長さ1)を返す
    XMFLOAT3 RandomDirection(XMFLOAT3 direction, float spreadAngle, bool use3D) {
        XMFLOAT3 dir = Math::Normalize(direction);
        if (Math::Length(dir) < 0.5f) dir = { 0.f, 1.f, 0.f };   // 向きが0なら上
        float half = Math::Clamp(spreadAngle, 0.f, 360.f) * 0.5f;

        if (!use3D) {
            // 2D: XY平面で、向きを±half度の範囲で回す
            float rad = XMConvertToRadians(Math::RandomRange(-half, half));
            float c = cosf(rad), s = sinf(rad);
            return { dir.x * c - dir.y * s, dir.x * s + dir.y * c, dir.z };
        }

        // 3D: 向きを軸にした円錐の中で、ランダムな向き(球の上に均等に散らばる)
        XMFLOAT3 helper = (fabsf(dir.y) < 0.99f) ? XMFLOAT3{ 0.f, 1.f, 0.f } : XMFLOAT3{ 1.f, 0.f, 0.f };
        XMFLOAT3 right = Math::Normalize(Math::Cross(helper, dir));
        XMFLOAT3 up = Math::Cross(dir, right);
        float cosTheta = Math::RandomRange(cosf(XMConvertToRadians(half)), 1.f);
        float sinTheta = sqrtf((std::max)(0.f, 1.f - cosTheta * cosTheta));
        float phi = Math::RandomRange(0.f, XM_2PI);
        return dir * cosTheta + (right * cosf(phi) + up * sinf(phi)) * sinTheta;
    }
}

ParticleRenderer::ParticleRenderer(const ParticleSettings& s) : settings(s) {
    // 画像を借りる(Resourcesで共有するので、何個作っても読み込みは1回)
    texID = Resources::LoadTexture(settings.texture.empty() ? defaultTexture : settings.texture);
    particles.reserve(256);
}

void ParticleRenderer::Play() {
    playing = true;
}

void ParticleRenderer::Stop() {
    playing = false;
}

void ParticleRenderer::Burst(int count) {
    for (int i = 0; i < count; i++) Emit();
}

bool ParticleRenderer::IsPlaying() const {
    return playing || !particles.empty();
}

void ParticleRenderer::Emit() {
    // 上限まで出ていたら、新しくは出さない
    if ((int)particles.size() >= maxParticles) return;

    Particle p;
    // worldSpace=trueは出た場所(ワールド)から、falseはオブジェクトからの相対(0)から出す
    if (settings.worldSpace && GetOwner()) p.position = GetOwner()->transform.GetWorldTransform().position;

    float speed = Math::RandomRange(settings.speedMin, settings.speedMax);
    p.velocity = RandomDirection(settings.direction, settings.spreadAngle, settings.use3D) * speed;
    p.lifetime = (std::max)(0.001f, Math::RandomRange(settings.lifetimeMin, settings.lifetimeMax));   // 0以下だと0で割ってしまう
    particles.push_back(p);
}

void ParticleRenderer::Update(float dt) {
    // 出ている粒を動かす: 重力で速さを変えて、動かして、寿命が来たら消す(後ろの粒を入れ替えて詰める)
    for (size_t i = 0; i < particles.size();) {
        Particle& p = particles[i];
        p.age += dt;
        if (p.age >= p.lifetime) {
            p = particles.back();
            particles.pop_back();
            continue;
        }
        p.velocity = p.velocity + settings.gravity * dt;
        p.position = p.position + p.velocity * dt;
        i++;
    }

    // 新しい粒を出す: emitRate個/秒(端数は次のフレームに持ち越す)
    if (playing && settings.emitRate > 0.f) {
        emitTimer += settings.emitRate * dt;
        while (emitTimer >= 1.f) {
            emitTimer -= 1.f;
            Emit();
        }
    }

    // 使い捨てのエフェクトは、粒がすべて消えたら自分を消す
    if (destroyWhenFinished && !IsPlaying()) GetOwner()->Destroy();
}

void ParticleRenderer::Draw() {
    if (texID == UINT_MAX || particles.empty()) return;

    // worldSpace=falseの粒は、オブジェクトの今の位置に付いて動く
    XMFLOAT3 origin = { 0.f, 0.f, 0.f };
    if (!settings.worldSpace) origin = GetOwner()->transform.GetWorldTransform().position;

    BlendMode blend = settings.additive ? BlendMode::Additive : BlendMode::Soft;

    for (const Particle& p : particles) {
        // 寿命に沿って、大きさと色をstartからendへ変える
        float t = Math::Clamp(p.age / p.lifetime, 0.f, 1.f);
        float size = Math::Lerp(settings.startSize, settings.endSize, t);
        if (size < minDrawSize) continue;

        XMFLOAT4 color = {
            Math::Lerp(settings.startColor.x, settings.endColor.x, t),
            Math::Lerp(settings.startColor.y, settings.endColor.y, t),
            Math::Lerp(settings.startColor.z, settings.endColor.z, t),
            Math::Clamp(Math::Lerp(settings.startColor.w, settings.endColor.w, t), 0.f, 1.f)
        };
        if (color.w <= 0.f) continue;

        Transform transform;
        transform.position = origin + p.position;
        transform.scale = { size, size, 1.f };

        // 3Dは、カメラの方を完全に向く板(ビルボード)。半透明なので深度は書かない
        Image::Draw(transform, texID, color, SpriteSheet(), settings.use3D, false, false, false, blend, false);
    }
}

void ParticleRenderer::Uninit() {
    // 借りた画像を返す
    if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
    texID = UINT_MAX;
}
