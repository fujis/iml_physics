# iml_physics

情報メディア実験テーマ「物理エンジンを用いたアプリケーション開発」の
練習問題解答含むTA向けサンプルコード(2022年度からのglfwでmac環境にも対応したバージョン)．

[実験ページ](http://slis.tsukuba.ac.jp/~fujisawa.makoto.fu/lecture/iml/)

## コードと実験進行の関係
(未移行とあるのはまだglfw環境に移行していないもの)
1. 物理シミュレーションとは？
	- 受講生向け配布コード : src/sphere
	- 練習問題1-6の解答含むコード : src/sphere_test
2. 物理エンジンとは？
	- 練習問題1-5の解答含むコード : src/btsphere
3. 多数の剛体間の衝突判定,衝突応答
	- 受講生向け配布コード : src/btcube
	- 説明ページ1の練習問題1-8および説明ページ2の練習問題1-2の解答含むコード : src/btcube_test
4. 剛体間リンク
	- 練習問題1-5の解答含むコード : src/btlink
		- 「補足：視点の追従」も src/btlink に含まれてます．
5. 3Dモデル読み込みと弾性体
	- 説明ページ1の解答コード : src/btmodel
	- 説明ページ2の解答コード : src/btsoftbody
6. 補足ページ
	- 「補足：複合形状」: src/btcompound
	- 「補足：文字列描画」: src/btcube_text  (未移行) → imguiでtext表示できるので画面上での文字列描画については削除


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

* macで実行した場合の現時点で分かっている問題点(実験上は大きな問題ではないのでそのまま使うかも)
	* btcubeでrx_shadow_gl.hで影を付けた場合，物体がないところに影ができている．
	* btcubeでrx_shadow_glsl.hで影を付けた場合，影の履歴(残像?)みたいなのが残る．
	* btcubeでマウスで物体選択時に物体の下の方をクリックしないとつかんでくれない(マウスy座標がずれてる？)．