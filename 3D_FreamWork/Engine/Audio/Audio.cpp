#include "Audio.h"
#include "../Core/Resources.h"
#include "../Render/Image.h"
#include <x3daudio.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <climits>
#include <algorithm>

using namespace DirectX;

// ─── このファイルの構成 ─────────────────────────────────
// 音は2つに分けて持つ:
//   Wave  = 音のデータ(WAVの中身)。同じ音は1つを共有する(Resources::LoadSound が管理)
//   Voice = 再生口(IXAudio2SourceVoice)。鳴らす度に借りて、鳴り終わったら返して使い回す(プール)
// 音量のグループ(BGM/SE)は、XAudio2のサブミックスボイス(途中でまとめる口)で作る:
//   再生口 → グループのサブミックス → マスタリングボイス → スピーカー
// 位置による聞こえ方(距離で小さく・左右の向き)は X3DAudio で計算して、再生口の出力マトリクスに入れる

namespace {
    // ─── 音のデータ ─────────────────────────────
    struct Wave {
        WAVEFORMATEX format{};
        BYTE* data = nullptr;
        UINT32 size = 0;
        bool valid = false;
    };

    // ─── 再生口(プールで使い回す) ───────────────
    struct Voice {
        IXAudio2SourceVoice* voice = nullptr;
        WAVEFORMATEX format{};          // この再生口を作った時の音のフォーマット(違うフォーマットの音には使い回せない)
        bool inUse = false;             // 借りられている(鳴っている)か
        unsigned int generation = 0;    // 返す度に増やす。古い番号(VoiceHandle)で操作されても何も起きないようにする
        unsigned int waveID = UINT_MAX; // 鳴らしている音のデータ
        bool ownsSoundRef = false;      // 鳴り終わったらResources::ReleaseSoundするか(PlayOneShotで借りた時)
        AudioGroup group = AudioGroup::SE;
        bool spatial = false;           // 位置で聞こえ方を変えるか
        bool matrixApplied = false;     // 位置用の出力マトリクスを入れてあるか(入れてあるとき、位置を使わなくなったら戻す)
        XMFLOAT3 position{ 0.f, 0.f, 0.f };
        float minDistance = Audio::defaultMinDistance;
        float maxDistance = Audio::defaultMaxDistance;
    };

    // ─── IDで鳴らす音(Audio::Load。BGM向け。専用の再生口を1つ持ち続ける) ───────
    struct SoundData {
        IXAudio2SourceVoice* sourceVoice = nullptr;
        unsigned int waveID = UINT_MAX;   // このIDだけが使う音のデータ(Resourcesは通さない)
        bool loop = false;
        bool valid = false;
    };

    constexpr int groupCount = 2;
    constexpr float speedOfSound = 343.f;   // X3DAudioInitializeに渡す値(ドップラー効果は使わないので影響しない)

    IXAudio2* xAudio2 = nullptr;
    IXAudio2MasteringVoice* masterVoice = nullptr;
    IXAudio2SubmixVoice* groupVoices[groupCount] = {};   // AudioGroupごとのサブミックス
    float groupVolumes[groupCount] = { 1.f, 1.f };       // グループの音量(Initの前に決めても覚えておく)
    X3DAUDIO_HANDLE x3dInstance = {};
    UINT32 outputChannels = 2;                           // マスタリングボイスの入力チャンネル数(スピーカーの数)
    bool ready = false;                                  // Initが最後まで成功したか

    std::vector<Wave> waves;
    std::vector<Voice> voices;
    std::vector<SoundData> sounds;
    std::unordered_set<std::wstring> warnedPaths;   // 読み込みに失敗したと知らせたパス(同じパスで何度も出さない)
    bool warnedVoiceLimit = false;                  // 再生口の上限を知らせたか

    // 聞く位置と向き
    struct ListenerPose {
        XMFLOAT3 position{ 0.f, 0.f, 0.f };
        XMFLOAT3 front{ 0.f, 1.f, 0.f };
        XMFLOAT3 up{ 0.f, 0.f, -1.f };
    };
    ListenerPose pendingListener;        // AudioListenerComponentが今フレーム伝えた聞く位置
    bool listenerSet = false;            // 今フレーム伝えられたか
    ListenerPose currentListener;        // 今使っている聞く位置(Updateで決める)

    int GroupIndex(AudioGroup group) { return (group == AudioGroup::BGM) ? 0 : 1; }

    // ─── WAV読み込みヘルパー ─────────────────────
    //
    // ★ "fmt "チャンクの直後に必ず"data"チャンクが来る44バイト固定ヘッダー、という
    //   決め打ちでは読めないファイルがある(例: ffmpeg/Lavfで書き出したWAVは、"fmt "と
    //   "data"の間に"LIST"(メタデータ)チャンクが挟まっていることが多い)。
    //   そのため、チャンクを先頭から1つずつ読み進めて、"fmt "と"data"を名前で探す
    // 失敗したらfalseを返し、理由をerrorに入れる
    bool LoadWAV(const std::wstring& filepath,
        WAVEFORMATEX& fmt, BYTE*& data, UINT32& size, const char*& error) {
        FILE* file = nullptr;
        _wfopen_s(&file, filepath.c_str(), L"rb");
        if (!file) {
            error = "ファイルが見つかりません";
            return false;
        }

        char riff[4], wave[4];
        UINT32 riffSize;
        fread(riff, 1, 4, file);
        fread(&riffSize, sizeof(UINT32), 1, file);
        fread(wave, 1, 4, file);
        if (memcmp(riff, "RIFF", 4) != 0 || memcmp(wave, "WAVE", 4) != 0) {
            error = "WAVファイルではありません";
            fclose(file);
            return false;
        }

        bool foundFmt = false, foundData = false;
        data = nullptr;
        size = 0;

        char chunkId[4];
        UINT32 chunkSize;
        while (fread(chunkId, 1, 4, file) == 4 && fread(&chunkSize, sizeof(UINT32), 1, file) == 1) {
            if (memcmp(chunkId, "fmt ", 4) == 0) {
                UINT16 audioFormat = 0, channels = 0, blockAlign = 0, bitsPerSample = 0;
                UINT32 sampleRate = 0, byteRate = 0;
                fread(&audioFormat, sizeof(UINT16), 1, file);
                fread(&channels, sizeof(UINT16), 1, file);
                fread(&sampleRate, sizeof(UINT32), 1, file);
                fread(&byteRate, sizeof(UINT32), 1, file);
                fread(&blockAlign, sizeof(UINT16), 1, file);
                fread(&bitsPerSample, sizeof(UINT16), 1, file);
                // fmtチャンクが16バイトより大きい(WAVE_FORMAT_EXTENSIBLEなど)場合は、残りを読み飛ばす
                long read = 16;
                if ((long)chunkSize > read) fseek(file, chunkSize - read, SEEK_CUR);

                fmt.wFormatTag = audioFormat;
                fmt.nChannels = channels;
                fmt.nSamplesPerSec = sampleRate;
                fmt.nAvgBytesPerSec = byteRate;
                fmt.nBlockAlign = blockAlign;
                fmt.wBitsPerSample = bitsPerSample;
                fmt.cbSize = 0;
                foundFmt = true;
            }
            else if (memcmp(chunkId, "data", 4) == 0) {
                size = chunkSize;
                data = new BYTE[size];
                fread(data, size, 1, file);
                foundData = true;
                break;  // 音声データは見つかったので、以降のチャンク(メタデータ等)は読まない
            }
            else {
                // 知らないチャンク(LIST/fact/JUNKなど)は読み飛ばす。
                // RIFFのチャンクは2バイト単位に揃えられるため、奇数サイズの時は1バイト分の
                // パディングも一緒に読み飛ばす
                fseek(file, chunkSize + (chunkSize & 1), SEEK_CUR);
            }
        }

        fclose(file);
        if (!foundFmt || !foundData) {
            error = "fmt/dataチャンクが見つかりません";
            delete[] data;
            data = nullptr;
            return false;
        }
        return true;
    }

    // 音のデータを読み込んで登録する(失敗したら、そのパスについて1回だけ出力ウィンドウに知らせる)
    unsigned int LoadWaveData(const std::wstring& filepath) {
        Wave wave;
        const char* error = "";
        if (!LoadWAV(filepath, wave.format, wave.data, wave.size, error)) {
            if (warnedPaths.insert(filepath).second) {
                OutputDebugStringA("★ Audio: 音を読み込めません(");
                OutputDebugStringA(error);
                OutputDebugStringA("): ");
                OutputDebugStringW(filepath.c_str());
                OutputDebugStringA("\n");
            }
            return UINT_MAX;
        }
        wave.valid = true;

        // 解放済みの場所があれば使い回す
        for (size_t i = 0; i < waves.size(); i++) {
            if (!waves[i].valid) { waves[i] = wave; return (unsigned int)i; }
        }
        waves.push_back(wave);
        return (unsigned int)(waves.size() - 1);
    }

    void UnloadWaveData(unsigned int waveID) {
        if (waveID >= waves.size() || !waves[waveID].valid) return;
        delete[] waves[waveID].data;
        waves[waveID] = Wave{};
    }

    // ─── 再生口の出力先(グループのサブミックス) ───────────
    void SetOutputGroup(IXAudio2SourceVoice* voice, AudioGroup group) {
        XAUDIO2_SEND_DESCRIPTOR desc = { 0, groupVoices[GroupIndex(group)] };
        XAUDIO2_VOICE_SENDS sends = { 1, &desc };
        voice->SetOutputVoices(&sends);   // 出力マトリクスも標準(左右そのまま)に戻る
    }

    bool SameFormat(const WAVEFORMATEX& a, const WAVEFORMATEX& b) {
        return a.wFormatTag == b.wFormatTag && a.nChannels == b.nChannels
            && a.nSamplesPerSec == b.nSamplesPerSec && a.wBitsPerSample == b.wBitsPerSample
            && a.nBlockAlign == b.nBlockAlign;
    }

    // ─── 聞く位置 ───────────────────────────────
    XMFLOAT3 Normalized(XMFLOAT3 v, XMFLOAT3 fallback) {
        float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        if (length < 0.0001f) return fallback;
        return { v.x / length, v.y / length, v.z / length };
    }

    // 向きを「長さ1で、frontとupが直角」にそろえる(X3DAudioはそろっていないと正しく計算できない)
    ListenerPose MakeValidPose(XMFLOAT3 position, XMFLOAT3 front, XMFLOAT3 up) {
        ListenerPose pose;
        pose.position = position;
        pose.front = Normalized(front, { 0.f, 0.f, 1.f });

        // upからfront向きの成分を引いて、frontと直角にする
        float d = up.x * pose.front.x + up.y * pose.front.y + up.z * pose.front.z;
        XMFLOAT3 t = { up.x - pose.front.x * d, up.y - pose.front.y * d, up.z - pose.front.z * d };
        if (sqrtf(t.x * t.x + t.y * t.y + t.z * t.z) < 0.0001f) {
            // upがfrontと同じ向きだった時: frontと平行でない軸から作り直す
            XMFLOAT3 axis = (fabsf(pose.front.y) < 0.9f) ? XMFLOAT3{ 0.f, 1.f, 0.f } : XMFLOAT3{ 0.f, 0.f, 1.f };
            float d2 = axis.x * pose.front.x + axis.y * pose.front.y + axis.z * pose.front.z;
            t = { axis.x - pose.front.x * d2, axis.y - pose.front.y * d2, axis.z - pose.front.z * d2 };
        }
        pose.up = Normalized(t, { 0.f, 1.f, 0.f });
        return pose;
    }

    // 今フレームの聞く位置を決める。
    // AudioListenerComponentが伝えていればその位置、無ければ2Dカメラの位置(x,y。zは0)。
    // 2Dのときの向き: 画面の上(+Y)が正面、右(+X)が右。X3DAudioは左手系で「右 = up × front」なので、
    // 画面の手前(カメラ側=-Z)を頭の上にすると右が+Xになる
    void UpdateListener() {
        if (listenerSet) {
            currentListener = MakeValidPose(pendingListener.position, pendingListener.front, pendingListener.up);
        }
        else {
            const XMFLOAT3 cam = Image::GetCamera().position;
            currentListener = MakeValidPose({ cam.x, cam.y, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, -1.f });
        }
        listenerSet = false;
    }

    // ─── 位置による聞こえ方 ─────────────────────
    // リスナーとの位置関係から、距離による音量と左右の定位を X3DAudio で求め、再生口の出力マトリクスに入れる
    void ApplySpatial(Voice& v) {
        if (!v.voice) return;
        const UINT32 srcChannels = v.format.nChannels;
        if (srcChannels == 0 || srcChannels > 8) return;

        // 距離による音量: minDistanceまで1、maxDistanceで0、その間は直線。
        // X3DAudioの曲線は「距離 / CurveDistanceScaler」が0〜1の横軸なので、scalerにmaxDistanceを渡す
        float maxD = (v.maxDistance > 1.f) ? v.maxDistance : 1.f;
        float minD = (std::min)((std::max)(v.minDistance, 0.f), maxD);
        float ratio = (std::min)((std::max)(minD / maxD, 0.001f), 0.999f);
        X3DAUDIO_DISTANCE_CURVE_POINT points[3] = { { 0.f, 1.f }, { ratio, 1.f }, { 1.f, 0.f } };
        X3DAUDIO_DISTANCE_CURVE curve = { points, 3 };

        X3DAUDIO_LISTENER listener = {};
        listener.OrientFront = { currentListener.front.x, currentListener.front.y, currentListener.front.z };
        listener.OrientTop = { currentListener.up.x, currentListener.up.y, currentListener.up.z };
        listener.Position = { currentListener.position.x, currentListener.position.y, currentListener.position.z };

        // ステレオの音は、左右のチャンネルを少し離して置く(モノラルは1点)
        float azimuths[8] = {};
        if (srcChannels == 2) {
            azimuths[0] = X3DAUDIO_PI * 1.75f;   // 左(前から反時計回りに45度 = 360-45度)
            azimuths[1] = X3DAUDIO_PI * 0.25f;   // 右
        }
        else {
            for (UINT32 i = 0; i < srcChannels; i++) azimuths[i] = X3DAUDIO_2PI * i / srcChannels;
        }

        X3DAUDIO_EMITTER emitter = {};
        emitter.OrientFront = { 0.f, 0.f, 1.f };
        emitter.OrientTop = { 0.f, 1.f, 0.f };
        emitter.Position = { v.position.x, v.position.y, v.position.z };
        emitter.ChannelCount = srcChannels;
        emitter.ChannelRadius = 1.f;
        emitter.pChannelAzimuths = azimuths;
        emitter.pVolumeCurve = &curve;
        emitter.CurveDistanceScaler = maxD;
        emitter.DopplerScaler = 1.f;

        std::vector<float> matrix(srcChannels * outputChannels, 0.f);
        X3DAUDIO_DSP_SETTINGS dsp = {};
        dsp.SrcChannelCount = srcChannels;
        dsp.DstChannelCount = outputChannels;
        dsp.pMatrixCoefficients = matrix.data();
        X3DAudioCalculate(x3dInstance, &listener, &emitter, X3DAUDIO_CALCULATE_MATRIX, &dsp);

        v.voice->SetOutputMatrix(groupVoices[GroupIndex(v.group)], srcChannels, outputChannels, matrix.data());
        v.matrixApplied = true;
    }

    // 位置を使うかどうかに合わせて、出力マトリクスを整える
    void RefreshOutput(Voice& v) {
        if (v.spatial) {
            ApplySpatial(v);
        }
        else if (v.matrixApplied) {
            SetOutputGroup(v.voice, v.group);   // 位置を使わなくなったら、標準のマトリクスに戻す
            v.matrixApplied = false;
        }
    }

    // ─── 再生口のプール ─────────────────────────
    // 再生口を返す(止めて、次に借りられるようにする)
    void FreeVoice(int index, bool releaseRef) {
        Voice& v = voices[index];
        if (!v.inUse) return;
        if (v.voice) {
            v.voice->Stop();
            v.voice->FlushSourceBuffers();
        }
        v.inUse = false;
        v.generation++;
        unsigned int waveID = v.waveID;
        bool owns = v.ownsSoundRef;
        v.waveID = UINT_MAX;
        v.ownsSoundRef = false;
        if (owns && releaseRef) Resources::ReleaseSound(waveID);
    }

    // 空いている再生口を探して返す。同じフォーマットのものがあれば使い回し、無ければ作る。上限なら-1
    int AcquireVoice(const WAVEFORMATEX& format, AudioGroup group) {
        int inUseCount = 0;
        int sameFormat = -1;
        int emptySlot = -1;
        int otherFormat = -1;
        for (int i = 0; i < (int)voices.size(); i++) {
            if (voices[i].inUse) { inUseCount++; continue; }
            if (!voices[i].voice) { if (emptySlot < 0) emptySlot = i; }
            else if (SameFormat(voices[i].format, format)) { if (sameFormat < 0) sameFormat = i; }
            else if (otherFormat < 0) otherFormat = i;
        }

        // 使い回せるものがあればそれを使う
        if (sameFormat >= 0) return sameFormat;

        // 上限(maxVoices)まで借りられている
        if (inUseCount >= Audio::maxVoices) {
            if (!warnedVoiceLimit) {
                warnedVoiceLimit = true;
                OutputDebugStringA("★ Audio: 同時に鳴らせる数の上限を超えたので、鳴らしませんでした\n");
            }
            return -1;
        }

        // 新しく作る場所: 空き > 追加(上限まで) > 別フォーマットで空いているものを作り直す
        int slot = emptySlot;
        if (slot < 0 && (int)voices.size() < Audio::maxVoices) {
            voices.emplace_back();
            slot = (int)voices.size() - 1;
        }
        if (slot < 0) slot = otherFormat;
        if (slot < 0) return -1;

        Voice& v = voices[slot];
        if (v.voice) { v.voice->DestroyVoice(); v.voice = nullptr; }

        XAUDIO2_SEND_DESCRIPTOR desc = { 0, groupVoices[GroupIndex(group)] };
        XAUDIO2_VOICE_SENDS sends = { 1, &desc };
        HRESULT hr = xAudio2->CreateSourceVoice(&v.voice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sends);
        if (FAILED(hr)) {
            v.voice = nullptr;
            OutputDebugStringA("★ Audio: 再生口を作れません(CreateSourceVoice 失敗)\n");
            return -1;
        }
        v.format = format;
        return slot;
    }

    Audio::VoiceHandle StartVoiceInternal(unsigned int waveID, const Audio::VoiceParams& params, bool ownsSoundRef) {
        if (!ready || waveID >= waves.size() || !waves[waveID].valid) return {};
        const Wave& wave = waves[waveID];

        int index = AcquireVoice(wave.format, params.group);
        if (index < 0) return {};
        Voice& v = voices[index];

        // 借りた再生口の設定(前に鳴らした時の設定が残っているので、毎回入れ直す)
        SetOutputGroup(v.voice, params.group);
        v.group = params.group;
        v.spatial = params.spatial;
        v.matrixApplied = false;
        v.position = params.position;
        v.minDistance = params.minDistance;
        v.maxDistance = params.maxDistance;
        v.waveID = waveID;
        v.voice->SetVolume((std::max)(params.volume, 0.f));
        if (v.spatial) ApplySpatial(v);   // 鳴らし始めから、位置に合った聞こえ方にする

        XAUDIO2_BUFFER buffer = {};
        buffer.AudioBytes = wave.size;
        buffer.pAudioData = wave.data;
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.LoopCount = params.loop ? XAUDIO2_LOOP_INFINITE : 0;
        if (FAILED(v.voice->SubmitSourceBuffer(&buffer))) {
            OutputDebugStringA("★ Audio: 音を鳴らせません(SubmitSourceBuffer 失敗)\n");
            v.waveID = UINT_MAX;
            return {};
        }
        v.voice->Start();

        v.inUse = true;
        v.ownsSoundRef = ownsSoundRef;
        return { index, v.generation };
    }

    bool IsValidHandle(Audio::VoiceHandle handle) {
        return handle.index >= 0 && handle.index < (int)voices.size()
            && voices[handle.index].inUse && voices[handle.index].generation == handle.generation;
    }

    void PlayOneShotInternal(const std::wstring& path, const Audio::VoiceParams& params) {
        if (!ready) return;

        // 同じパスは Resources が1回だけ読み込む。この再生口が鳴り終わるまで借りておく
        unsigned int waveID = Resources::LoadSound(path);
        if (waveID == UINT_MAX) return;

        Audio::VoiceHandle handle = StartVoiceInternal(waveID, params, true);
        if (handle.index < 0) Resources::ReleaseSound(waveID);   // 鳴らせなかったので借りを返す
    }
}

namespace Audio {

    bool Init() {
        HRESULT hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) {
            OutputDebugStringA("★ XAudio2Create 失敗\n");
            return false;
        }

        // 出力は常にステレオ(2チャンネル)にする。スピーカーが5.1や7.1でも、左右の聞こえ方が同じ計算で決まり、
        // 2Dゲームで「左の敵は左から」が分かりやすい(スピーカーへの割り当てはWindows側でやってくれる)
        hr = xAudio2->CreateMasteringVoice(&masterVoice, 2);
        if (FAILED(hr)) {
            OutputDebugStringA("★ CreateMasteringVoice 失敗\n");
            return false;
        }

        // グループごとのサブミックス(スピーカーと同じチャンネル数・サンプリング周波数で作る)
        XAUDIO2_VOICE_DETAILS details = {};
        masterVoice->GetVoiceDetails(&details);
        outputChannels = details.InputChannels;
        for (int i = 0; i < groupCount; i++) {
            hr = xAudio2->CreateSubmixVoice(&groupVoices[i], details.InputChannels, details.InputSampleRate);
            if (FAILED(hr)) {
                OutputDebugStringA("★ CreateSubmixVoice 失敗\n");
                return false;
            }
            groupVoices[i]->SetVolume(groupVolumes[i]);
        }

        // 位置による聞こえ方(X3DAudio)
        DWORD channelMask = 0;
        masterVoice->GetChannelMask(&channelMask);
        if (FAILED(X3DAudioInitialize(channelMask, speedOfSound, x3dInstance))) {
            OutputDebugStringA("★ X3DAudioInitialize 失敗\n");
            return false;
        }

        listenerSet = false;
        UpdateListener();
        ready = true;
        return true;
    }

    void Uninit() {
        // 解放の順番: 再生口 → 音のデータ → サブミックス → マスタリングボイス → XAudio2
        // (サブミックスやマスタリングボイスは、そこへ送っている再生口を先に壊してからでないと壊せない)
        ready = false;
        ReleaseAll();
        for (Voice& v : voices) {
            if (v.voice) { v.voice->Stop(); v.voice->DestroyVoice(); v.voice = nullptr; }
        }
        voices.clear();
        for (unsigned int i = 0; i < (unsigned int)waves.size(); i++) UnloadWaveData(i);
        waves.clear();
        for (int i = 0; i < groupCount; i++) {
            if (groupVoices[i]) { groupVoices[i]->DestroyVoice(); groupVoices[i] = nullptr; }
        }
        if (masterVoice) { masterVoice->DestroyVoice(); masterVoice = nullptr; }
        if (xAudio2) { xAudio2->Release();          xAudio2 = nullptr; }
        warnedPaths.clear();
        warnedVoiceLimit = false;
    }


    unsigned int Load(const std::wstring& filepath, bool loop, AudioGroup group) {
        if (!xAudio2 || !groupVoices[GroupIndex(group)]) return UINT_MAX;

        unsigned int waveID = LoadWaveData(filepath);
        if (waveID == UINT_MAX) return UINT_MAX;

        XAUDIO2_SEND_DESCRIPTOR desc = { 0, groupVoices[GroupIndex(group)] };
        XAUDIO2_VOICE_SENDS sends = { 1, &desc };
        IXAudio2SourceVoice* sourceVoice = nullptr;
        HRESULT hr = xAudio2->CreateSourceVoice(&sourceVoice, &waves[waveID].format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sends);
        if (FAILED(hr)) {
            UnloadWaveData(waveID);
            OutputDebugStringA("★ CreateSourceVoice 失敗\n");
            return UINT_MAX;
        }

        SoundData sound;
        sound.sourceVoice = sourceVoice;
        sound.waveID = waveID;
        sound.loop = loop;
        sound.valid = true;

        sounds.push_back(sound);
        return (unsigned int)(sounds.size() - 1);
    }

    // 再生
    void Play(unsigned int id, float volume) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        auto& s = sounds[id];
        const Wave& wave = waves[s.waveID];

        // 再生前にバッファをリセット
        s.sourceVoice->Stop();
        s.sourceVoice->FlushSourceBuffers();

        XAUDIO2_BUFFER buffer = {};
        buffer.AudioBytes = wave.size;
        buffer.pAudioData = wave.data;
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.LoopCount = s.loop ? XAUDIO2_LOOP_INFINITE : 0;

        s.sourceVoice->SubmitSourceBuffer(&buffer);
        s.sourceVoice->SetVolume(volume);
        s.sourceVoice->Start();
    }

    // 停止
    void Stop(unsigned int id) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].sourceVoice->Stop();
        sounds[id].sourceVoice->FlushSourceBuffers();
    }

    // 一時停止
    void Pause(unsigned int id) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].sourceVoice->Stop();
    }

    // 再開
    void Resume(unsigned int id) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].sourceVoice->Start();
    }

    // 音量調整
    void SetVolume(unsigned int id, float volume) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].sourceVoice->SetVolume(volume);
    }

    // ループ設定
    void SetLoop(unsigned int id, bool loop) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        sounds[id].loop = loop;
    }

    // 解放
    void Release(unsigned int id) {
        if (id >= sounds.size() || !sounds[id].valid) return;
        auto& s = sounds[id];
        s.sourceVoice->Stop();
        s.sourceVoice->DestroyVoice();   // 再生口を壊してから音のデータを消す
        UnloadWaveData(s.waveID);
        s.sourceVoice = nullptr;
        s.waveID = UINT_MAX;
        s.valid = false;
    }

    void ReleaseAll() {
        for (int i = 0; i < (int)sounds.size(); i++) {
            Release(i);
        }
        sounds.clear();
    }

    // ─── グループの音量 ───────────────────────────
    void SetGroupVolume(AudioGroup group, float volume) {
        volume = (std::min)((std::max)(volume, 0.f), 1.f);
        int i = GroupIndex(group);
        groupVolumes[i] = volume;
        if (groupVoices[i]) groupVoices[i]->SetVolume(volume);
    }

    float GetGroupVolume(AudioGroup group) {
        return groupVolumes[GroupIndex(group)];
    }

    // ─── 鳴らしっぱなしの効果音 ─────────────────────
    void PlayOneShot(const std::wstring& path, float volume) {
        VoiceParams params;
        params.volume = volume;
        PlayOneShotInternal(path, params);
    }

    void PlayOneShotAt(const std::wstring& path, XMFLOAT3 position, float volume) {
        VoiceParams params;
        params.volume = volume;
        params.spatial = true;
        params.position = position;
        PlayOneShotInternal(path, params);
    }

    // ─── Engine内部用 ───────────────────────────
    void Update() {
        if (!ready) return;

        UpdateListener();

        for (int i = 0; i < (int)voices.size(); i++) {
            Voice& v = voices[i];
            if (!v.inUse) continue;

            // 鳴り終わった(キューのバッファが無くなった)ら、再生口を返す
            XAUDIO2_VOICE_STATE state = {};
            v.voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            if (state.BuffersQueued == 0) {
                FreeVoice(i, true);
                continue;
            }

            // 位置で聞こえ方が変わる音は、リスナーや音の位置が動いた分を毎フレーム反映する
            if (v.spatial) ApplySpatial(v);
        }
    }

    unsigned int LoadWave(const std::wstring& filepath) {
        return LoadWaveData(filepath);
    }

    void ReleaseWave(unsigned int waveID) {
        // この音で鳴っている再生口があれば、データを消す前に止めて返す(Resources側の借りは数えなおすので返さない)
        for (int i = 0; i < (int)voices.size(); i++) {
            if (voices[i].inUse && voices[i].waveID == waveID) FreeVoice(i, false);
        }
        UnloadWaveData(waveID);
    }

    void SetListener(XMFLOAT3 position, XMFLOAT3 front, XMFLOAT3 up) {
        pendingListener.position = position;
        pendingListener.front = front;
        pendingListener.up = up;
        listenerSet = true;
    }

    VoiceHandle StartVoice(unsigned int waveID, const VoiceParams& params) {
        return StartVoiceInternal(waveID, params, false);
    }

    void UpdateVoice(VoiceHandle handle, const VoiceParams& params) {
        if (!IsValidHandle(handle)) return;
        Voice& v = voices[handle.index];
        v.voice->SetVolume((std::max)(params.volume, 0.f));
        v.spatial = params.spatial;
        v.position = params.position;
        v.minDistance = params.minDistance;
        v.maxDistance = params.maxDistance;
        RefreshOutput(v);
    }

    void StopVoice(VoiceHandle handle) {
        if (!IsValidHandle(handle)) return;
        FreeVoice(handle.index, true);
    }

    bool IsVoicePlaying(VoiceHandle handle) {
        return IsValidHandle(handle);
    }
}
