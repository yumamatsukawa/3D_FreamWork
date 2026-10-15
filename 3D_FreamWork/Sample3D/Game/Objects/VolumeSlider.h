#pragma once

class Scene;
class GameObject;
class TitleController;

// タイトル画面のBGM音量スライダーを作る(0〜1。動かすとtitleのBGMの音量が変わり、離した時に保存される)
GameObject* CreateVolumeSlider(Scene& scene, TitleController* title);
