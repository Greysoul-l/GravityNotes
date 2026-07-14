#include "options_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

static std::string g_DirectPlayPath = "";
static int g_StartMeasure = 0;

void Options_Initialize()
{
	std::ifstream ifs("options.yml");
	if (!ifs.is_open())
	{
		return;
	}

	std::string line;
	while (std::getline(ifs, line))
	{
		// コメントの除去
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
		{
			line = line.substr(0, commentPos);
		}

		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos)
		{
			continue;
		}

		std::string key = line.substr(0, colonPos);
		std::string val = line.substr(colonPos + 1);

		// 前後の余白トリム
		key.erase(0, key.find_first_not_of(" \t\r\n"));
		if (!key.empty())
		{
			key.erase(key.find_last_not_of(" \t\r\n") + 1);
		}

		val.erase(0, val.find_first_not_of(" \t\r\n"));
		if (!val.empty())
		{
			val.erase(val.find_last_not_of(" \t\r\n") + 1);
		}

		if (key == "directplay")
		{
			// クォーテーション囲みの除去
			if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
			{
				val = val.substr(1, val.size() - 2);
			}
			else if (val.size() >= 2 && val.front() == '\'' && val.back() == '\'')
			{
				val = val.substr(1, val.size() - 2);
			}

			g_DirectPlayPath = val;
		}
		else if (key == "startmeasure")
		{
			try
			{
				g_StartMeasure = std::stoi(val);
			}
			catch (...)
			{
				g_StartMeasure = 0;
			}
		}
	}
}

const std::string& Options_GetDirectPlayPath()
{
	return g_DirectPlayPath;
}

int Options_GetStartMeasure()
{
	return g_StartMeasure;
}
