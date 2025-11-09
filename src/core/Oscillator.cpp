#include "Oscillator.h"
#include <cmath>

namespace Core
{

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;

void Oscillator::setSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
}

void Oscillator::setFrequency(float frequency)
{
    frequency_ = frequency;
}

void Oscillator::setWaveform(Waveform waveform)
{
    waveform_ = waveform;
}

float Oscillator::getNextSample()
{
    float sample = 0.0f;

    switch (waveform_)
    {
        case Waveform::Sine:
            sample = generateSine();
            break;
        case Waveform::Saw:
            sample = generateSaw();
            break;
        case Waveform::Square:
            sample = generateSquare();
            break;
        case Waveform::Triangle:
            sample = generateTriangle();
            break;
    }

    // Update phase
    phase_ += frequency_ / static_cast<float>(sampleRate_);
    if (phase_ >= 1.0f)
        phase_ -= 1.0f;

    return sample;
}

void Oscillator::reset()
{
    phase_ = 0.0f;
}

float Oscillator::generateSine()
{
    return std::sin(TWO_PI * phase_);
}

float Oscillator::generateSaw()
{
    // Simple naive sawtooth (will be replaced with bandlimited version)
    return 2.0f * phase_ - 1.0f;
}

float Oscillator::generateSquare()
{
    // Simple naive square (will be replaced with bandlimited version)
    return phase_ < 0.5f ? 1.0f : -1.0f;
}

float Oscillator::generateTriangle()
{
    // Simple naive triangle
    if (phase_ < 0.5f)
        return 4.0f * phase_ - 1.0f;
    else
        return 3.0f - 4.0f * phase_;
}

} // namespace Core
