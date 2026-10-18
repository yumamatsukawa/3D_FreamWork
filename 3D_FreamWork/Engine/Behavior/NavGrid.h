#pragma once
#include "MoveComponent.h"
#include <DirectXMath.h>
#include <cstdint>
#include <string>
#include <vector>

class TerrainRenderer;

// 格子(NavGrid)の設定。2D(MovePlane::XY)でも3Dの地面(MovePlane::XZ)でも同じ形で使う
struct NavGridSettings {
    DirectX::XMFLOAT3 origin{};                 // 格子の左手前の角のワールド座標(XYなら x,y,z。XZなら x,z だけ使う。高さは各マスが覚える)
    DirectX::XMFLOAT2 size{ 6000.f, 6000.f };   // 格子の大きさ(XYなら横とたて、XZなら X と Z)
    float cellSize = 50.f;                      // 1マスの辺の長さ。大きいほど軽くて粗い。歩く物の半径くらいがおすすめ
    MovePlane plane = MovePlane::XZ;            // 歩く平面(MoveComponent と同じ)
    float agentRadius = 40.f;                   // 歩く物の半径。この分だけ障害物を太らせて、壁にこすらない道を作る
    std::string obstacleTag = "Wall";           // このタグの物が障害物(VisionComponent の壁と同じタグ)
    // ── 3Dの高さ(planeがXZの時だけ使う) ──
    float baseHeight = 0.f;                     // 地形が無い所の地面の高さ(Sample3D の Ground なら -50)
    float agentHeight = 100.f;                  // 歩く物の背の高さ。壁を探す高さの上の端
    float maxStepHeight = 30.f;                 // 登れる段差の高さ。壁を探す高さの下の端(これより低い物は障害物にしない)
    float maxSlopeDegrees = 40.f;               // 登れる坂の角度。これより急なマスは通れない
};

// 地面を升目に区切って、各マスが「通れるか」(3Dなら地面の高さも)を覚えておく。経路探索(PathFinder)の土台。
//
// 決まり:
// - マスは (cellX, cellZ) で表す。cellX は X 方向、cellZ は平面のもう一方(XYなら Y 方向、XZなら Z 方向)。0 が格子の左手前
// - 障害物は3種類を別々に覚える: 手で登録(SetBlocked / SetBlockedBox)・物理から(BakeFromPhysics)・地形から(BakeFromTerrain)。
//   どれか1つでも塞いでいれば通れない。焼き直しは、その種類の分だけを作り直す(手で登録した分は消えない。全部消すのは Clear)
// - 障害物は agentRadius だけ太らせる(壁から半径ぶん離れたマスも通れなくなる)。歩く物を「1つの点」として扱えるようにするため
// - 格子の外は通れない。格子は有限の範囲(上限 maxCells。超える設定は cellSize を自動で大きくして出力ウィンドウに知らせる)
// - 3D(XZ)は「平面の升目 + 各マスの地面の高さ」の2.5次元。同じ XZ に床が複数ある場所(橋の下など)は扱えない
class NavGrid {
public:
    static constexpr int maxCells = 512 * 512;   // 格子のマスの数の上限

    explicit NavGrid(const NavGridSettings& newSettings);

    // ── 障害物を取り込む ──
    void Clear();                                                                   // 全部の種類の障害物を消す(全マス通れる・高さは baseHeight に戻る)
    void SetBlocked(int cellX, int cellZ, bool blocked);                            // 手で1マス
    void SetBlockedBox(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 size, bool blocked = true);   // 手で箱の範囲(池・柵など物理の無い障害物。高さは見ない。太らせる)
    void BakeFromPhysics();                                                         // 全マスを Physics::OverlapBox(obstacleTag) で調べて、壁に重なるマスを通れなくする
    void BakeRegionFromPhysics(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 size);   // 範囲だけ調べ直す(動く壁用)
    void BakeFromTerrain(const TerrainRenderer& terrain);                           // 地形の高さを各マスに覚え、急な坂・大きな段差を通れなくする(XZだけ)

    // ── 調べる ──
    bool WorldToCell(DirectX::XMFLOAT3 position, int& cellX, int& cellZ) const;     // 格子の外なら false
    DirectX::XMFLOAT3 CellToWorld(int cellX, int cellZ) const;                      // マスの中心(XZなら y は覚えた地面の高さ)
    DirectX::XMFLOAT3 ClampToGrid(DirectX::XMFLOAT3 position) const;               // 格子の外なら、格子の中の一番近い点(縁から半マス内側)にする。平面の外の成分(高さ)はそのまま
    bool IsInside(int cellX, int cellZ) const;                                      // 格子の中のマスか
    bool IsWalkable(int cellX, int cellZ) const;                                    // 通れるか(格子の外は通れない)
    bool IsWalkable(DirectX::XMFLOAT3 position) const;
    // 2点の間を、通れるマスだけでまっすぐ行けるか(平面の上だけで考える。経路の直線化に使う)。壁の角をかすめる線は不可
    bool HasLineOfWalk(DirectX::XMFLOAT3 from, DirectX::XMFLOAT3 to) const;
    // 通れないマスの中にいる時に、近くの通れるマスの中心を探す。格子の外・maxRing マス以内に無ければ false
    bool FindNearestWalkable(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3& result, int maxRing = 6) const;

    int GetCellCountX() const { return cellCountX; }
    int GetCellCountZ() const { return cellCountZ; }
    const NavGridSettings& GetSettings() const { return settings; }
    int GetVersion() const { return version; }   // 障害物や高さが変わる度に増える番号(歩く側が「経路が古くなった」と気付くため)

private:
    // マスごとの障害物の種類(ビット)
    static constexpr uint8_t blockedManual = 1;     // 手で登録
    static constexpr uint8_t blockedPhysics = 2;    // 物理から
    static constexpr uint8_t blockedTerrain = 4;    // 地形から

    NavGridSettings settings;
    int cellCountX = 1;
    int cellCountZ = 1;
    std::vector<uint8_t> blocked;    // マスごとの障害物の種類(0なら通れる)
    std::vector<float> heights;      // マスごとの地面の高さ(XZだけ)
    int version = 0;                 // 障害物や高さが変わる度に増える

    int ToIndex(int cellX, int cellZ) const { return cellZ * cellCountX + cellX; }
    float GetCellHeight(int cellX, int cellZ) const;
    // 平面の座標(u=X、v=XYならY・XZならZ)をマスの座標にする(格子の外でも計算する)
    DirectX::XMFLOAT2 ToCellSpace(float u, float v) const;
    // 平面の矩形 [minU,maxU] x [minV,maxV] に重なるマスの範囲(格子の中に収める)。1マスも重ならなければ false
    bool GetCellRange(float minU, float maxU, float minV, float maxV, int& x0, int& z0, int& x1, int& z1) const;
    // マス (x0,z0)〜(x1,z1) を太らせた範囲に、障害物のタグの物があるか(物理の問い合わせ)
    bool HasPhysicsObstacle(int x0, int z0, int x1, int z1) const;
    // マスの範囲の物理の障害物を調べ直す。変わったマスがあれば true
    bool BakePhysicsRange(int x0, int z0, int x1, int z1);
    // 種類 kind の印を付ける/外す。変わったら true
    bool SetFlag(int index, uint8_t kind, bool on);
};
