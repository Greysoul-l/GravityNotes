# Rainbowノーツ直後のEnemyタイミングズレ不具合 調査・修正報告

## 1. 発生していた現象
譜面データ上は、`beat: 16.0` に長さ3拍の Rainbowノーツ（`endBeat: 19.0`）、その1拍後の `beat: 20.0` から8分のEnemyノーツが配置されているにもかかわらず、**Rainbowをホールドし始めた（16.0拍）直後の 17.0拍目付近のタイミングで次のEnemyが流れてきてぶつかってしまう（タイミングが3拍分詰まる）** 現象が発生していました。

---

## 2. 調査の経緯とデータ分析

### ① 譜面データの検証
デバッグログより、JSONのロード結果は以下の通り正常であることが確認されました。
- `beat: 16.0`, `endBeat: 19.0` の Rainbowノーツは正しく認識されている。

### ② Rainbowホールド中の時間経過の検証
ホールド中のZ座標と進行度の推移ログを収集した結果：
- ホールド開始から完了まで、実時間で **約1.15秒〜1.18秒（3拍分）** かかっており、ロープ進行度の計算自体は正常。

### ③ スポーン時・判定時のタイミング検証
ノーツの生成時と判定時の `m_ElapsedTime`（BGM経過時間）をダンプした結果、決定的な原因が判明しました：
- `beat: 20` のEnemy： `elapsed = -0.10秒`（BGM開始直前）に `initZ = 79.9` でスポーン。
- `beat: 20.5` のEnemy： `elapsed = 2.64秒`（BGM開始直後）に `initZ = 54.4` でスポーン。

`beat: 20.5` の本来のスポーン時刻は **約0.09秒** であるべきですが、BGMが鳴り始めた直後に `m_ElapsedTime` が `0.0秒` から **いきなり `2.64秒` へジャンプ（ワープ）** していました。

---

## 3. 根本原因：XAudio2 の SamplesPlayed 累積仕様

`NoteManager` は、BGMの現在時間を `GetPlaybackPositionSec()` から取得しています。この関数は、XAudio2のソースボイスが持つ `SamplesPlayed`（再生済みサンプル数）をベースに時間を計算していました。

しかし、XAudio2の仕様として以下の特徴があります：
1. `IXAudio2SourceVoice::Stop()` や `FlushSourceBuffers()` を実行しても、`SamplesPlayed`（累計カウンタ）は **リセットされず累積し続ける**。
2. 本ゲームでは、ステージ選択画面ですでにそのステージのBGMが裏でループ再生されています。ゲーム画面に遷移した際、同じBGMのアドレッシング（キャッシュ）が引き継がれます。
3. そのため、ゲームが開始して `PlaySound` が呼ばれた瞬間、BGMの曲自体は頭から再生し直されるものの、`SamplesPlayed` は **「ステージ選択画面やロード中に裏で再生されていた時間」を保持した状態（今回のログでは約2.64秒分）** でスタートしてしまいます。

結果として、ゲーム開始（`m_ElapsedTime = 0.0`）になった次のフレームで、経過時間が一気に `2.64秒` にワープし、それ以降にスポーンするノーツが **手前（プレイヤーの近く）のZ座標でスポーンされ、その分だけ早くプレイヤーに到達してしまっていた** のが、タイミングが詰まって見えた原因でした。

---

## 4. 解決方法（修正内容）

再生開始時の累積サンプル数をオフセットとして差し引くことで、常に「今回の再生の開始時点」を `0秒` とするように修正しました。

### ① `sound.h` の修正
`SoundData` 構造体に `startSamples` メンバーを追加。
```cpp
struct SoundData {
    ...
    UINT64 startSamples = 0; // 追加：再生開始時の累積サンプル数オフセット
};
```

### ② `sound.cpp` の修正
- `PlaySound` 関数にて、バッファを送信して再生を開始する直前の `SamplesPlayed` を記録。
  ```cpp
  XAUDIO2_VOICE_STATE voiceState = {};
  data->pSourceVoice->GetState(&voiceState);
  data->startSamples = voiceState.SamplesPlayed;
  ```
- `GetPlaybackPositionSec` 関数にて、取得した `SamplesPlayed` から `startSamples` を差し引いて時間を計算。
  ```cpp
  double GetPlaybackPositionSec(const SoundData* data)
  {
      if (!data || !data->pSourceVoice || !data->pWfx) return 0.0;
      XAUDIO2_VOICE_STATE state = {};
      data->pSourceVoice->GetState(&state);
      return static_cast<double>(state.SamplesPlayed - data->startSamples) / data->pWfx->nSamplesPerSec;
  }
  ```

これにより、ステージ選択画面から遷移した直後の最初のプレイであっても、リトライ時であっても、経過時間がBGMの聴感上の再生位置と常に `0.0秒` から完璧に同期するようになり、不具合が解消されました。
