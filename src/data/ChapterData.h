#pragma once

#include <string>
#include <vector>

struct ChapterData
{
	int UID = -1;

	std::string name;
	std::string description;

	std::string mapTexture;

	struct StageMapData
	{
		int UID = -1;
		sf::Vector2f position = { 0.f, 0.f };
	};

	std::vector<StageMapData> stages;
};