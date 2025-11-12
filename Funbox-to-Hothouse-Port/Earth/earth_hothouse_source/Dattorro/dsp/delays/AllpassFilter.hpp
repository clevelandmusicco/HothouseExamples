#pragma once
#include "InterpDelay.hpp"

class AllpassFilter {
public:
    AllpassFilter() {
        //clear();
        gain = 0.;
    }

    AllpassFilter(int maxDelay, int initDelay = 0, float gain = 0.) {
        //clear();
        delay = InterpDelay(maxDelay, initDelay);
        this->gain = gain;
    }

    // inline void initializeAllPassFilter(const int &maxDelay, const float &initDelay = 0, const float &gain = 0.) {
    //     clear();
    //     // delay = InterpDelay(maxDelay, initDelay);
    //     delay.initializeDelay(maxDelay, initDelay);
    //     this->gain = gain;
    // }

    #pragma GCC push_options
    #pragma GCC optimize ("Ofast")

    inline float process() {
        _inSum = input + delay.output * gain;
        output = delay.output + _inSum * gain * -1.;
        delay.input = _inSum;
        delay.process();
        return output;
    }

    #pragma GCC pop_options

    void clear() {
        input = 0.;
        output = 0.;
        _inSum = 0.;
        _outSum = 0.;
        delay.clear();
    }

    inline void setGain(const float &newGain) {
        gain = newGain;
    }

    float input;
    float output;
    InterpDelay delay;

private:
    float gain;
    float _inSum;
    float _outSum;
};