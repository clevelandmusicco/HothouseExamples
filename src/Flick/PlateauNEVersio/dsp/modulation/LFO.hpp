#ifndef DSJ_LFO_HPP
#define DSJ_LFO_HPP
#include <vector>
#include <cmath>
#include <cstdint>

#ifndef  M_PI
#define M_PI		3.14159265358979323846
#endif

class TriSawLFO {
public:
    TriSawLFO(float sampleRate = 32000.0, float frequency = 1.0) {
        phase = 0.0;
        _output = 0.0;
        _sampleRate = sampleRate;
        _1_sampleRate = 1 / sampleRate;
        _step = 0.0;
        _rising = true;
        setFrequency(frequency);
        setRevPoint(0.5);
    }

    inline float process() {
        if(_step > 1.0) {
            _step -= 1.0;
            _rising = true;
        }

        if(_step >= _revPoint) {
            _rising = false;
        }

        if(_rising) {
            _output = _step * _riseRate;
        }
        else {
            _output = _step * _fallRate - _fallRate;
        }

        _step += _stepSize;
        _output *= 2.0;
        _output -= 1.0;
        return _output;
    }

    inline void setFrequency(const float &frequency) {
        if (frequency == _frequency) {
            return;
        }
        _frequency = frequency;
        calcStepSize();
    }

    inline void setRevPoint(const float &revPoint) {
        _revPoint = revPoint;
        if(_revPoint < 0.0001) {
            _revPoint = 0.0001;
        }
        if(_revPoint > 0.999) {
            _revPoint = 0.999;
        }

        _riseRate = 1.0 / _revPoint;
        _fallRate = -1.0 / (1.0 - _revPoint);
    }

    void setSamplerate(float sampleRate) {
        _sampleRate = sampleRate;
        _1_sampleRate = 1 / sampleRate;
        calcStepSize();
    }

    inline float getOutput() const {
        return _output;
    }

    float phase;

private:
    float _output;
    float _sampleRate;
    float _1_sampleRate;
    float _frequency = 0.0;
    float _revPoint;
    float _riseRate;
    float _fallRate;
    float _step;
    float _stepSize;
    bool _rising;

    inline void calcStepSize() {
        // _stepSize = _frequency / _sampleRate;
        _stepSize = _frequency * _1_sampleRate;
    }
};

#endif
