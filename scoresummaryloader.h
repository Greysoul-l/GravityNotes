#pragma once

#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include "framework/nlohmann/json.hpp"
#include "debug_ostream.h"

struct ScoreSummary
{
	std::string jsonname;
	std::string musicname;
	std::string musicauthor;
	std::string scoreauthor;
	float difficulty = 0.0f;
	float bpm = 0.0f;
	std::string thumbnail;
	std::string music;
	int vinylIndex = -1;
};

inline ScoreSummary LoadSingleScoreSummary(const std::string& jsonName, const std::string& directoryPath = "asset\\score")
{
	ScoreSummary summary;

	if (jsonName.empty())
	{
		return summary;
	}

	const std::string jsonPath = directoryPath + "\\" + jsonName;
	std::ifstream file(jsonPath);
	if (!file.is_open())
	{
		return summary;
	}

	try
	{

		nlohmann::json jsonData;
		file >> jsonData;

		summary.jsonname = jsonName;
		hal::dout << summary.jsonname << std::endl;
		summary.musicname = jsonData.value("musicname", "");
		hal::dout << summary.musicname << std::endl;
		summary.musicauthor = jsonData.value("musicauthor", "");
		summary.scoreauthor = jsonData.value("scoreauthor", "");
		summary.difficulty = jsonData.value("difficulty", 0.0f);
		summary.bpm = jsonData.value("bpm", 0.0f);
		summary.thumbnail = jsonData.value("thumbnail", "");
		summary.music = jsonData.value("music", "");
	}
	catch (...)
	{
		return ScoreSummary{};
	}

	return summary;
}

inline std::vector<ScoreSummary> LoadScoreSummaries(const std::string& directoryPath = "asset\\score")
{
	std::vector<ScoreSummary> summaries;

	const std::string pattern = directoryPath + "\\*.json";
	WIN32_FIND_DATAA findData = {};
	HANDLE findHandle = FindFirstFileA(pattern.c_str(), &findData);
	int index = 0;

	if (findHandle == INVALID_HANDLE_VALUE)
	{
		return summaries;
	}
	do
	{
		if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			continue;
		}

		ScoreSummary summary = LoadSingleScoreSummary(findData.cFileName, directoryPath);
		if (!summary.jsonname.empty())
		{
			summaries.push_back(summary);
		}


	} while (FindNextFileA(findHandle, &findData) != 0);

	FindClose(findHandle);

	std::sort(summaries.begin(), summaries.end(), [](const ScoreSummary& a, const ScoreSummary& b)
		{
			if (a.difficulty == b.difficulty)
			{
				return a.jsonname < b.jsonname;
			}
			return a.difficulty > b.difficulty;
		});

	for (size_t i = 0; i < summaries.size(); ++i)
	{
		summaries[i].vinylIndex = static_cast<int>(i);
	}

	return summaries;
}