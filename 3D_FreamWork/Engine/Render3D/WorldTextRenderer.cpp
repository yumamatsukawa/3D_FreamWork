#include "WorldTextRenderer.h"
#include "../Core/GameObject.h"
#include "../Render/Image.h"
#include "../Render/Visibility.h"
#include "RenderStats.h"
#include "Material.h"
#include <algorithm>
#include <cmath>
#include <vector>

using namespace DirectX;

namespace {
    // 見た目がすべて同じか
    bool SameStyle(const TextStyle& a, const TextStyle& b) {
        return a.size == b.size && a.color.x == b.color.x && a.color.y == b.color.y && a.color.z == b.color.z && a.color.w == b.color.w &&
            a.fontName == b.fontName && a.align == b.align && a.bold == b.bold &&
            a.outlineColor.x == b.outlineColor.x && a.outlineColor.y == b.outlineColor.y && a.outlineColor.z == b.outlineColor.z &&
            a.outlineColor.w == b.outlineColor.w && a.outlineWidth == b.outlineWidth;
    }

    // 板の頂点(-0.5〜0.5の正方形。正面は -Z 側。UV は左上が(0,0))
    std::vector<MeshVertex> CreateQuad() {
        const XMFLOAT3 normal = { 0.f, 0.f, -1.f };
        const XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
        const MeshVertex topLeft = { { -0.5f, 0.5f, 0.f }, normal, white, { 0.f, 0.f } };
        const MeshVertex topRight = { { 0.5f, 0.5f, 0.f }, normal, white, { 1.f, 0.f } };
        const MeshVertex bottomLeft = { { -0.5f, -0.5f, 0.f }, normal, white, { 0.f, 1.f } };
        const MeshVertex bottomRight = { { 0.5f, -0.5f, 0.f }, normal, white, { 1.f, 1.f } };
        return { topLeft, topRight, bottomLeft, topRight, bottomRight, bottomLeft };
    }
}

WorldTextRenderer::~WorldTextRenderer() {
    // Uninit が呼ばれずに壊された時も、借りたテクスチャを返す(Uninit 済みなら何もしない)
    ReleaseTexture();
}

void WorldTextRenderer::SetText(const std::wstring& value) {
    if (value == text) return;
    text = value;
    dirty = true;
}

void WorldTextRenderer::SetStyle(const TextStyle& value) {
    if (SameStyle(value, style)) return;
    TextStyle a = value, b = style;
    a.color.w = b.color.w = 1.f;   // 色の濃さ(w)だけの変更は、絵を作り直さない(板の色の濃さで表す)
    if (!SameStyle(a, b)) dirty = true;
    style = value;
}

void WorldTextRenderer::SetColor(XMFLOAT4 color) {
    TextStyle next = style;
    next.color = color;
    SetStyle(next);
}

void WorldTextRenderer::SetPixelsPerUnit(float value) {
    if (value > 0.f) pixelsPerUnit = value;
}

void WorldTextRenderer::SetBackground(XMFLOAT4 color, float paddingPixels) {
    paddingPixels = (std::max)(paddingPixels, 0.f);
    if (color.x == background.x && color.y == background.y && color.z == background.z && color.w == background.w && paddingPixels == padding) return;
    background = color;
    padding = paddingPixels;
    dirty = true;
}

XMFLOAT2 WorldTextRenderer::GetWorldSize() const {
    if (!texture) return { 0.f, 0.f };
    const float perUnit = pixelsPerUnit * shrink;
    return { texture->width / perUnit, texture->height / perUnit };
}

void WorldTextRenderer::ReleaseTexture() {
    if (texture) TextTexture::Release(texture);
    texture = nullptr;
}

void WorldTextRenderer::Rebuild() {
    dirty = false;

    // 文字の大きさを測る。テクスチャが上限を超える時は、文字の大きさを縮めて収める(板のワールドの大きさは shrink で保つ)
    TextStyle drawStyle = style;
    float drawPadding = padding;
    float factor = 1.f;
    int width = 0, height = 0;
    if (text.empty() || !TextTexture::Measure(text, drawStyle, drawPadding, width, height)) {
        ReleaseTexture();
        return;
    }
    for (int i = 0; i < 8 && (width > maxTextureSize || height > maxTextureSize); i++) {
        const float scale = (float)maxTextureSize / (float)(std::max)(width, height) * 0.98f;
        factor *= scale;
        drawStyle.size *= scale;
        drawStyle.outlineWidth *= scale;
        drawPadding *= scale;
        if (!TextTexture::Measure(text, drawStyle, drawPadding, width, height)) { ReleaseTexture(); return; }
    }
    if (width > maxTextureSize || height > maxTextureSize) {
        OutputDebugStringA("★ WorldTextRenderer: 文字が大きすぎて、テクスチャ(最大2048)に収まりません\n");
        ReleaseTexture();
        return;
    }
    if (factor < 1.f) OutputDebugStringA("★ WorldTextRenderer: 文字が大きいので、テクスチャの細かさを下げました(最大2048)\n");

    // 新しい絵を先に借りてから、前の絵を返す(同じ絵なら、いったん0になって作り直すことがないように)
    TextTexture::Desc desc;
    desc.text = text;
    desc.style = drawStyle;
    desc.background = background;
    desc.width = width;
    desc.height = height;
    TextTexture::Shared* next = TextTexture::Acquire(desc);
    ReleaseTexture();
    texture = next;
    shrink = factor;
}

void WorldTextRenderer::Init() {
    mesh = std::make_shared<Mesh>();
    meshReady = mesh->Init(CreateQuad());

    // 半透明の文字は、壁などの不透明な物より後に描く(先に描くと、後ろの壁に塗りつぶされる)。変えたい時は SetDrawPriority
    if (GetOwner()->GetDrawPriority() == 0) GetOwner()->SetDrawPriority(defaultDrawPriority);
}

void WorldTextRenderer::Draw() {
    if (!meshReady || text.empty()) return;
    if (dirty) Rebuild();
    if (!texture) return;

    const Camera& camera = Image::GetCamera3D();
    const XMVECTOR eye = XMLoadFloat3(&camera.position);
    const Transform& transform = GetOwner()->transform;
    const XMMATRIX ownerWorld = transform.GetWorldMatrix();

    // 板の大きさ(ワールド)と、寄せ方による板の横のずれ(Center=0 / Left=板の半分右 / Right=板の半分左)
    const XMFLOAT2 size = GetWorldSize();
    const float offsetX = style.align == TextAlign::Left ? size.x * 0.5f : style.align == TextAlign::Right ? -size.x * 0.5f : 0.f;
    const XMMATRIX plate = XMMatrixScaling(size.x, size.y, 1.f);
    const XMMATRIX offset = XMMatrixTranslation(offsetX, 0.f, 0.f);

    // ビルボード: オーナーの位置と大きさだけ使い、向きはカメラの方へ(Sprite のビルボードと同じ式)
    XMMATRIX after = ownerWorld;   // 板の大きさ・ずれの後にかける行列
    if (billboard) {
        XMFLOAT3 forward;
        XMStoreFloat3(&forward, XMVector3Normalize(XMLoadFloat3(&camera.target) - eye));
        const float yaw = lockY ? 0.f : atan2f(forward.x, forward.z);
        const float pitch = lockX ? 0.f : atan2f(-forward.y, sqrtf(forward.x * forward.x + forward.z * forward.z));
        const XMMATRIX facing = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.f);
        const XMMATRIX roll = XMMatrixRotationZ(XMConvertToRadians(transform.rotate.z));   // 板の中での回転
        after = roll * XMMatrixScaling(XMVectorGetX(XMVector3Length(ownerWorld.r[0])), XMVectorGetX(XMVector3Length(ownerWorld.r[1])),
            XMVectorGetX(XMVector3Length(ownerWorld.r[2]))) * facing * XMMatrixTranslationFromVector(ownerWorld.r[3]);
    }

    // 裏側から見ているか(板の正面 -Z 側にカメラがいなければ裏)。裏で見えない設定なら描かない。見える設定なら、板を裏返して文字が読める向きにする
    XMMATRIX world = plate * offset * after;
    const XMVECTOR center = XMVector3TransformCoord(XMVectorZero(), world);
    const XMVECTOR normal = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0.f, 0.f, -1.f, 0.f), world));
    if (XMVectorGetX(XMVector3Dot(normal, eye - center)) < 0.f) {
        if (!doubleSided) return;
        world = plate * XMMatrixRotationY(XM_PI) * offset * after;
    }

    // 遠い物・画面の外は描かない(球は、板の対角線の半分)
    XMFLOAT3 centerPos;
    XMStoreFloat3(&centerPos, center);
    const float widthWorld = XMVectorGetX(XMVector3Length(world.r[0]));
    const float heightWorld = XMVectorGetX(XMVector3Length(world.r[1]));
    const float radius = 0.5f * sqrtf(widthWorld * widthWorld + heightWorld * heightWorld);
    if (lod.drawDistance > 0.f && Lod::IsBeyond(centerPos, radius, lod.drawDistance)) {
        RenderStats::AddLodCulled();
        return;
    }
    if (frustumCulling && !Visibility::IsSphereInView3D(centerPos, radius)) {
        Visibility::CountCulled();
        return;
    }

    // 描く: 文字の濃さ(color.w)は板の色の濃さで表す。背景も文字も不透明な時だけ深度に書く
    Material material;
    material.color = { 1.f, 1.f, 1.f, style.color.w };
    material.lit = lit;
    material.depthWrite = background.w >= 1.f && style.color.w >= 1.f;
    mesh->SetTexture(texture->srv);
    mesh->Draw(world, camera, material, Image::GetLight());
}

void WorldTextRenderer::Uninit() {
    ReleaseTexture();
    if (meshReady) {
        mesh->SetTexture(nullptr);
        mesh->Uninit();
        meshReady = false;
    }
}
