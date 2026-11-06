#include "NetworkTransformComponent.h"
#include "NetworkObjects.h"
#include "NetworkConfig.h"
#include "../Core/GameObject.h"
#include "../Core/MathUtil.h"
#include <algorithm>
#include <cmath>

namespace {
    // スナップショット1件の「何が入っているか」の印
    constexpr uint8_t flagPosition = 1;
    constexpr uint8_t flagRotation = 2;
    constexpr uint8_t flagScale = 4;
    constexpr uint8_t flag2D = 8;

    constexpr size_t maxSamples = 32;       // 溜める写真の数の上限
    constexpr int scaleResendInterval = 60; // 大きさは変わらなくても、これだけ送る度に1回は送り直す(途中参加の人用)

    float Clamp(float v) {
        return (std::max)(-Network::positionLimit, (std::min)(Network::positionLimit, v));
    }
}

void NetworkTransformComponent::Init() {
    NetworkObjects::RegisterTransformSync(this);
}

void NetworkTransformComponent::Uninit() {
    NetworkObjects::UnregisterTransformSync(this);
}

float NetworkTransformComponent::LerpAngle(float from, float to, float t) {
    // 差を -180〜180 に収めてから補間する(350度 → 10度 は +20度)
    float diff = std::fmod(to - from, 360.f);
    if (diff > 180.f) diff -= 360.f;
    if (diff < -180.f) diff += 360.f;
    return from + diff * t;
}

void NetworkTransformComponent::WriteState(NetworkWriter& writer) {
    const Transform& t = GetOwner()->transform;

    // 大きさは、変わった時と、60回に1回だけ送る
    bool sendScale = false;
    if (settings.syncScale) {
        scaleResendCounter++;
        if (scaleResendCounter >= scaleResendInterval || t.scale.x != lastSentScale.x || t.scale.y != lastSentScale.y || t.scale.z != lastSentScale.z) {
            sendScale = true;
            scaleResendCounter = 0;
            lastSentScale = t.scale;
        }
    }

    uint8_t flags = 0;
    if (settings.syncPosition) flags |= flagPosition;
    if (settings.syncRotation) flags |= flagRotation;
    if (sendScale) flags |= flagScale;
    if (settings.is2D) flags |= flag2D;
    writer.WriteU8(flags);

    if (flags & flagPosition) {
        writer.WriteFloat(t.position.x);
        writer.WriteFloat(t.position.y);
        if (!settings.is2D) writer.WriteFloat(t.position.z);
    }
    if (flags & flagRotation) {
        if (!settings.is2D) {
            writer.WriteFloat(t.rotate.x);
            writer.WriteFloat(t.rotate.y);
        }
        writer.WriteFloat(t.rotate.z);
    }
    if (flags & flagScale) {
        writer.WriteFloat(t.scale.x);
        writer.WriteFloat(t.scale.y);
        writer.WriteFloat(t.scale.z);
    }
}

bool NetworkTransformComponent::ReadState(NetworkReader& reader, double hostTime) {
    const uint8_t flags = reader.ReadU8();
    if (!reader.IsOk() || (flags & ~(flagPosition | flagRotation | flagScale | flag2D)) != 0) return false;
    const bool is2D = (flags & flag2D) != 0;

    // 入っていない項目は、直前の写真(無ければ今の Transform)の値を引き継ぐ
    Sample sample;
    const Transform& t = GetOwner()->transform;
    if (!samples.empty()) sample = samples.back();
    else {
        sample.position = t.position;
        sample.rotate = t.rotate;
        sample.scale = t.scale;
    }
    sample.time = hostTime;

    if (flags & flagPosition) {
        sample.position.x = Clamp(reader.ReadFloat());
        sample.position.y = Clamp(reader.ReadFloat());
        if (!is2D) sample.position.z = Clamp(reader.ReadFloat());
    }
    if (flags & flagRotation) {
        if (!is2D) {
            sample.rotate.x = reader.ReadFloat();
            sample.rotate.y = reader.ReadFloat();
        }
        sample.rotate.z = reader.ReadFloat();
    }
    if (flags & flagScale) {
        sample.scale.x = Clamp(reader.ReadFloat());
        sample.scale.y = Clamp(reader.ReadFloat());
        sample.scale.z = Clamp(reader.ReadFloat());
    }
    if (!reader.IsOk()) return false;

    // 古い写真(順番が入れ替わって届いた物)は捨てる
    if (!samples.empty() && sample.time <= samples.back().time) return true;
    samples.push_back(sample);
    if (samples.size() > maxSamples) samples.pop_front();
    return true;
}

void NetworkTransformComponent::Interpolate(double renderTime) {
    if (samples.empty() || !GetOwner()) return;

    // 表示の時刻より前に終わっている写真を捨てる(表示の時刻をはさむ2枚を残す)
    while (samples.size() >= 2 && samples[1].time <= renderTime) samples.pop_front();

    Sample result = samples.front();
    if (samples.size() >= 2 && renderTime > samples[0].time) {
        const Sample& a = samples[0];
        const Sample& b = samples[1];
        const float dx = b.position.x - a.position.x, dy = b.position.y - a.position.y, dz = b.position.z - a.position.z;
        if (std::sqrt(dx * dx + dy * dy + dz * dz) > settings.teleportDistance) {
            result = b;   // ワープ(リスポーンなど)は補間せずに飛ぶ
        }
        else {
            const float t = (float)((renderTime - a.time) / (b.time - a.time));
            result.position = Math::Lerp(a.position, b.position, t);
            result.rotate = { LerpAngle(a.rotate.x, b.rotate.x, t), LerpAngle(a.rotate.y, b.rotate.y, t), LerpAngle(a.rotate.z, b.rotate.z, t) };
            result.scale = Math::Lerp(a.scale, b.scale, t);
        }
    }
    // 表示の時刻が最新より後(通信が遅れた)なら、最新の位置で止める。予測では動かさない

    Transform& t = GetOwner()->transform;
    if (settings.syncPosition) {
        t.position.x = result.position.x;
        t.position.y = result.position.y;
        if (!settings.is2D) t.position.z = result.position.z;
    }
    if (settings.syncRotation) {
        if (!settings.is2D) {
            t.rotate.x = result.rotate.x;
            t.rotate.y = result.rotate.y;
        }
        t.rotate.z = result.rotate.z;
    }
    if (settings.syncScale) t.scale = result.scale;
}
