#include "ModelAnimationComponent.h"
#include "../Core/GameObject.h"
#include "../Render3D/ModelRenderer.h"
#include <algorithm>
#include <cmath>
#include <windows.h>

void ModelAnimationComponent::Init() {
    // 同じオブジェクトの ModelRenderer を探す(先に ModelRenderer を付けておくこと)
    renderer = GetOwner()->GetComponent<ModelRenderer>();
    if (!renderer || !renderer->GetModel()) {
        OutputDebugStringA("★ ModelAnimationComponent: ModelRenderer が無いか、モデルを読めていないので、動かせません(ModelRenderer を先に付けてください)\n");
        renderer = nullptr;
        return;
    }
    model = renderer->GetModel();
    if (model->GetClipCount() == 0) {
        OutputDebugStringA("★ ModelAnimationComponent: このモデルにはアニメーション(クリップ)が入っていません(FBXの書き出しで Animation をオンにしてください)\n");
    }
}

std::vector<std::string> ModelAnimationComponent::GetClipNames() const {
    std::vector<std::string> names;
    if (!model) return names;
    for (int i = 0; i < model->GetClipCount(); i++) names.push_back(model->GetClip(i).name);
    return names;
}

bool ModelAnimationComponent::StartClip(const std::string& clipName, bool loopPlay) {
    if (!renderer || !model) return false;

    int index = model->FindClip(clipName);
    if (index < 0) {
        // 毎フレーム呼ばれても出力ウィンドウが埋まらないように、名前ごとに1回だけ知らせる
        if (warnedNames.insert(clipName).second) {
            OutputDebugStringA(("★ ModelAnimationComponent: クリップ \"" + clipName +
                "\" がモデルにありません(GetClipNames で名前を確認してください)\n").c_str());
        }
        return false;
    }

    // 再生中の同じクリップなら何もしない(毎フレーム呼ばれても最初に戻らない)。終わったクリップなら、もう一度最初から
    if (index == clipIndex && !finished) return false;

    clipIndex = index;
    time = 0.f;
    loop = loopPlay;
    finished = false;
    fadeDuration = 0.f;
    return true;
}

void ModelAnimationComponent::Play(const std::string& clipName, bool loopPlay) {
    if (StartClip(clipName, loopPlay)) Apply();   // 最初のポーズをすぐ反映する
}

void ModelAnimationComponent::CrossFade(const std::string& clipName, float seconds) {
    if (!renderer || !model) return;

    // 今のポーズを覚えてから切り替える(StartClipが失敗した時=同じクリップ・名前なし、は何もしない)
    std::vector<ModelPose> from;
    CapturePose(from);
    if (!StartClip(clipName, true)) return;

    if (seconds > 0.f) {
        fadeFrom = std::move(from);
        fadeTime = 0.f;
        fadeDuration = seconds;
    }
    Apply();
}

void ModelAnimationComponent::CapturePose(std::vector<ModelPose>& out) const {
    out.clear();
    if (!renderer || !model) return;
    for (int n = 0; n < model->GetNodeCount(); n++) {
        Transform* t = renderer->GetNodeTransform(n);
        ModelPose pose;
        pose.position = t->position;
        pose.scale = t->scale;
        // Transform::GetLocalMatrix と同じ回転(Pitch=x, Yaw=y, Roll=z)をクォータニオンにする
        XMVECTOR q = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(t->rotate.x), XMConvertToRadians(t->rotate.y), XMConvertToRadians(t->rotate.z));
        XMStoreFloat4(&pose.rotation, q);
        out.push_back(pose);
    }
}

void ModelAnimationComponent::SamplePose(int node, ModelPose& out) const {
    const ModelClip& clip = model->GetClip(clipIndex);
    const std::vector<ModelPose>& keys = clip.nodePoses[node];

    // 今の時刻の前後2つの値を、線形(回転はslerp)でつなぐ
    float position = (std::max)(0.f, time) * clip.fps;
    int i0 = (int)floorf(position);
    if (i0 >= clip.frameCount - 1) { out = keys[clip.frameCount - 1]; return; }
    float a = position - (float)i0;
    const ModelPose& p0 = keys[i0];
    const ModelPose& p1 = keys[i0 + 1];

    XMStoreFloat3(&out.position, XMVectorLerp(XMLoadFloat3(&p0.position), XMLoadFloat3(&p1.position), a));
    XMStoreFloat3(&out.scale, XMVectorLerp(XMLoadFloat3(&p0.scale), XMLoadFloat3(&p1.scale), a));
    XMStoreFloat4(&out.rotation, XMQuaternionSlerp(XMLoadFloat4(&p0.rotation), XMLoadFloat4(&p1.rotation), a));
}

void ModelAnimationComponent::Apply() {
    if (!renderer || !model || clipIndex < 0) return;

    // CrossFade中は、始めた時のポーズから新しいクリップのポーズへ混ぜていく
    float blend = (fadeDuration > 0.f) ? (std::min)(1.f, fadeTime / fadeDuration) : 1.f;
    bool fading = fadeDuration > 0.f && blend < 1.f && (int)fadeFrom.size() == model->GetNodeCount();

    for (int n = 0; n < model->GetNodeCount(); n++) {
        Transform* t = renderer->GetNodeTransform(n);
        ModelPose pose;
        SamplePose(n, pose);

        if (fading) {
            const ModelPose& from = fadeFrom[n];
            XMStoreFloat3(&pose.position, XMVectorLerp(XMLoadFloat3(&from.position), XMLoadFloat3(&pose.position), blend));
            XMStoreFloat3(&pose.scale, XMVectorLerp(XMLoadFloat3(&from.scale), XMLoadFloat3(&pose.scale), blend));
            XMStoreFloat4(&pose.rotation, XMQuaternionSlerp(XMLoadFloat4(&from.rotation), XMLoadFloat4(&pose.rotation), blend));
        }

        t->position = pose.position;
        t->scale = pose.scale;
        t->rotate = Model::QuaternionToEuler(pose.rotation);
    }
}

void ModelAnimationComponent::Update(float dt) {
    // 終わった動きは最後のポーズのまま(以後は書き込まないので、手で動かせる)
    if (!renderer || !model || clipIndex < 0 || finished) return;

    if (fadeDuration > 0.f) {
        fadeTime += dt;
        if (fadeTime >= fadeDuration) fadeDuration = 0.f;
    }

    const ModelClip& clip = model->GetClip(clipIndex);
    time += dt * speed;
    if (time >= clip.duration) {
        if (loop && clip.duration > 0.f) {
            time = fmodf(time, clip.duration);
        }
        else {
            time = clip.duration;
            finished = !loop;   // ループなのに長さ0なら、終わらせずに最初のポーズのまま
        }
    }

    Apply();
}
