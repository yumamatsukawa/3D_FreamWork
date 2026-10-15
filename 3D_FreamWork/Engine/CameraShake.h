#pragma once

// 画面を揺らす(2D用カメラ Image::GetCamera() と 3D用カメラ Image::GetCamera3D() の両方)。
// カメラの position を直接は変えず、「描画している間だけ」カメラをずらして、描き終わったら元の値に戻す。
// そのため Update の中で読むカメラの値・Input::GetMouseWorldPosition()・CameraFollowComponent の計算には揺れが入らない
// (Draw の中で読むカメラの値・Image::WorldToScreen2D などには揺れが入る。描いた絵と一緒に揺れる)
//
// 使い方:
//   CameraShake::Shake(6.f, 0.2f);   // 強さ6(2Dはピクセル、3Dはワールドの単位)で0.2秒揺らす
//
// - 揺れはだんだん弱まって、duration秒で0になる。時間はポーズ中も止まらない(Time::GetUnscaledDeltaTime())
// - 揺れている途中にまた Shake が呼ばれた時は、「今の揺れの強さ(弱まった後の値)」と比べて、
//   新しい方が強いか同じなら新しい揺れに置き換える(最初から数え直す)。弱ければ無視する(強い揺れが弱い揺れで消されない)
// - 3Dカメラは、カメラから見た右・上の向きにずらす(position と target を同じだけずらすので、向きは変わらない)
// - シーンを切り替えると、揺れは止まる(SceneManagerが Stop を呼ぶ)
namespace CameraShake {
    void Shake(float strength, float duration);  // 画面を揺らす(強さ, 秒)。どちらかが0以下なら何もしない
    bool IsShaking();                            // 揺れている途中か
    void Stop();                                 // 揺れをすぐに止める(描画の途中で呼ばれても、カメラはEndDrawでちゃんと元に戻る)

    // ─── Engine内部用(GameEngineだけが呼ぶ。ゲーム側からは呼ばない) ─────────────
    void Update(float unscaledDt);  // 毎フレーム1回、揺れを進める
    void BeginDraw();               // 描画の前: 揺れの分だけカメラをずらす
    void EndDraw();                 // 描画の後: カメラを描画の前の値に戻す
}
