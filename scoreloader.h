#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <set>
#include "framework/nlohmann/json.hpp"
#include "debug_ostream.h"

enum class ScoreType
{
	Enemy,
	Barrier,
	Orb,
	Hold,
	RopeHold
};

enum class ScoreWall
{
	Up,
	Down,
	Left,
	Right
};

// RopeHold(Rainbow) の回転方向。省略時は最短経路（従来互換）
enum class RotationDir
{
	CW,
	CCW
};

inline std::string NormalizeScoreToken(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return value;
}

inline ScoreType ParseScoreType(const std::string& value)
{
	const std::string token = NormalizeScoreToken(value);
	if (token == "enemy") return ScoreType::Enemy;
	if (token == "tap") return ScoreType::Enemy;		//最新の譜面エディタがtapだったのキモい
	if (token == "barrier") return ScoreType::Barrier;
	if (token == "orb") return ScoreType::Orb;
	if (token == "hold") return ScoreType::Hold;
	if (token == "rope") return ScoreType::RopeHold;
	if (token == "rainbow") return ScoreType::RopeHold;

	throw std::runtime_error("Invalid score type: " + value);
}

inline ScoreWall ParseScoreWall(const std::string& value)
{
	const std::string token = NormalizeScoreToken(value);
	if (token == "up") return ScoreWall::Up;
	if (token == "down") return ScoreWall::Down;
	if (token == "left") return ScoreWall::Left;
	if (token == "right") return ScoreWall::Right;
	throw std::runtime_error("Invalid score wall: " + value);
}

inline RotationDir ParseRotationDir(const std::string& value)
{
	const std::string token = NormalizeScoreToken(value);
	if (token == "cw")  return RotationDir::CW;
	if (token == "ccw") return RotationDir::CCW;
	throw std::runtime_error("Invalid rotation direction: " + value);
}

// スコアイベントの構造体
struct ScoreEvent
{
	float beat;
	int   lane;
	ScoreType type;
	ScoreWall wall;
	// Hold / RopeHold 専用フィールド（それ以外では beat・lane・wall と同値）
	float     endBeat;
	int       endLane;
	ScoreWall endWall;
	// RopeHold 専用：回転方向（"direction"キー省略時は hasDirection=false、最短経路を使う）
	bool        hasDirection;
	RotationDir direction;
};

// スコアデータの構造体
struct ScoreData
{
	float bpm;
	std::string music;
	// 音楽ファイルの実際の音の出だしと譜面上のbeat=0のタイミングのズレを補正する値（秒）。
	// 例: 曲の先頭に0.2秒の無音があるなら offset=0.2 とすることで、
	//     ノーツの通過タイミングと実際に聴こえる音を一致させる。
	// JSONに"offset"キーが無い場合は0.0（補正なし）として扱う。
	float offset;
	std::vector<ScoreEvent> events;
	int fullCombo = 0;
};

// 音楽ファイルのパス解決を行うインライン関数
inline std::string ResolveMusicPath(const std::string& musicFile)
{
	if (musicFile.empty()) return "";
	// 1. asset/score/ をチェック
	std::string path1 = "asset/score/" + musicFile;
	std::ifstream f1(path1);
	if (f1.good()) return path1;
	
	// 2. music/ をチェック
	std::string path2 = "music/" + musicFile;
	std::ifstream f2(path2);
	if (f2.good()) return path2;
	
	// 見つからなければデフォルトで asset/score/ を返す
	return path1;
}

// JSONファイルからスコアデータを読み込む
inline ScoreData LoadScore(const std::string& filePath)
{
	try
	{
		std::ifstream file(filePath);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file: " + filePath);
		}

		nlohmann::json jsonData;
		file >> jsonData;
		file.close();

		ScoreData scoreData;
		scoreData.bpm = jsonData["bpm"].get<float>();
		scoreData.music = jsonData["music"].get<std::string>();
		scoreData.offset = jsonData.value("offset", 0.0f);

		// イベント配列をパース
		if (jsonData.contains("events"))
		{
			for (const auto& event : jsonData["events"])
			{
				ScoreEvent scoreEvent;
				scoreEvent.beat    = event["beat"].get<float>();
				scoreEvent.lane    = event["lane"].get<int>();
				scoreEvent.type    = ParseScoreType(event["type"].get<std::string>());
				scoreEvent.wall    = ParseScoreWall(event["wall"].get<std::string>());
				scoreEvent.endBeat = event.contains("endBeat") ? event["endBeat"].get<float>() : scoreEvent.beat;
				scoreEvent.endLane = event.contains("endLane") ? event["endLane"].get<int>() : scoreEvent.lane;
				// endWall: RopeHold専用（なければ wall と同値）
				std::string endWallStr = event.value("endWall", std::string(""));
				scoreEvent.endWall = endWallStr.empty() ? scoreEvent.wall : ParseScoreWall(endWallStr);

				// direction: RopeHold の回転方向（"CW"/"CCW"）。省略時は最短経路
				std::string directionStr = event.value("direction", std::string(""));
				scoreEvent.hasDirection = !directionStr.empty();
				scoreEvent.direction    = scoreEvent.hasDirection ? ParseRotationDir(directionStr) : RotationDir::CW;

				scoreData.events.push_back(scoreEvent);
			}

			std::sort(scoreData.events.begin(), scoreData.events.end(), [](const ScoreEvent& a, const ScoreEvent& b) {
				return a.beat < b.beat;
			});
		}

		// フルコンボ（最大コンボ）数の逆算
		int fullCombo = 0;
		std::set<float> barrierBeats;
		for (const auto& ev : scoreData.events)
		{
			switch (ev.type)
			{
			case ScoreType::Enemy:
				fullCombo += 1;
				break;
			case ScoreType::Barrier:
				barrierBeats.insert(ev.beat);
				break;
			case ScoreType::Hold:
				{
					float diffBeat = ev.endBeat - ev.beat;
					float stepVal = diffBeat / 0.25f; // HOLD_BEAT_INTERVAL = 0.25f
					int totalSteps = (stepVal > 0.0f) ? (int)ceilf(stepVal) + 1 : 2;
					if (totalSteps < 2) totalSteps = 2;
					fullCombo += totalSteps;
				}
				break;
			case ScoreType::RopeHold:
				fullCombo += 1;
				break;
			case ScoreType::Orb:
				// Orb はコンボに影響しないため加算しない
				break;
			}
		}
		fullCombo += static_cast<int>(barrierBeats.size());
		scoreData.fullCombo = fullCombo;

		return scoreData;
	}
	catch (...)
	{
		std::string fallbackScore = "asset/score/shiningstar.json";
		if (filePath != fallbackScore)
		{
			hal::dout << "[scoreloader.h] ファイルの取得に失敗したため、フォールバックとして" << fallbackScore << "を読み込みました" << std::endl;
			return LoadScore(fallbackScore);
		}
		throw;
	}
}
