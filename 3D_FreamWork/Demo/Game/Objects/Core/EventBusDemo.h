#pragma once

class Scene;
class GameObject;

// EventBusデモの中身(記録係・受け取る側・送る側とそのボタン・コイン)を作る。
// EventBusに登録するComponentが入るので、シーンの Uninit で EventBus::Get().Clear() すること
GameObject* CreateEventBusDemo(Scene& scene);
