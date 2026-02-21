#include "StepSequencer.h"
#include <cmath>

namespace SineLanguage {
namespace WestCoastDrumSynth {

StepSequencer::StepSequencer() {
    reset();
}

void StepSequencer::setPlaying(bool playing) {
    if (playing && !playing_) {
        currentStep_ = 0;
        sampleCounter_ = 0.0;
    }
    playing_ = playing;
}

void StepSequencer::setStep(int voice, int step, bool active) {
    if (voice >= 0 && voice < kNumDrumVoices && step >= 0 && step < kNumSteps) {
        steps_[voice][step].active = active;
    }
}

void StepSequencer::setStepVelocity(int voice, int step, float velocity) {
    if (voice >= 0 && voice < kNumDrumVoices && step >= 0 && step < kNumSteps) {
        steps_[voice][step].velocity = velocity;
    }
}

bool StepSequencer::getStep(int voice, int step) const {
    if (voice >= 0 && voice < kNumDrumVoices && step >= 0 && step < kNumSteps) {
        return steps_[voice][step].active;
    }
    return false;
}

float StepSequencer::getStepVelocity(int voice, int step) const {
    if (voice >= 0 && voice < kNumDrumVoices && step >= 0 && step < kNumSteps) {
        return steps_[voice][step].velocity;
    }
    return 0.8f;
}

double StepSequencer::getSamplesPerStep() const {
    // 16th notes: 4 steps per beat
    double beatsPerSecond = tempo_ / 60.0;
    double stepsPerSecond = beatsPerSecond * 4.0;
    return sampleRate_ / stepsPerSecond;
}

double StepSequencer::getSwingOffset(int step) const {
    if (step % 2 == 1) {
        return swing_ * getSamplesPerStep() * 0.5;
    }
    return 0.0;
}

void StepSequencer::process(int numSamples) {
    if (!playing_) return;

    double samplesPerStep = getSamplesPerStep();

    for (int i = 0; i < numSamples; ++i) {
        sampleCounter_ += 1.0;

        double threshold = samplesPerStep + getSwingOffset(currentStep_);
        if (sampleCounter_ >= threshold) {
            sampleCounter_ -= threshold;
            currentStep_ = (currentStep_ + 1) % kNumSteps;

            if (triggerCallback_) {
                for (int v = 0; v < kNumDrumVoices; ++v) {
                    if (steps_[v][currentStep_].active) {
                        triggerCallback_(v, steps_[v][currentStep_].velocity);
                    }
                }
            }
        }
    }
}

void StepSequencer::handleTransportPosition(double ppqPosition, double tempo, bool isPlaying) {
    tempo_ = static_cast<float>(tempo);
    playing_ = isPlaying;

    if (isPlaying) {
        double stepsPerBeat = 4.0;
        double totalSteps = ppqPosition * stepsPerBeat;
        currentStep_ = static_cast<int>(std::fmod(totalSteps, static_cast<double>(kNumSteps)));
    }
}

void StepSequencer::reset() {
    currentStep_ = 0;
    sampleCounter_ = 0.0;
    for (int v = 0; v < kNumDrumVoices; ++v) {
        for (int s = 0; s < kNumSteps; ++s) {
            steps_[v][s].active = false;
            steps_[v][s].velocity = 0.8f;
        }
    }
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
