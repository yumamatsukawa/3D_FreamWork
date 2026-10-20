#pragma once
#include "../../../../Engine/UI/Text.h"

// デモの文字の見た目を作る(日本語フォント・黒い縁取り・縦は中心)。Draw の中で Text::Draw に渡す
// 例: Text::Draw(L"Score", -600.f, 100.f, MakeDemoTextStyle(24.f, { 1.f, 0.9f, 0.4f, 1.f }));
TextStyle MakeDemoTextStyle(float size, DirectX::XMFLOAT4 color = { 1.f, 1.f, 1.f, 1.f }, TextAlign align = TextAlign::Left);
