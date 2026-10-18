#pragma once

// 描画の重さを測るカウンタ(描画命令の数・三角形の数)。LODの効果を確かめるために使う。
// Visibility::GetCulledCount と同じく、すべて「前のフレームの数」を返す(HUDが描く時に今のフレームが途中でも、数が揺れないように)。
//
// 決まり:
// - 数えるのは3Dの描画だけ(Mesh / SkinMesh / 水面)。2Dのスプライト・文字は数えない。当たり判定の線(ギズモ)の描画も数える
// - 「描画命令」は GPU に出した Draw の数。枠線は1つの物につき3回出す(本物の印・枠線・印を消す)ので、枠線は物の数×3
// - 「三角形」は、その命令で描いた三角形の数。線のメッシュは0
// - 種類は3つ: 通常(本番の描画)/ 影(影のパス)/ 枠線。合計は GetDrawCallTotal / GetTriangleCount
// - 距離で省いた数(GetLodCulledCount)は、LodSettings の drawDistance / shadowDistance / outlineDistance で省いた回数の合計。
//   画面の外で省いた数は Visibility::GetCulledCount で別に見る
// - 数えるのは単純な足し算だけ。Release でも有効
namespace RenderStats {
    // 通常の描画
    int GetDrawCallCount();
    int GetDrawTriangleCount();
    // 影のパス
    int GetShadowDrawCount();
    int GetShadowTriangleCount();
    // 枠線
    int GetOutlineDrawCount();
    int GetOutlineTriangleCount();
    // 合計(通常+影+枠線)
    int GetDrawCallTotal();
    int GetTriangleCount();
    // 距離で省いた数
    int GetLodCulledCount();

    // ─── Engine内部用(GameEngineと、Mesh / SkinMesh / WaterRenderer / 各Renderer だけが呼ぶ) ───
    // 前のフレームの数を確定させて、数え直す。Visibility::BeginFrame の隣で1回
    void BeginFrame();
    void AddDraw(int triangleCount);
    void AddShadowDraw(int triangleCount);
    void AddOutlineDraw(int triangleCount);   // 1回の命令分(DrawOutline は3回呼ぶ)
    void AddLodCulled();
}
