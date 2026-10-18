#include "SliderComponent.h"
#include "../Render/Image.h"
#include "../Core/Input.h"
#include "../Core/Time.h"
#include "../Core/MathUtil.h"
#include <windows.h>
#include <cmath>
#include <utility>

using namespace DirectX;

namespace {
    int draggingCount = 0;   // つまみをつかんでいるスライダーの数(IsAnyDragging用)

    // 物理的な左ボタン(マウスの設定で左右を入れ替えていれば、右ボタンのこと)が、今押されているか。
    // GetAsyncKeyState は論理ボタンではなく物理ボタンの状態を返すので、入れ替えの設定を見て選ぶ
    bool IsMouseButtonPhysicallyDown() {
        const int button = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
        return (GetAsyncKeyState(button) & 0x8000) != 0;
    }
}

bool SliderComponent::IsAnyDragging() {
    return draggingCount > 0;
}

XMFLOAT2 SliderComponent::GetCenter() const {
    return GetAnchoredCenter(anchor, offset, size);
}

float SliderComponent::GetValueRatio() const {
    if (!(maxValue > minValue)) return 0.f;
    return Math::Clamp((value - minValue) / (maxValue - minValue), 0.f, 1.f);
}

float SliderComponent::NormalizeValue(float newValue) const {
    float result = newValue;

    // 刻み幅: 最小値から刻み幅ごとの、一番近い値にする
    if (step > 0.f) result = minValue + roundf((newValue - minValue) / step) * step;

    // 範囲に収める。端にほんの少し(小数の誤差)届かない値は、端そのものにする
    result = Math::Clamp(result, minValue, maxValue);
    const float epsilon = (maxValue - minValue) * 1e-5f;
    if (fabsf(result - maxValue) <= epsilon) result = maxValue;
    if (fabsf(result - minValue) <= epsilon) result = minValue;
    return result;
}

void SliderComponent::SetRange(float minimum, float maximum) {
    if (!std::isfinite(minimum) || !std::isfinite(maximum)) return;
    if (minimum > maximum) std::swap(minimum, maximum);
    minValue = minimum;
    maxValue = maximum;
    value = NormalizeValue(value);   // 範囲の外に出た今の値は収める(onChangedは呼ばない)
}

void SliderComponent::SetValue(float newValue) {
    if (!std::isfinite(newValue)) return;
    value = NormalizeValue(newValue);
}

void SliderComponent::SetStep(float stepSize) {
    step = (std::isfinite(stepSize) && stepSize > 0.f) ? stepSize : 0.f;
    value = NormalizeValue(value);
}

void SliderComponent::SetAnchor(Anchor anchorPoint, XMFLOAT2 anchorOffset) {
    anchor = anchorPoint;
    offset = anchorOffset;
}

void SliderComponent::SetSize(float width, float height) {
    if (!(width > 0.f) || !(height > 0.f)) return;   // 0以下(やNaN)は無視
    size = { width, height };
}

void SliderComponent::SetColors(XMFLOAT4 track, XMFLOAT4 fill, XMFLOAT4 handle) {
    trackColor = track;
    fillColor = fill;
    handleColor = handle;
}

void SliderComponent::SetLabel(const std::wstring& text, const TextStyle& style) {
    label = text;
    labelStyle = style;
}

bool SliderComponent::IsPointOnSlider(XMFLOAT2 point) const {
    // 溝と、はみ出しているつまみを含む四角
    XMFLOAT2 center = GetCenter();
    const float halfWidth = size.x * 0.5f + handleWidth * 0.5f;
    const float halfHeight = (size.y + handleExtraHeight) * 0.5f;
    return fabsf(point.x - center.x) <= halfWidth && fabsf(point.y - center.y) <= halfHeight;
}

void SliderComponent::ApplyMouse(float mouseX) {
    // マウスのx → 溝の中での位置(0〜1。溝の外に出ていても端に収める)
    const float left = GetCenter().x - size.x * 0.5f;
    const float t = Math::Clamp((mouseX - left) / size.x, 0.f, 1.f);

    // 両端は誤差が出ないよう、最小値・最大値そのものにする
    float newValue = minValue + t * (maxValue - minValue);
    if (t >= 1.f) newValue = maxValue;
    if (t <= 0.f) newValue = minValue;
    newValue = NormalizeValue(newValue);

    // 実際に変わった時だけ知らせる
    if (newValue == value) return;
    value = newValue;
    if (onChanged) onChanged(value);
}

void SliderComponent::EndDrag() {
    if (!dragging) return;
    dragging = false;
    --draggingCount;
    if (onDragEnd) onDragEnd(value);
}

void SliderComponent::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作できない(つかんでいたら離したことにする)
    if (Time::IsPaused()) {
        EndDrag();
        return;
    }

    // マウスの位置(Input::GetMousePositionはY+=下なので、画面座標(Y+=上)に直す)
    XMFLOAT2 mouse = Input::GetMousePosition();
    mouse.y = -mouse.y;

    // 溝やつまみの上で押し始めたら、つかむ
    if (!dragging && Input::GetKeyDown(MOUSE_LEFT) && IsPointOnSlider(mouse)) {
        dragging = true;
        ++draggingCount;
    }
    if (!dragging) return;

    // つかんでいる間は、溝の外に出てもマウスに付いてくる。ボタンを離したら終わり。
    // Input::GetKeyPress が使う GetKeyboardState は、ウィンドウの外で離したことに気付けず「押されたまま」に見えることがある
    // (溝の外へ出して離す操作は普通に起きる)。そのため、物理的なボタンの状態(GetAsyncKeyState)も見て、離れていたら離したことにする。
    // 他のアプリのウィンドウをクリックしている時にドラッグが終わるのは、安全側なので問題ない
    if (Input::GetKeyPress(MOUSE_LEFT) && IsMouseButtonPhysicallyDown()) ApplyMouse(mouse.x);
    else EndDrag();
}

void SliderComponent::Draw() {
    const XMFLOAT2 center = GetCenter();
    const float left = center.x - size.x * 0.5f;
    const float ratio = GetValueRatio();
    const float handleX = left + size.x * ratio;

    // 溝
    Image::DrawRect(center, size, trackColor);

    // 溝の左からつまみまで
    if (ratio > 0.f) {
        const float width = size.x * ratio;
        Image::DrawRect({ left + width * 0.5f, center.y }, { width, size.y }, fillColor);
    }

    // つまみ(つかんでいる間は少し暗くする)
    const float shade = dragging ? 0.75f : 1.f;
    Image::DrawRect({ handleX, center.y }, { handleWidth, size.y + handleExtraHeight },
        { handleColor.x * shade, handleColor.y * shade, handleColor.z * shade, handleColor.w });

    // 文字(つまみの上に出す。寄せ方に合わせて、溝の左端・中心・右端に合わせる)
    if (!label.empty()) {
        float labelX = center.x;
        if (labelStyle.align == TextAlign::Left) labelX = left;
        else if (labelStyle.align == TextAlign::Right) labelX = left + size.x;
        const float labelY = center.y + (size.y + handleExtraHeight) * 0.5f + labelGap + labelStyle.size * 0.5f;
        Text::Draw(label, labelX, labelY, labelStyle);
    }
}

void SliderComponent::OnActiveChanged(bool active) {
    // 非アクティブになった時につかんでいたら、離したことにする(つかんだままだと、ボタンがずっと反応しなくなるため)
    if (!active) EndDrag();
}

void SliderComponent::Uninit() {
    // 壊される時につかんでいたら、数だけ戻す(onDragEndは呼ばない。壊れる物を触らせないため)
    if (dragging) {
        dragging = false;
        --draggingCount;
    }
}
