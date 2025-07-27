#pragma once

#include "TweenClip.h"

#include <vector>
#include <memory>

class Tween 
{
public:

	template<typename TweenType>
	std::shared_ptr<TweenClip<TweenType>> create(TweenType start, TweenType end, int frameDuration, tweeny::easing easingStyle);

	template<typename TweenType>
	bool play(TweenClip<TweenType> tweenClip);


private:

	std::vector<std::shared_ptr<ITweenClip>> m_tweenClips;
};