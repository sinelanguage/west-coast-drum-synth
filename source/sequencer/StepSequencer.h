#pragma once

#include "wcparams.h"
#include <functional>

namespace SineLanguage {
namespace WestCoastDrumSynth {

struct StepData {
    bool active = false;
    float velocity = 0.8f;
};

class StepSequencer {
public:
    StepSequencer();

    void setSampleRate(float sr) { sampleRate_ = sr; }
    void setTempo(float bpm) { tempo_ = bpm; }
    void setSwing(float swing) { swing_ = swing; }
    void setPlaying(bool playing);

    bool isPlaying() const { return playing_; }
    int getCurrentStep() const { return currentStep_; }

    void setStep(int voice, int step, bool active);
    void setStepVelocity(int voice, int step, float velocity);
    bool getStep(int voice, int step) const;
    float getStepVelocity(int voice, int step) const;

    using TriggerCallback = std::function<void(int voice, float velocity)>;
    void setTriggerCallback(TriggerCallback cb) { triggerCallback_ = cb; }

    void process(int numSamples);
    void reset();

    void handleTransportPosition(double ppqPosition, double tempo, bool isPlaying);

private:
    float sampleRate_ = 44100.0f;
    float tempo_ = 120.0f;
    float swing_ = 0.0f;
    bool playing_ = false;

    int currentStep_ = 0;
    double sampleCounter_ = 0.0;

    StepData steps_[kNumDrumVoices][kNumSteps];
    TriggerCallback triggerCallback_;

    double getSamplesPerStep() const;
    double getSwingOffset(int step) const;
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
