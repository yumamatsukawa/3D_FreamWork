#pragma once
#include <DirectXMath.h>

// 画面のどこを基準にして置くか(9か所)。画面サイズが変わっても、隅や中央からの距離は変わらない。
// 画面座標は「画面中央が(0,0)、Y+=上、単位はゲーム画面のピクセル」(Text::Drawと同じ)。
//
// 使い方:
//   XMFLOAT2 pos = GetAnchoredPosition(Anchor::TopLeft, { 40.f, -40.f });   // 左上から右に40、下に40
//   Text::Draw(L"Score", pos.x, pos.y, style);
enum class Anchor {
    TopLeft,    Top,     TopRight,
    Left,       Center,  Right,
    BottomLeft, Bottom,  BottomRight
};

// 基準の位置(画面の隅・辺の真ん中・中央)に offset を足した、画面座標を返す。
// offset は画面座標系(Y+=上)でのずらす量なので、下へずらす時はYをマイナスにする
// 例: TopLeft に {40,-40} なら、左上の隅から右へ40、下へ40
// 画面の大きさは Graphics::width / Graphics::height から毎回計算する(画面サイズが変わってもずれない)
DirectX::XMFLOAT2 GetAnchoredPosition(Anchor anchor, DirectX::XMFLOAT2 offset);

// 大きさ size の四角を、四角の anchor 側(左上なら左上の角、下なら下の辺の真ん中、中央なら中心)を
// GetAnchoredPosition(anchor, offset) に合わせて置いた時の、四角の「中心」の画面座標を返す。
// ゲージやスライダーのように「左上から右へ40、下へ40の所にバーを置く」時に使う(四角がはみ出さない)
DirectX::XMFLOAT2 GetAnchoredCenter(Anchor anchor, DirectX::XMFLOAT2 offset, DirectX::XMFLOAT2 size);
