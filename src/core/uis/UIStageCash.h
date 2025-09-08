#pragma once

#include "UITextElement.h"

class Stage;

class UIStageCash : public UITextElement
{
public:

	UIStageCash(std::shared_ptr<Stage> stage);

	void update(float deltaTime) override;

private:

	std::weak_ptr<Stage> m_stage;
};