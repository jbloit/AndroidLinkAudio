//
// Created by julien@macmini on 07/05/2018.
//

#include "AudioEngine.h"
#include "logging_macros.h"
#include <inttypes.h>
#include <cstring>

// ------------------------------------------------------------------------------------------------
//                                           RENDERING
// ------------------------------------------------------------------------------------------------

DataCallbackResult
AudioEngine::onAudioReady(AudioStream *audioStream, void *audioData, int32_t numFrames) {

    int32_t bufferSize = audioStream->getBufferSizeInFrames();

    if (mBufferSizeSelection == kBufferSizeAutomatic) {
        mLatencyTuner->tune();
    } else if (bufferSize != (mBufferSizeSelection * mFramesPerBurst)) {
        audioStream->setBufferSizeInFrames(mBufferSizeSelection * mFramesPerBurst);
        bufferSize = audioStream->getBufferSizeInFrames();
    }

    int32_t channelCount = audioStream->getChannelCount();

    auto sessionState = link.captureAudioSessionState();
    const auto hostTime = mHostTimeFilter.sampleTimeToHostTime(mSampleTime);
    mSampleTime += numFrames;

    std::chrono::milliseconds latencyMs(static_cast<std::int64_t>(mCurrentOutputLatencyMillis));
    std::chrono::microseconds latencyMuSec(latencyMs);
    const std::chrono::microseconds bufferBeginAtOutput = hostTime + latencyMuSec;
    const auto microsPerSample = 1e6 / mSampleRate;


    if (audioStream->getFormat() == oboe::AudioFormat::Float) {

        // Logic: successive renders are ADDED to the initial zero buffer.

        // zero-fill buffer
        memset(static_cast<float *>(audioData), 0,
               sizeof(float) * channelCount * numFrames);

        renderBarClick(static_cast<float *>(audioData) + 1, channelCount,
                       numFrames, sessionState, bufferBeginAtOutput, microsPerSample);

        // sinewave synth
        for (int i = 0; i < channelCount; ++i) {
            mOscillators[i].render(static_cast<float *>(audioData) + i, channelCount, numFrames);
        }

    } else
        // ---------------------------------------------------------------------------------
        //                  RENDER TO INT16 INSTEAD OF FLOAT
        // ---------------------------------------------------------------------------------
    {
        // zero-fill buffer
        memset(static_cast<uint16_t *>(audioData), 0,
               sizeof(int16_t) * channelCount * numFrames);

    }

    if (mIsLatencyDetectionSupported) {
        calculateCurrentOutputLatencyMillis(audioStream, &mCurrentOutputLatencyMillis);
    }

    return DataCallbackResult::Continue;
}


void AudioEngine::prepareOscillators() {

    double frequency = 440.0;
    constexpr double interval = 110.0;
    constexpr float amplitude = 0.4;

    for (SineGenerator &osc : mOscillators){
        osc.setup(frequency, mSampleRate, amplitude);
        frequency += interval;
    }
}

void AudioEngine::renderBarClick(float *buffer,
                                     int32_t channelStride,
                                     int32_t numFrames,
                                     ableton::Link::SessionState sessionState,
                                     std::chrono::microseconds bufferBeginAtOutput,
                                     double microsPerSample) {
    for (int i = 0, sampleIndex = 0; i < numFrames; i++) {

        const auto sampleHostTime = bufferBeginAtOutput + std::chrono::microseconds(llround(i * microsPerSample));
        float sample = 0;
        const double barPhase = sessionState.phaseAtTime(sampleHostTime, mQuantum);
        if ((barPhase - mLastBarPhase) < -(mQuantum/2)) {
            // uncomment if you want to render a click on each bar.
            sample = 1.0;
            LOGD("BAR PHASE AT TICK: %f, lastBarPhase %f", barPhase, mLastBarPhase);
            timeAtLastBar = sampleHostTime;
        }
        buffer[sampleIndex] +=  sample;
        sampleIndex += channelStride;
        mLastBarPhase = barPhase;
    }
}

// ------------------------------------------------------------------------------------------------
//                                           LIFECYCLE
// ------------------------------------------------------------------------------------------------
AudioEngine::AudioEngine(): link(60.) {

    mPlayStatus = stopped;
    mLastBeatPhase = 0.;
    mLastBarPhase = 0.;
    mSampleTime = 0.0;
    createPlaybackStream();
    link.enable(true);


}

AudioEngine::~AudioEngine() {
    closeOutputStream();
}

void AudioEngine::createPlaybackStream() {

    oboe::AudioStreamBuilder builder;
    setupPlaybackStreamParameters(&builder);

    oboe::Result result = builder.openStream(&mPlayStream);

    if (result == oboe::Result::OK && mPlayStream != nullptr) {

        mSampleRate = mPlayStream->getSampleRate();
        mFramesPerBurst = mPlayStream->getFramesPerBurst();

        int channelCount = mPlayStream->getChannelCount();
        if (channelCount != mChannelCount){
            LOGW("Requested %d channels but received %d", mChannelCount, channelCount);
        }

        // Set the buffer size to the burst size - this will give us the minimum possible latency
        mPlayStream->setBufferSizeInFrames(mFramesPerBurst);

        prepareOscillators();

        // Create a latency tuner which will automatically tune our buffer size.
        mLatencyTuner = std::unique_ptr<oboe::LatencyTuner>(new oboe::LatencyTuner(*mPlayStream));
        // Start the stream - the dataCallback function will start being called
        result = mPlayStream->requestStart();
        if (result != oboe::Result::OK) {
            LOGE("Error starting stream. %s", oboe::convertToText(result));
        }

        mIsLatencyDetectionSupported = (mPlayStream->getTimestamp(CLOCK_MONOTONIC, 0, 0) !=
                                        oboe::Result::ErrorUnimplemented);

    } else {
        LOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
    }
}

void AudioEngine::closeOutputStream() {

    if (mPlayStream != nullptr) {
        oboe::Result result = mPlayStream->requestStop();
        if (result != oboe::Result::OK) {
            LOGE("Error stopping output stream. %s", oboe::convertToText(result));
        }

        result = mPlayStream->close();
        if (result != oboe::Result::OK) {
            LOGE("Error closing output stream. %s", oboe::convertToText(result));
        }
    }
}

void AudioEngine::setupPlaybackStreamParameters(oboe::AudioStreamBuilder *builder) {
    builder->setAudioApi(mAudioApi);
    builder->setDeviceId(mPlaybackDeviceId);
    builder->setChannelCount(mChannelCount);

    // We request EXCLUSIVE mode since this will give us the lowest possible latency.
    // If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode.
    builder->setSharingMode(oboe::SharingMode::Exclusive);
    builder->setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder->setCallback(this);
}

oboe::Result AudioEngine::calculateCurrentOutputLatencyMillis(oboe::AudioStream *stream,
                                                     double *latencyMillis) {

    // Get the time that a known audio frame was presented for playing
    int64_t existingFrameIndex;
    int64_t existingFramePresentationTime;
    oboe::Result result = stream->getTimestamp(CLOCK_MONOTONIC,
                                               &existingFrameIndex,
                                               &existingFramePresentationTime);

    if (result == oboe::Result::OK) {

        // Get the write index for the next audio frame
        int64_t writeIndex = stream->getFramesWritten();

        // Calculate the number of frames between our known frame and the write index
        int64_t frameIndexDelta = writeIndex - existingFrameIndex;

        // Calculate the time which the next frame will be presented
        int64_t frameTimeDelta = (frameIndexDelta * oboe::kNanosPerSecond) / mSampleRate;
        int64_t nextFramePresentationTime = existingFramePresentationTime + frameTimeDelta;

        // Assume that the next frame will be written at the current time
        using namespace std::chrono;
        int64_t nextFrameWriteTime =
                duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();

        // Calculate the latency
        *latencyMillis = (double) (nextFramePresentationTime - nextFrameWriteTime)
                         / kNanosPerMillisecond;
    } else {
        LOGE("Error calculating latency: %s", oboe::convertToText(result));
    }

    return result;
}