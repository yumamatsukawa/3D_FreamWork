#pragma once

class Scene;
class GameManager;

// ゲームの進行役(スコア・ゲームオーバー)を作る。HUDが値を読めるようGameManagerを返す
GameManager* CreateGameSystem(Scene& scene);
