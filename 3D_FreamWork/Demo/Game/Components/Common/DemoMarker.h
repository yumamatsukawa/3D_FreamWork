#pragma once
#include "../../Scenes/Common/DemoRegistry.h"
#include <DirectXMath.h>
#include <string>

// デモで使う「点と線の印」を描く道具(Math・Coord などが共有する。Component の Draw の中で呼ぶ)。
// 2D版はワールドの位置を2Dカメラで、3D版は3Dカメラで画面の位置に直して描く(3Dはカメラの後ろの点は描かない)

// ワールドの位置 → 画面の位置。見える時だけ true(3Dのカメラの後ろは false)
bool WorldToDemoScreen(DemoMode mode, const DirectX::XMFLOAT3& position, DirectX::XMFLOAT2& screen);

// 点(size: 四角の大きさ)
void DrawDemoPoint(DemoMode mode, const DirectX::XMFLOAT3& position, float size, const DirectX::XMFLOAT4& color);

// 線(点を並べて引く。線を引く API は無いため)。先は大きな点にして矢印の先にする
// dotCount: 点の数(長い線は大きくして、点の間を詰める)
void DrawDemoLine(DemoMode mode, const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to, const DirectX::XMFLOAT4& color, int dotCount = 28);

// 点の近くの文字(yOffset: 画面で上へずらす量。下へずらす時はマイナス)
void DrawDemoLabel(DemoMode mode, const DirectX::XMFLOAT3& position, const std::wstring& text, float yOffset = 24.f);

// 円(2DはXYの面、3Dは地面と水平のXZの面)を点で並べて描く
void DrawDemoRing(DemoMode mode, const DirectX::XMFLOAT3& center, float radius, const DirectX::XMFLOAT4& color, int dotCount = 48);

// 四角の枠(2DはXYの面、3Dは水平のXZの面。size は横と奥行き)を点で並べて描く
void DrawDemoRect(DemoMode mode, const DirectX::XMFLOAT3& center, DirectX::XMFLOAT2 size, const DirectX::XMFLOAT4& color);
