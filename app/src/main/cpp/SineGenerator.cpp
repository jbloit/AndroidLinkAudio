//
// Created by julien@macmini on 25/03/2018.
//

#include "SineGenerator.h"

constexpr int kDefaultFrameRate = 48000;
constexpr float kDefaultAmplitude = 0.01;
constexpr float kDefaultFrequency = 440.0;
constexpr double kTwoPi = M_PI * 2;

SineGenerator::SineGenerator() {
    setup(kDefaultFrequency, kDefaultFrameRate, kDefaultAmplitude);
}

void SineGenerator::setup(double frequency, int32_t frameRate) {
    mFrameRate = frameRate;
    mPhaseIncrement = getPhaseIncremement(frequency);
}

void SineGenerator::setup(double frequency, int32_t frameRate, float amplitude) {
    setup(frequency, frameRate);
    mAmplitude = amplitude;
}

void SineGenerator::setSweep(double frequencyLow, double frequencyHigh, double seconds) {
    mPhaseIncrementLow = getPhaseIncremement(frequencyLow);
    mPhaseIncrementHigh = getPhaseIncremement(frequencyHigh);

    double numFrames = seconds * mFrameRate;
    mUpScaler = pow((frequencyHigh / frequencyLow), (1.0 / numFrames));
    mDownScaler = 1.0 / mUpScaler;
    mGoingUp = true;
    mSweeping = true;
}

void SineGenerator::render(int16_t *buffer, int32_t channelStride, int32_t numFrames) {
    int sampleIndex = 0;
    for (int i = 0; i < numFrames; i++) {
        buffer[sampleIndex] += (int16_t) (INT16_MAX * sinf(mPhase) * mAmplitude);
        sampleIndex += channelStride;
        advancePhase();
    }
}

void SineGenerator::render(float *buffer, int32_t channelStride, int32_t numFrames) {
    for (int i = 0, sampleIndex = 0; i < numFrames; i++) {
        buffer[sampleIndex] += (float) (sinf(mPhase) * mAmplitude);
        sampleIndex += channelStride;
        advancePhase();
    }
}

void SineGenerator::advancePhase() {
    mPhase += mPhaseIncrement;
    while (mPhase >= kTwoPi) {
        mPhase -= kTwoPi;
    }
    if (mSweeping) {
        if (mGoingUp) {
            mPhaseIncrement *= mUpScaler;
            if (mPhaseIncrement > mPhaseIncrementHigh) {
                mGoingUp = false;
            }
        } else {
            mPhaseIncrement *= mDownScaler;
            if (mPhaseIncrement < mPhaseIncrementLow) {
                mGoingUp = true;
            }
        }
    }
}

double SineGenerator::getPhaseIncremement(double frequency) {
    return frequency * kTwoPi / mFrameRate;
}