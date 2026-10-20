#pragma once

class Scene;
class GameObject;

// Timeデモの中身(倍率の影響を受けて回る物・受けずに回る物・操作役)を作る
GameObject* CreateTimeDemo(Scene& scene);
