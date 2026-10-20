#pragma once

class Scene;
class GameObject;

// Csvデモの本体を作る(2Dと3Dの区別が無い機能)。
// デモ専用の表(Assets/Demo/DemoTable.csv)を読んで画面に出し、ファイルが保存し直されたら自動で読み直す操作役を置く
GameObject* CreateCsvDemo(Scene& scene);