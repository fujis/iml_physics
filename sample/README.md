# macでのビルド

1. パッケージマネージャ Homebrew をインストール(参考:https://qiita.com/zaburo/items/29fe23c1ceb6056109fd )
2. ターミナルからHomebrewで必要なパッケージをインストール  
`$ brew install glfw`  
`$ brew install glew`  
3. ターミナルでMakefileがあるフォルダに移動し，
`$ make`
でビルド(ビルド済みのファイルを削除してリセットする場合は`$ make clean`)
4. ビルドに成功したら(特にエラーメッセージが出なければ)，
`$ make run`
で実行

