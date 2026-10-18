#include "Terrain.h"
#include "../Data/Csv.h"
#include <windows.h>
#include <wincodec.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#pragma comment(lib, "windowscodecs.lib")

namespace {
    const int maxCells = 256;           // 分割数の上限(頂点は257x257=66049個。32ビットのインデックス)
    const int maxSampleSize = 8192;     // 読み込む画像・表の大きさの上限(メモリを使いすぎないため)

    // value を lo〜hi に収める(std::clamp は C++17 なので、このプロジェクトでは使えない)
    template <class T>
    T Clamp(T value, T lo, T hi) { return (value < lo) ? lo : ((value > hi) ? hi : value); }

    // 0〜1の数値の格子(画像と同じ並び: 最初の行が上)から、(u, v)の場所の値を双線形補間で取り出す。u・vは0〜1(v=0が上)
    float SampleGrid(const std::vector<float>& values, int w, int h, float u, float v) {
        float px = Clamp(u, 0.f, 1.f) * (float)(w - 1);
        float py = Clamp(v, 0.f, 1.f) * (float)(h - 1);
        int x0 = (std::min)((int)floorf(px), w - 1);
        int y0 = (std::min)((int)floorf(py), h - 1);
        int x1 = (std::min)(x0 + 1, w - 1);
        int y1 = (std::min)(y0 + 1, h - 1);
        float fx = px - (float)x0, fy = py - (float)y0;
        float top = values[(size_t)y0 * w + x0] * (1.f - fx) + values[(size_t)y0 * w + x1] * fx;
        float bottom = values[(size_t)y1 * w + x0] * (1.f - fx) + values[(size_t)y1 * w + x1] * fx;
        return top * (1.f - fy) + bottom * fy;
    }

    // 画像ファイルを読んで、0〜1の数値(画像と同じ並び)にする。失敗したらfalse
    bool ReadImageValues(const std::wstring& path, std::vector<float>& values, int& width, int& height) {
        // WICはCOMが必要。すでに初期化されていれば(S_FALSE)そのまま使い、別の方式で初期化済み(RPC_E_CHANGED_MODE)でも使える
        HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bool comInitialized = SUCCEEDED(comResult);

        IWICImagingFactory* factory = nullptr;
        IWICBitmapDecoder* decoder = nullptr;
        IWICBitmapFrameDecode* frame = nullptr;
        IWICFormatConverter* converter = nullptr;
        bool ok = false;

        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
        if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);

        UINT w = 0, h = 0;
        WICPixelFormatGUID format = {};
        if (SUCCEEDED(hr)) hr = frame->GetSize(&w, &h);
        if (SUCCEEDED(hr)) hr = frame->GetPixelFormat(&format);
        if (SUCCEEDED(hr) && (w < 2 || h < 2 || w > (UINT)maxSampleSize || h > (UINT)maxSampleSize)) {
            OutputDebugStringW((L"★ Terrain: 画像の大きさが範囲外です(2〜8192): " + path + L"\n").c_str());
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr)) {
            values.assign((size_t)w * h, 0.f);
            if (format == GUID_WICPixelFormat16bppGray) {
                // 16ビットのグレースケール: 65536段階をそのまま使う
                std::vector<uint16_t> pixels((size_t)w * h);
                hr = frame->CopyPixels(nullptr, w * 2, (UINT)(pixels.size() * 2), reinterpret_cast<BYTE*>(pixels.data()));
                if (SUCCEEDED(hr)) {
                    for (size_t i = 0; i < pixels.size(); i++) values[i] = (float)pixels[i] / 65535.f;
                    ok = true;
                }
            }
            else {
                // それ以外(8ビットのグレースケール・カラー・パレット): RGBAに変換して、赤(R)の256段階を使う
                hr = factory->CreateFormatConverter(&converter);
                if (SUCCEEDED(hr)) hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
                std::vector<BYTE> pixels((size_t)w * h * 4);
                if (SUCCEEDED(hr)) hr = converter->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data());
                if (SUCCEEDED(hr)) {
                    for (size_t i = 0; i < values.size(); i++) values[i] = (float)pixels[i * 4] / 255.f;
                    ok = true;
                }
            }
        }
        width = (int)w;
        height = (int)h;

        if (!ok) OutputDebugStringW((L"★ Terrain: 高さの画像を読み込めません: " + path + L"\n").c_str());
        if (converter) converter->Release();
        if (frame) frame->Release();
        if (decoder) decoder->Release();
        if (factory) factory->Release();
        if (comInitialized) CoUninitialize();
        return ok;
    }
}

void Terrain::SanitizeSettings(TerrainSettings& s) {
    if (!(s.width > 0.f)) { OutputDebugStringA("★ Terrain: width は0より大きくしてください(10000にしました)\n"); s.width = 10000.f; }
    if (!(s.depth > 0.f)) { OutputDebugStringA("★ Terrain: depth は0より大きくしてください(10000にしました)\n"); s.depth = 10000.f; }
    int clampedX = Clamp(s.cellsX, 1, maxCells);
    int clampedZ = Clamp(s.cellsZ, 1, maxCells);
    if (clampedX != s.cellsX || clampedZ != s.cellsZ) {
        char message[160];
        sprintf_s(message, "★ Terrain: 分割数は1から%dまでです(cellsX=%d, cellsZ=%d を %d, %d にしました)\n", maxCells, s.cellsX, s.cellsZ, clampedX, clampedZ);
        OutputDebugStringA(message);
    }
    s.cellsX = clampedX;
    s.cellsZ = clampedZ;
}

std::shared_ptr<Terrain> Terrain::Build(const std::vector<float>& values, int sampleWidth, int sampleHeight, TerrainSettings settings) {
    SanitizeSettings(settings);
    std::shared_ptr<Terrain> terrain(new Terrain());
    terrain->settings = settings;

    // 頂点 (x, z) の高さ: 画像の左が X=0、画像の上が Z=最大
    int vertexCountX = settings.cellsX + 1, vertexCountZ = settings.cellsZ + 1;
    terrain->heights.resize((size_t)vertexCountX * vertexCountZ);
    for (int x = 0; x < vertexCountX; x++) {
        for (int z = 0; z < vertexCountZ; z++) {
            float u = (float)x / (float)settings.cellsX;
            float v = 1.f - (float)z / (float)settings.cellsZ;
            float value = Clamp(SampleGrid(values, sampleWidth, sampleHeight, u, v), 0.f, 1.f);
            terrain->heights[(size_t)x * vertexCountZ + z] = value * settings.maxHeight;
        }
    }
    return terrain;
}

std::shared_ptr<Terrain> Terrain::LoadFromImage(const std::wstring& path, const TerrainSettings& settings) {
    std::vector<float> values;
    int w = 0, h = 0;
    if (!ReadImageValues(path, values, w, h)) return nullptr;
    return Build(values, w, h, settings);
}

std::shared_ptr<Terrain> Terrain::LoadFromCsv(const std::wstring& path, const TerrainSettings& settings) {
    Csv table = Csv::Load(path);
    if (!table.IsValid()) return nullptr;   // 読めなかった理由は Csv が出力ウィンドウに出している
    int w = table.GetColumnCount();
    int h = table.GetRowCount();
    if (w < 2 || h < 2 || w > maxSampleSize || h > maxSampleSize || !table.HasColumn("x0")) {
        OutputDebugStringW((L"★ Terrain: CSVは1行目が x0,x1,x2,... で、2列2行以上にしてください: " + path + L"\n").c_str());
        return nullptr;
    }
    std::vector<float> values((size_t)w * h);
    for (int row = 0; row < h; row++) {
        for (int column = 0; column < w; column++) {
            values[(size_t)row * w + column] = Clamp(table.GetFloat(row, "x" + std::to_string(column), 0.f), 0.f, 1.f);
        }
    }
    return Build(values, w, h, settings);
}

std::shared_ptr<Terrain> Terrain::CreateFromSamples(const std::vector<float>& values, int sampleWidth, int sampleHeight,
    const TerrainSettings& settings) {
    if (sampleWidth < 2 || sampleHeight < 2 || values.size() < (size_t)sampleWidth * sampleHeight) return nullptr;
    return Build(values, sampleWidth, sampleHeight, settings);
}

float Terrain::GetHeightAtVertex(int x, int z) const {
    x = Clamp(x, 0, settings.cellsX);
    z = Clamp(z, 0, settings.cellsZ);
    return heights[(size_t)x * GetVertexCountZ() + z];
}

Terrain::CellPos Terrain::Locate(float localX, float localZ) const {
    float gx = Clamp(localX, 0.f, settings.width) / GetCellSizeX();
    float gz = Clamp(localZ, 0.f, settings.depth) / GetCellSizeZ();
    CellPos pos;
    pos.cx = (std::min)((int)floorf(gx), settings.cellsX - 1);
    pos.cz = (std::min)((int)floorf(gz), settings.cellsZ - 1);
    pos.fx = Clamp(gx - (float)pos.cx, 0.f, 1.f);
    pos.fz = Clamp(gz - (float)pos.cz, 0.f, 1.f);
    return pos;
}

float Terrain::GetHeightAt(float localX, float localZ) const {
    CellPos p = Locate(localX, localZ);
    float a = GetHeightAtVertex(p.cx, p.cz);           // (x, z)
    float b = GetHeightAtVertex(p.cx + 1, p.cz);       // (x+1, z)
    float c = GetHeightAtVertex(p.cx + 1, p.cz + 1);   // (x+1, z+1)
    float d = GetHeightAtVertex(p.cx, p.cz + 1);       // (x, z+1)
    // 対角線は a-c。fx >= fz の側が三角形 a,b,c、反対側が三角形 a,d,c
    if (p.fx >= p.fz) return a + (b - a) * p.fx + (c - b) * p.fz;
    return a + (d - a) * p.fz + (c - d) * p.fx;
}

XMFLOAT3 Terrain::GetNormalAt(float localX, float localZ) const {
    CellPos p = Locate(localX, localZ);
    float a = GetHeightAtVertex(p.cx, p.cz);
    float b = GetHeightAtVertex(p.cx + 1, p.cz);
    float c = GetHeightAtVertex(p.cx + 1, p.cz + 1);
    float d = GetHeightAtVertex(p.cx, p.cz + 1);
    // 三角形の面は y = a + slopeX * x + slopeZ * z。向きは (-slopeX, 1, -slopeZ)
    float slopeX, slopeZ;
    if (p.fx >= p.fz) { slopeX = (b - a) / GetCellSizeX(); slopeZ = (c - b) / GetCellSizeZ(); }
    else { slopeX = (c - d) / GetCellSizeX(); slopeZ = (d - a) / GetCellSizeZ(); }
    float length = sqrtf(slopeX * slopeX + 1.f + slopeZ * slopeZ);
    return { -slopeX / length, 1.f / length, -slopeZ / length };
}

bool Terrain::Contains(float localX, float localZ) const {
    return localX >= 0.f && localX <= settings.width && localZ >= 0.f && localZ <= settings.depth;
}

void Terrain::BuildMesh(std::vector<MeshVertex>& vertices, std::vector<UINT>& indices) const {
    int vertexCountX = GetVertexCountX(), vertexCountZ = GetVertexCountZ();
    float cellX = GetCellSizeX(), cellZ = GetCellSizeZ();

    vertices.clear();
    vertices.reserve((size_t)vertexCountX * vertexCountZ);
    for (int x = 0; x < vertexCountX; x++) {
        for (int z = 0; z < vertexCountZ; z++) {
            // 法線: 両隣の頂点の高さの差(中心差分。端は片側だけ)から、傾きを求める
            int x0 = (std::max)(x - 1, 0), x1 = (std::min)(x + 1, settings.cellsX);
            int z0 = (std::max)(z - 1, 0), z1 = (std::min)(z + 1, settings.cellsZ);
            float slopeX = (GetHeightAtVertex(x1, z) - GetHeightAtVertex(x0, z)) / ((float)(x1 - x0) * cellX);
            float slopeZ = (GetHeightAtVertex(x, z1) - GetHeightAtVertex(x, z0)) / ((float)(z1 - z0) * cellZ);
            float length = sqrtf(slopeX * slopeX + 1.f + slopeZ * slopeZ);

            MeshVertex vertex;
            vertex.Position = { (float)x * cellX, GetHeightAtVertex(x, z), (float)z * cellZ };
            vertex.Normal = { -slopeX / length, 1.f / length, -slopeZ / length };
            vertex.Color = { 1.f, 1.f, 1.f, 1.f };
            vertex.TexCoord = { vertex.Position.x / settings.width * settings.uvRepeat, vertex.Position.z / settings.depth * settings.uvRepeat };
            vertices.push_back(vertex);
        }
    }

    // 1マス = 2枚の三角形。対角線は (x, z) → (x+1, z+1)。上から見て時計回りに並べる
    indices.clear();
    indices.reserve((size_t)settings.cellsX * settings.cellsZ * 6);
    for (int x = 0; x < settings.cellsX; x++) {
        for (int z = 0; z < settings.cellsZ; z++) {
            UINT a = (UINT)(x * vertexCountZ + z);                // (x, z)
            UINT b = (UINT)((x + 1) * vertexCountZ + z);          // (x+1, z)
            UINT c = (UINT)((x + 1) * vertexCountZ + z + 1);      // (x+1, z+1)
            UINT d = (UINT)(x * vertexCountZ + z + 1);            // (x, z+1)
            indices.push_back(a); indices.push_back(d); indices.push_back(c);
            indices.push_back(a); indices.push_back(c); indices.push_back(b);
        }
    }
}
