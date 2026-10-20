#pragma once

class Scene;
class GameObject;

// Outlineデモの本体を作る。枠線を付ける立方体・壁と壁の後ろの球(壁越しの枠線)・縁の光を付ける球と、操作役
GameObject* CreateOutlineDemo(Scene& scene);
