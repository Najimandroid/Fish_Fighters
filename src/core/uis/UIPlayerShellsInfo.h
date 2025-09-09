#pragma once

#include "UITextElement.h"

class DataLoader;

class UIPlayerShellsInfo : public UITextElement
{
public:

	UIPlayerShellsInfo(std::shared_ptr<DataLoader> dataLoader);

	void update(float deltaTime) override;

private:

	std::shared_ptr<DataLoader> m_dataLoader;

	const float OFFSET_Y = 10.f;
	const float OFFSET_X = 10.f;
};