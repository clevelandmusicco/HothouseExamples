/*
  ==============================================================================

    StkPitchShift.h
    Ported from STK (Synthesis Toolkit) for Daisy/Hothouse
    Original Authors: Perry R. Cook and Gary P. Scavone, 1995--2017.
    
    Simple pitch shifter using delay lines.

  ==============================================================================
*/

#pragma once

#include "daisysp.h"

using namespace daisysp;

class StkPitchShift
{
public:
    StkPitchShift(int maxDelay = 5024) : maxDelay_(maxDelay)
    {
        lastFrame_ = 0.0f;
        delayLength_ = maxDelay_ - 24;
        halfLength_ = delayLength_ / 2;
        delay_[0] = 12.0f;
        delay_[1] = maxDelay_ / 2.0f;
        effectMix_ = 0.5f;
        rate_ = 1.0f;
        
        delayLine_[0].Init();
        delayLine_[1].Init();
        delayLine_[0].SetDelay(static_cast<float>(maxDelay_));
        delayLine_[1].SetDelay(static_cast<float>(maxDelay_));
    }

    void Clear()
    {
        delayLine_[0].Reset();
        delayLine_[1].Reset();
        lastFrame_ = 0.0f;
    }

    void SetShift(float shift)
    {
        if (shift < 1.0f) {
            rate_ = 1.0f - shift;
        }
        else if (shift > 1.0f) {
            rate_ = 1.0f - shift;
        }
        else {
            rate_ = 0.0f;
        }
    }

    float LastOut() const { return lastFrame_; }

    float Process(float input)
    {
        // Calculate the two delay length values, keeping them within the
        // range 12 to maxDelay-12.
        delay_[0] += rate_;
        while (delay_[0] > maxDelay_ - 12) delay_[0] -= delayLength_;
        while (delay_[0] < 12) delay_[0] += delayLength_;

        delay_[1] = delay_[0] + halfLength_;
        while (delay_[1] > maxDelay_ - 12) delay_[1] -= delayLength_;
        while (delay_[1] < 12) delay_[1] += delayLength_;

        // Set the new delay line lengths.
        delayLine_[0].SetDelay(delay_[0]);
        delayLine_[1].SetDelay(delay_[1]);

        // Calculate a triangular envelope.
        env_[1] = fabs((delay_[0] - halfLength_ + 12) * (1.0f / (halfLength_ + 12)));
        env_[0] = 1.0f - env_[1];

        // Write input to both delay lines
        delayLine_[0].Write(input);
        delayLine_[1].Write(input);

        // Read delayed output
        lastFrame_ = env_[0] * delayLine_[0].Read();
        lastFrame_ += env_[1] * delayLine_[1].Read();

        // Compute effect mix and output.
        lastFrame_ = effectMix_ * lastFrame_ + (1.0f - effectMix_) * input;

        return lastFrame_;
    }

    void SetEffectMix(float mix)
    {
        if (mix < 0.0f) {
            effectMix_ = 0.0f;
        }
        else if (mix > 1.0f) {
            effectMix_ = 1.0f;
        }
        else {
            effectMix_ = mix;
        }
    }

private:
    float lastFrame_;
    float effectMix_;
    int maxDelay_;

    DelayLine<float, 5024> delayLine_[2];
    float delay_[2];
    float env_[2];
    float rate_;
    int delayLength_;
    int halfLength_;
};
