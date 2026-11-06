#pragma once
#include "../../Components/Network/NetworkLobbyController.h"

class Scene;
class GameObject;

// ネットワークのデモ共通の「つなぐ画面」を作る(Host / Join / Find / Send / Disconnect のボタンと、状態の表示)。
// options で、出すボタンを選ぶ。戻り値は状態を表示する役のオブジェクト
GameObject* CreateNetworkLobby(Scene& scene, const NetworkLobbyOptions& options);