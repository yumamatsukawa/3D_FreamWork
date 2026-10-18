#pragma once
#include <xaudio2.h>
#include <DirectXMath.h>
#include <string>
#pragma comment(lib, "xaudio2.lib")   // X3DAudio(位置による聞こえ方)の関数もこのライブラリに入っている

// 音のグループ。グループごとに音量を変えられる(BGMだけ小さくする、など)
// IDで鳴らす Audio::Load の音は BGM、PlayOneShot / AudioSourceComponent は既定で SE
enum class AudioGroup { BGM, SE };

namespace Audio {
    bool Init();
    void Uninit();

    /* =======================================================
    *   Audioのロード(IDで鳴らす方法。長く鳴らすBGM向け)
    * ".wavのファイルのみがロードできる"
    * BGMなどループする  ファイルはloopをtrueに
    * SE などループしないファイルはloopをfalseに
    * groupは音量を変えるグループ(既定はBGM。Audio::SetGroupVolumeの影響を受ける)
    * ※効果音を重ねて鳴らしたい時は、下の PlayOneShot / AudioSourceComponent を使う
    ======================================================== */
    unsigned int Load(const std::wstring& filepath,
        bool loop = false, AudioGroup group = AudioGroup::BGM);
    // 再生
    void Play(unsigned int id, float volume = 1.f);
    // 停止
    void Stop(unsigned int id);
    // 一時停止
    void Pause(unsigned int id);
    // 再開
    void Resume(unsigned int id);
    // 音量調整
    void SetVolume(unsigned int id, float volume);
    // ループするかどうか
    void SetLoop(unsigned int id, bool loop);
    // 削除
    void Release(unsigned int id);
    // すべて削除
    void ReleaseAll();

    /* =======================================================
    *   グループごとの音量(0〜1。範囲外は0〜1に収める)
    * 保存したい時は SaveData に書く(例: "MusicVolume" / "SoundEffectVolume")
    ======================================================== */
    void  SetGroupVolume(AudioGroup group, float volume);
    float GetGroupVolume(AudioGroup group);

    /* =======================================================
    *   鳴らしっぱなしの効果音(重ねて鳴らせる。鳴り終われば自動で片付く)
    * 同じファイルは Resources で1回だけ読み込まれる(2回目以降は読み直さない)。
    * 同時に鳴らせる数は maxVoices(32)個まで。超えた分は鳴らさない(出力ウィンドウに1回知らせる)
    * 失敗(ファイルが読めない・音声デバイスが無い)した時は何もしない
    *
    * 位置で鳴らす PlayOneShotAt は、聞く位置(AudioListenerComponent。無ければ2Dカメラの位置)からの
    * 距離で小さくなり、左右の位置で聞こえる向きが変わる。defaultMinDistance までは最大音量、
    * defaultMaxDistance で聞こえなくなる(位置の単位はワールド座標)
    ======================================================== */
    void PlayOneShot(const std::wstring& path, float volume = 1.f);                                   // 位置に関係なく鳴らす(UIの音など)
    void PlayOneShotAt(const std::wstring& path, DirectX::XMFLOAT3 position, float volume = 1.f);     // その場所で鳴らす

    constexpr int   maxVoices = 32;              // 同時に鳴らせる数の上限(再生口の数)
    constexpr float defaultMinDistance = 300.f;  // この距離までは最大音量
    constexpr float defaultMaxDistance = 2500.f; // この距離で聞こえなくなる(その間は直線的に小さくなる)

    // ─── Engine内部用(GameEngine・Resources・AudioListenerComponent・AudioSourceComponentだけが呼ぶ。ゲーム側からは呼ばない) ─────────────

    void Update();   // 毎フレーム1回(シーンのUpdateの後)。鳴り終わった再生口を返し、位置で鳴る音の聞こえ方を更新する

    // 音のデータ(波形)を読み込む。Resources::LoadSound が呼ぶ。失敗したらUINT_MAX
    unsigned int LoadWave(const std::wstring& filepath);
    // 音のデータを解放する。このデータを使って鳴っている再生口があれば、先に止めて返す
    void ReleaseWave(unsigned int waveID);

    // 聞く位置と向き(AudioListenerComponentが毎フレーム伝える。伝えなかったフレームは2Dカメラの位置で聞く)
    // front=正面、up=頭の上の向き(どちらも長さ1でなくてよい)
    void SetListener(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 front, DirectX::XMFLOAT3 up);

    // 再生口の借り番号。鳴り終わったり返したりすると無効になる(古い番号で操作しても何も起きない)
    struct VoiceHandle {
        int index = -1;
        unsigned int generation = 0;
    };

    // 鳴らす時の設定
    struct VoiceParams {
        float volume = 1.f;                            // 音量(0〜1)
        bool loop = false;                             // 繰り返すか
        bool spatial = false;                          // 位置で聞こえ方を変えるか
        DirectX::XMFLOAT3 position{ 0.f, 0.f, 0.f };   // 音の位置(spatialの時)
        float minDistance = defaultMinDistance;        // この距離までは最大音量
        float maxDistance = defaultMaxDistance;        // この距離で聞こえなくなる
        AudioGroup group = AudioGroup::SE;             // 音量を変えるグループ
    };

    // 再生口を借りて鳴らす。借りられなかったら無効な番号(index=-1)を返す。波形データは呼ぶ側が(Resourcesで)持っておく
    VoiceHandle StartVoice(unsigned int waveID, const VoiceParams& params);
    // 鳴っている間の音量・位置・距離を変える(group・loopは変わらない)
    void UpdateVoice(VoiceHandle handle, const VoiceParams& params);
    // 止めて、再生口を返す
    void StopVoice(VoiceHandle handle);
    // まだ鳴っているか(鳴り終わって自動で返された後はfalse)
    bool IsVoicePlaying(VoiceHandle handle);
}
