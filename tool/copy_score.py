import os
import sys
import shutil
import json
from datetime import datetime

def load_selfeditor_path(config_file):
    if os.path.exists(config_file):
        try:
            with open(config_file, "r", encoding="utf-8") as f:
                path = f.read().strip()
                if os.path.isdir(path):
                    return path
        except Exception:
            pass
    return None

def save_selfeditor_path(config_file, path):
    try:
        with open(config_file, "w", encoding="utf-8") as f:
            f.write(path)
        return True
    except Exception as e:
        print(f"設定ファイルの保存に失敗しました: {e}")
        return False

def select_selfeditor_path(config_file):
    while True:
        path = input("SelfEditorのルートパスを設定してください（例: C:\\Users\\...\\SelfEditor）\n>> ").strip()
        if not path:
            print("パスを入力してください。")
            continue
        # 入力パスをクリーンアップして絶対パスにする
        path = os.path.abspath(path)
        if not os.path.isdir(path):
            print(f"指定されたディレクトリが存在しません: {path}")
            continue
        
        if save_selfeditor_path(config_file, path):
            print(f"パスを設定しました: {path}")
            return path

def get_json_files(selfeditor_path):
    # SelfEditorのルート直下から.jsonを取得
    json_files = []
    try:
        for entry in os.scandir(selfeditor_path):
            if entry.is_file() and entry.name.lower().endswith(".json"):
                json_files.append(entry)
    except Exception as e:
        print(f"ファイル一覧の取得中にエラーが発生しました: {e}")
        return []
    
    # 更新日時の新しい順にソート
    json_files.sort(key=lambda x: x.stat().st_mtime, reverse=True)
    return json_files

def parse_score_author(file_path):
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            data = json.load(f)
            return data.get("scoreauthor", "なし")
    except Exception:
        return "パースエラー"

def main():
    print("========================================")
    print(" 譜面データ コピーツール")
    print("========================================")
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_file = os.path.join(script_dir, "selfeditor_path.txt")
    
    # １：対話式でSelfEditorのルートパス設定
    selfeditor_path = load_selfeditor_path(config_file)
    if not selfeditor_path:
        selfeditor_path = select_selfeditor_path(config_file)
    else:
        print(f"現在のSelfEditorルートパス: {selfeditor_path}")
        change = input("パスを変更しますか？[y/N] (デフォルトno) >> ").strip().lower()
        if change in ("y", "yes"):
            selfeditor_path = select_selfeditor_path(config_file)

    project_root = os.path.dirname(script_dir)
    dest_score_dir = os.path.join(project_root, "asset", "score")
    dest_sound_dir = os.path.join(project_root, "asset", "sound", "bgm")

    # ディレクトリ作成
    os.makedirs(dest_score_dir, exist_ok=True)
    os.makedirs(dest_sound_dir, exist_ok=True)

    while True:
        # ２：ルートパスから.jsonをすべて取得し、新しい順に表示。
        json_files = get_json_files(selfeditor_path)
        if not json_files:
            print(f"\n{selfeditor_path} 直下に.jsonファイルが見つかりませんでした。")
            change = input("SelfEditorルートパスを再設定しますか？[y/N] (デフォルトno) >> ").strip().lower()
            if change in ("y", "yes"):
                selfeditor_path = select_selfeditor_path(config_file)
                continue
            else:
                print("プログラムを終了します。")
                sys.exit(0)

        print("\n--- 譜面ファイル一覧 ---")
        file_info_list = []
        for idx, entry in enumerate(json_files, 1):
            mtime = datetime.fromtimestamp(entry.stat().st_mtime)
            # m/d H:M 形式
            mtime_str = f"{mtime.month}/{mtime.day} {mtime.hour}:{mtime.minute:02d}"
            author = parse_score_author(entry.path)
            file_info_list.append((entry, author, mtime_str))
            print(f"{idx}.{entry.name} | {mtime_str} | scoreauthor:{author}")

        print("\nq. 終了")
        selection = input("\nコピーする譜面の数字を選択 >> ").strip()
        if selection.lower() == 'q':
            print("プログラムを終了します。")
            sys.exit(0)

        if not selection.isdigit():
            print("無効な入力です。数字を入力してください。")
            continue

        sel_idx = int(selection) - 1
        if sel_idx < 0 or sel_idx >= len(file_info_list):
            print("範囲外の数値です。一覧にある数字を選択してください。")
            continue

        selected_entry, author, mtime_str = file_info_list[sel_idx]
        src_json_path = selected_entry.path
        json_name = selected_entry.name

        # 例外：コピー先 にファイル名が重複するファイルがあった場合
        dest_json_path = os.path.join(dest_score_dir, json_name)
        
        try:
            with open(src_json_path, "r", encoding="utf-8") as f:
                src_data = json.load(f)
        except Exception as e:
            print(f"選択したJSONファイルの読み込みに失敗しました: {e}")
            continue

        json_only = False
        if os.path.exists(dest_json_path):
            try:
                with open(dest_json_path, "r", encoding="utf-8") as f:
                    dest_data = json.load(f)
            except Exception:
                dest_data = {}
            
            src_music = src_data.get("music", "")
            dest_music = dest_data.get("music", "")

            if src_music == dest_music:
                print("\n[情報] コピー先に同名ファイルがありますが、tag:musicが同じためJSONのみ上書きします。")
                json_only = True
            else:
                print(f"\n[警告] コピー先に同名ファイルがあり、かつtag:musicが異なります。")
                print(f"コピー元: {src_music}")
                print(f"コピー先: {dest_music}")
                print("ファイル名を変えてください。")
                continue

        # ３：jsonのデータを表示し、コピーしますか？[y,n]と聞く（デフォルトyes）
        print(f"\n{selection}.{json_name} | {mtime_str} | scoreauthor:{author}")
        print("{")
        preview_keys = ["musicname", "musicauthor", "scoreauthor", "difficulty", "bpm", "thumbnail", "music"]
        for key in preview_keys:
            val = src_data.get(key, None)
            if val is not None:
                # 文字列型の場合はダブルクォーテーションで囲む
                if isinstance(val, str):
                    print(f'\t"{key}": "{val}",')
                else:
                    print(f'\t"{key}": {val},')
        print("    (以下省略)\n}")

        confirm = input("コピーしますか？[y/n] (デフォルトyes) >> ").strip().lower()
        if confirm not in ("", "y", "yes"):
            print("コピーをキャンセルしました。")
            continue

        # ４：コピー完了後の譜面と音データのパスを表示して２に戻る
        # 譜面データコピー
        try:
            shutil.copy2(src_json_path, dest_json_path)
            print(f"\n譜面データをコピーしました: {dest_json_path}")
        except Exception as e:
            print(f"譜面データのコピー中にエラーが発生しました: {e}")
            continue

        # 音データコピー
        if not json_only:
            music_filename = src_data.get("music", "")
            if music_filename:
                src_sound_path = os.path.join(selfeditor_path, "Assets", "music", music_filename)
                dest_sound_path = os.path.join(dest_sound_dir, music_filename)
                
                # コピー先にすでに同一の音楽ファイルがある場合はコピーを省略する（または上書き）
                # ここでは music タグが同じなら上書き処理を行う仕様。
                if os.path.exists(src_sound_path):
                    try:
                        shutil.copy2(src_sound_path, dest_sound_path)
                        print(f"音データをコピーしました: {dest_sound_path}")
                    except Exception as e:
                        print(f"音データのコピー中にエラーが発生しました: {e}")
                else:
                    print(f"[警告] 音データがコピー元に見つかりませんでした: {src_sound_path}")
            else:
                print("[情報] JSONにmusicタグが設定されていないため、音データのコピーをスキップしました。")
        else:
            print("[情報] tag:musicが同じため、音データのコピーをスキップしました。")

if __name__ == "__main__":
    main()
