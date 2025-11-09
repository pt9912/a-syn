#include "AdsrEnvelope.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr float kMinTimeSeconds = 0.001f;
constexpr float kMaxTimeSeconds = 10.0f;
constexpr float kMinSustain = 0.0f;
constexpr float kMaxSustain = 1.0f;
constexpr float kDefaultAttack = 0.01f;
constexpr float kDefaultDecay = 0.2f;
constexpr float kDefaultSustain = 0.7f;
constexpr float kDefaultRelease = 0.5f;
constexpr double kDefaultSampleRate = 44100.0;
constexpr double kMinSampleRate = 8000.0;
constexpr double kMaxSampleRate = 192000.0;
constexpr float kReleaseEpsilon = 0.0001f;

float clampTime(float value, float fallback)
{
    if (!std::isfinite(value))
        return fallback;
    return std::clamp(value, kMinTimeSeconds, kMaxTimeSeconds);
}

float clampSustain(float value)
{
    if (!std::isfinite(value))
        return kDefaultSustain;
    return std::clamp(value, kMinSustain, kMaxSustain);
}

double clampSampleRate(double value)
{
    if (!std::isfinite(value))
        return kDefaultSampleRate;
    return std::clamp(value, kMinSampleRate, kMaxSampleRate);
}
} // namespace

namespace Core
{

AdsrEnvelope::AdsrEnvelope()
{
    releaseCoeff_ = 1.0f;
    updateReleaseCoefficient();
}

void AdsrEnvelope::setSampleRate(double sampleRate)
{
    sampleRate_ = clampSampleRate(sampleRate);
    updateReleaseCoefficient();
}

void AdsrEnvelope::setAttack(float attackTimeSeconds)
{
    attackTime_ = clampTime(attackTimeSeconds, kDefaultAttack);
}

void AdsrEnvelope::setDecay(float decayTimeSeconds)
{
    decayTime_ = clampTime(decayTimeSeconds, kDefaultDecay);
}

void AdsrEnvelope::setSustain(float sustainLevel)
{
    sustainLevel_ = clampSustain(sustainLevel);
}

void AdsrEnvelope::setRelease(float releaseTimeSeconds)
{
    releaseTime_ = clampTime(releaseTimeSeconds, kDefaultRelease);
    updateReleaseCoefficient();
}

void AdsrEnvelope::noteOn()
{
    currentLevel_ = 0.0f;
    state_ = State::Attack;
}

void AdsrEnvelope::noteOff()
{
    if (state_ != State::Idle)
        state_ = State::Release;
}

void AdsrEnvelope::reset()
{
    state_ = State::Idle;
    currentLevel_ = 0.0f;
}

float AdsrEnvelope::getNextSample()
{
    const float sampleRateF = static_cast<float>(sampleRate_);

    switch (state_)
    {
        case State::Idle:
            return 0.0f;

        case State::Attack:
        {
            const float increment = 1.0f / (attackTime_ * sampleRateF);
            currentLevel_ += increment;
            if (currentLevel_ >= 1.0f)
            {
                currentLevel_ = 1.0f;
                state_ = (sustainLevel_ >= 1.0f) ? State::Sustain : State::Decay;
            }
            return currentLevel_;
        }

        case State::Decay:
        {
            if (sustainLevel_ >= 1.0f)
            {
                state_ = State::Sustain;
                currentLevel_ = sustainLevel_;
                return currentLevel_;
            }

            const float decrement = (1.0f - sustainLevel_) / (decayTime_ * sampleRateF);
            currentLevel_ -= decrement;
            if (currentLevel_ <= sustainLevel_)
            {
                currentLevel_ = sustainLevel_;
                state_ = State::Sustain;
            }
            return currentLevel_;
        }

        case State::Sustain:
            return currentLevel_;

        case State::Release:
        {
            currentLevel_ *= releaseCoeff_;
            if (currentLevel_ <= kReleaseEpsilon)
            {
                currentLevel_ = 0.0f;
                state_ = State::Idle;
            }
            return currentLevel_;
        }
    }

    return 0.0f;
}

bool AdsrEnvelope::isActive() const
{
    return state_ != State::Idle;
}

void AdsrEnvelope::updateReleaseCoefficient()
{
    const float tau = std::max(releaseTime_, kMinTimeSeconds);
    const float sr = static_cast<float>(std::max(sampleRate_, kMinSampleRate));
    releaseCoeff_ = std::exp(-1.0f / (tau * sr));
}

} // namespace Core
