#pragma once

#include <tweeny.h>

class ITweenClip {
public:
    virtual ~ITweenClip() = default;
    virtual void update(int step) = 0;
};

template<typename TweenType>
class TweenClip : public ITweenClip {
public:
    TweenClip(TweenType start_, TweenType end_, float duration, tweeny::easing easingStyle, int frameDuration)
        : start(start_), end(end_) {
    }

    void update(int step) override {
        // Implémente l'interpolation ici
    }

    TweenType get_step(int step);

    TweenType start;
    TweenType end;
};