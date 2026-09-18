#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class GameObject;

// 名前(文字列)で識別する、疎結合な通知の仕組み(イベントバス/Pub-Sub)。
// 「何が起きたか」を知らせる側(Publish)と、「それを聞いて何をするか」を
// 決める側(Subscribe)がお互いを直接知らなくて済むようにする。
//
// 引数なしイベントの使い方:
//   EventBus::Get().Subscribe("EnemyDefeated", []() { score += 100; });
//   EventBus::Get().Publish("EnemyDefeated");
//
// GameObjectを1つ渡すイベントの使い方(「誰が」を伝えたい時):
//   EventBus::Get().SubscribeObject("FireBullet", [](GameObject* shooter) {
//       // shooterの位置・向きから弾を出す、など
//   });
//   EventBus::Get().PublishObject("FireBullet", owner);
//
// 注意: ラムダの中で「今のScene」などのポインタをキャプチャして使う場合、
//       そのSceneが破棄された後もEventBusには登録が残り続けてしまう
//       (dangling pointerを呼び出すバグになる)。
//       Scene::Uninit()などで、必ず Clear() を呼んで登録をリセットすること。
class EventBus {
public:
    using Callback = std::function<void()>;
    using ObjectCallback = std::function<void(GameObject*)>;

    static EventBus& Get() {
        static EventBus instance;
        return instance;
    }

    // ─── 引数なしイベント ─────────────────────
    void Subscribe(const std::string& eventName, Callback callback) {
        listeners[eventName].push_back(std::move(callback));
    }

    void Publish(const std::string& eventName) {
        auto it = listeners.find(eventName);
        if (it == listeners.end()) return;
        std::vector<Callback> callbacks = it->second;  // Publish中のSubscribeに備えてコピー
        for (auto& cb : callbacks) cb();
    }

    // ─── GameObjectを1つ渡すイベント ───────────
    void SubscribeObject(const std::string& eventName, ObjectCallback callback) {
        objectListeners[eventName].push_back(std::move(callback));
    }

    void PublishObject(const std::string& eventName, GameObject* obj) {
        auto it = objectListeners.find(eventName);
        if (it == objectListeners.end()) return;
        std::vector<ObjectCallback> callbacks = it->second;
        for (auto& cb : callbacks) cb(obj);
    }

    // 登録を全部消す(シーン切り替え時などに呼ぶ)
    void Clear() {
        listeners.clear();
        objectListeners.clear();
    }

private:
    EventBus() = default;
    std::unordered_map<std::string, std::vector<Callback>> listeners;
    std::unordered_map<std::string, std::vector<ObjectCallback>> objectListeners;
};
