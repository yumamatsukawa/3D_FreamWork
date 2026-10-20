#pragma once

class Scene;
class GameObject;

// Soundデモの本体を作る(2Dと3Dの区別が無い機能)。
// BGM(TitleBGM.wav。ループ)と SE(shot.wav・hit.wav)を鳴らすボタン、BGM/SEの音量グループのスライダーを置く。
// 操作役(キー・フェード・表示)が付く
GameObject* CreateSoundDemo(Scene& scene);
