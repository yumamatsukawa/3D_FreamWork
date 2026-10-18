#include "SpriteTransition.h"
#include "../UI/Text.h"
#include "../Render/Graphics.h"
#include <windows.h>
#include <wincodec.h>
#include <unordered_map>
#include <cstdio>
#include <cmath>
#pragma comment(lib, "windowscodecs.lib")

namespace {
    // 読み込んだ画像(ファイル名 → Direct2Dのビットマップ)。読み込めなかった画像も nullptr で覚えて、毎フレーム読み直さない
    std::unordered_map<std::wstring, ID2D1Bitmap*> bitmaps;
    ID2D1RenderTarget* bitmapTarget = nullptr;   // ビットマップを作った描画先(違う描画先では使えないため覚えておく)
    IWICImagingFactory* wicFactory = nullptr;    // 画像ファイルを読むためのWIC(最初に要った時に作る)

    void ReleaseBitmaps() {
        for (auto& pair : bitmaps) {
            if (pair.second) pair.second->Release();
        }
        bitmaps.clear();
        bitmapTarget = nullptr;
    }

    // 画像ファイルを読んで、Direct2Dのビットマップを作る(D3Dのテクスチャとは別物)。失敗したら nullptr
    ID2D1Bitmap* LoadSheetBitmap(ID2D1RenderTarget* target, const std::wstring& path) {
        if (!wicFactory) {
            CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
            if (!wicFactory) {
                OutputDebugStringA("★ SpriteTransition: WICの準備に失敗\n");
                return nullptr;
            }
        }

        // ファイルを読み、Direct2Dで描ける形(BGRA・透明度をかけ済み)に変換してからビットマップにする
        IWICBitmapDecoder* decoder = nullptr;
        IWICBitmapFrameDecode* frame = nullptr;
        IWICFormatConverter* converter = nullptr;
        ID2D1Bitmap* bitmap = nullptr;
        HRESULT hr = wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
        if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
        if (SUCCEEDED(hr)) hr = wicFactory->CreateFormatConverter(&converter);
        if (SUCCEEDED(hr)) hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);
        if (SUCCEEDED(hr)) hr = target->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap);

        // 途中で使った物は、成功しても失敗しても解放する(ビットマップは自分で中身を持っているので要らない)
        if (converter) converter->Release();
        if (frame) frame->Release();
        if (decoder) decoder->Release();

        if (FAILED(hr) || !bitmap) {
            OutputDebugStringW((L"★ SpriteTransition: 画像の読み込みに失敗: " + path + L"\n").c_str());
            return nullptr;
        }
        return bitmap;
    }

    // 画像を返す(まだなら読み込む。同じファイルは使い回す)
    ID2D1Bitmap* GetBitmap(ID2D1RenderTarget* target, const std::wstring& path) {
        // 描画先が変わっていたら、前の描画先で作ったビットマップは使えないので全部作り直す
        if (target != bitmapTarget) {
            ReleaseBitmaps();
            bitmapTarget = target;
        }
        auto found = bitmaps.find(path);
        if (found != bitmaps.end()) return found->second;

        ID2D1Bitmap* bitmap = LoadSheetBitmap(target, path);
        bitmaps[path] = bitmap;
        return bitmap;
    }
}

SpriteTransition::SpriteTransition(const std::wstring& path, int columns, int rows, float frameDuration)
    : coverSheet(MakeSheet(path, columns, rows, frameDuration)) {
}

SpriteTransition& SpriteTransition::SetOpenSheet(const std::wstring& path, int columns, int rows, float frameDuration) {
    openSheet = MakeSheet(path, columns, rows, frameDuration);
    hasOpenSheet = true;
    return *this;
}

SpriteTransition& SpriteTransition::SetSwitchFrame(int frame) {
    // 0〜(コマ数-2)に収める。最後のコマで切り替えると開く段階が0コマになり、新しいシーンがいきなり見えてしまうため
    int maxFrame = coverSheet.GetFrameCount() - 2;
    if (maxFrame < 0) maxFrame = 0;   // 1コマだけの画像(開く段階は0コマになる)
    int clamped = frame;
    if (clamped < 0) clamped = 0;
    if (clamped > maxFrame) clamped = maxFrame;
    if (clamped != frame) {
        char message[160];
        sprintf_s(message, "★ SpriteTransition::SetSwitchFrame: コマ%d は範囲外なので、コマ%d にしました(0から%dまで)\n", frame, clamped, maxFrame);
        OutputDebugStringA(message);
    }

    switchFrame = clamped;
    hasSwitchFrame = true;
    return *this;
}

void SpriteTransition::UnloadAll() {
    ReleaseBitmaps();
    if (wicFactory) { wicFactory->Release(); wicFactory = nullptr; }
}

float SpriteTransition::GetCoverDuration() const {
    // 切り替えるコマを決めた時: コマ0〜switchFrame の分
    if (hasSwitchFrame) return coverSheet.frameDuration * (float)GetCoverFrameCount();
    return coverSheet.GetDuration();
}

float SpriteTransition::GetOpenDuration() const {
    // 切り替えるコマを決めた時: コマswitchFrame+1〜最後のコマ の分
    if (hasSwitchFrame) return coverSheet.frameDuration * (float)GetOpenFrameCount();
    return GetOpenSheet().GetDuration();
}

void SpriteTransition::OnBegin() {
    // SetSwitchFrame と SetOpenSheet を両方使った時は、SetSwitchFrame を優先する(OnBeginは1回の切り替えで1回だけ呼ばれる)
    if (hasSwitchFrame && hasOpenSheet) {
        OutputDebugStringA("★ SpriteTransition: SetSwitchFrame と SetOpenSheet が両方指定されたので、SetOpenSheet の画像は使いません\n");
    }

    // 描き始める前に読み込んでおく(覆い始めや、シーンを切り替えた直後に読み込みで止まらないように)
    ID2D1RenderTarget* target = Text::GetRenderTarget();
    if (!target) return;
    GetBitmap(target, coverSheet.path);
    if (hasOpenSheet && !hasSwitchFrame) GetBitmap(target, openSheet.path);
}

void SpriteTransition::DrawCover(ID2D1RenderTarget* target, float progress) {
    // 切り替えるコマを決めた時: コマ0 → switchFrame
    if (hasSwitchFrame) {
        DrawFrame(target, coverSheet, GetFrameIndex(progress, GetCoverFrameCount()));
        return;
    }

    // 順再生: コマ0 → 最後のコマ
    DrawFrame(target, coverSheet, GetFrameIndex(progress, coverSheet.GetFrameCount()));
}

void SpriteTransition::DrawOpen(ID2D1RenderTarget* target, float progress) {
    // 切り替えるコマを決めた時: 順再生の続き(コマswitchFrame+1 → 最後のコマ)
    // progress=0(シーンを切り替えた瞬間と、その次の読み込みで長くなったフレーム)は、まだ全部覆っている switchFrame を出す。
    // そのため切り上げにして、0より少しでも進んだら switchFrame+1 にする(各コマは1コマの時間ずつ出る)
    if (hasSwitchFrame) {
        int openFrameCount = GetOpenFrameCount();
        int index = switchFrame + (int)ceilf(progress * (float)openFrameCount);
        if (index > switchFrame + openFrameCount) index = switchFrame + openFrameCount;
        DrawFrame(target, coverSheet, index);
        return;
    }

    // 逆再生: 最後のコマ(全部覆っている) → コマ0
    const Sheet& sheet = GetOpenSheet();
    int frameCount = sheet.GetFrameCount();
    DrawFrame(target, sheet, frameCount - 1 - GetFrameIndex(progress, frameCount));
}

int SpriteTransition::GetFrameIndex(float progress, int frameCount) {
    int index = (int)(progress * (float)frameCount);
    if (index < 0) index = 0;
    if (index > frameCount - 1) index = frameCount - 1;   // progress=1(覆い終わった時)は最後のコマのまま
    return index;
}

SpriteTransition::Sheet SpriteTransition::MakeSheet(const std::wstring& path, int columns, int rows, float frameDuration) {
    Sheet sheet;
    sheet.path = path;
    sheet.columns = (columns < 1) ? 1 : columns;
    sheet.rows = (rows < 1) ? 1 : rows;
    sheet.frameDuration = (frameDuration > 0.f) ? frameDuration : 0.f;   // 0以下なら一瞬で終わる
    return sheet;
}

void SpriteTransition::DrawFrame(ID2D1RenderTarget* target, const Sheet& sheet, int index) {
    ID2D1Bitmap* bitmap = GetBitmap(target, sheet.path);
    if (!bitmap) return;   // 読み込めなかった時は何も描かない

    // 画像の中の、このコマの四角(左上から右へ、行の終わりで次の行へ)
    D2D1_SIZE_F size = bitmap->GetSize();
    D2D1_SIZE_U pixelSize = bitmap->GetPixelSize();
    float frameWidth = size.width / (float)sheet.columns;
    float frameHeight = size.height / (float)sheet.rows;
    float left = frameWidth * (float)(index % sheet.columns);
    float top = frameHeight * (float)(index / sheet.columns);

    // 引き伸ばす時に隣のコマの色がにじまないように、半ピクセル内側だけを使う
    float insetX = (pixelSize.width > 0) ? 0.5f * size.width / (float)pixelSize.width : 0.f;
    float insetY = (pixelSize.height > 0) ? 0.5f * size.height / (float)pixelSize.height : 0.f;
    D2D1_RECT_F sourceRect = D2D1::RectF(left + insetX, top + insetY, left + frameWidth - insetX, top + frameHeight - insetY);

    // 画面全体に引き伸ばして描く
    D2D1_RECT_F screenRect = D2D1::RectF(0.f, 0.f, Graphics::width, Graphics::height);
    target->DrawBitmap(bitmap, screenRect, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &sourceRect);
}
