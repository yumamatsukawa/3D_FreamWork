#pragma once

class Scene;
class GameObject;

// 出来事のデモ(協力パズルの最小形)の物を並べる。スイッチを押すボタン2つ、リセットのボタン、扉の見た目と判断をする役。
// 2Dと3Dの区別が無い(画面に四角を描くだけ)
GameObject* CreateNetworkEventDemo(Scene& scene);