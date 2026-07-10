譜面エディタ（SelfEditor）からゲーム内にコピーするpyスクリプトを書く
pyはGravityNotes\toolフォルダに配置

コピー元のフォルダ構造
譜面：SelfEditor\xxx.json
音楽：SelfEditor\Assets\music\xxx.mp3

コピー先のフォルダ構造
譜面：GravityNotes\asset\score
音楽：GravityNotes\asset\sound\bgm

動作フロー
１：対話式でSelfEditorのルートパス設定
２：ルートパスから.jsonをすべて取得し、新しい順に以下のように表示。tag:scoreauthorのみjsonから読み取る。数字キーの入力を受け付ける

表示イメージ）
1.music1.json | 7/3 15:34 | scoreauthor:R.T
2.music2.json | 7/2 1:4 | scoreauthor:S.S
3.music3.json | 6/30 0:34 | scoreauthor:M.H

コピーする譜面の数字を選択 >> 

例外：コピー先 にファイル名が重複するファイルがあった場合
tag:musicが同じだった場合、jsonのみ上書き。
tag:musicが違う場合、ファイル名を変えてくださいと表示し２に戻る

３：jsonのデータを表示し、コピーしますか？[y,n]と聞く（デフォルトyes）

表示イメージ）
1.music1.json | 7/3 15:34 | scoreauthor:R.T
{
	"musicname": "シャイニングスター",
	"musicauthor": "森田交一",
	"scoreauthor": "S.S",
	"difficulty": 3.0,
	"bpm": 158.0,
	"thumbnail": "shiningstar.png",
	"music": "maou_14_shining_star.mp3",
    (以下省略)

コピーしますか？[y,n]

４：コピー完了後の譜面と音データのパスを表示して２に戻る

コピー先
譜面データ（xxx.json）
コピー先→GravityNotes\asset\score

音データ：譜面データのmusicタグから"xxx.mp3"を読みとり、"SelfEditor\Assets\music\"を足す
コピー先→GravityNotes\asset\sound\bgm