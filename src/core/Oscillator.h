#pragma once

namespace Core
{

enum class Waveform
{
    Sine,
    Saw,
    Square,
    Triangle
};

class Oscillator
{
public:
    Oscillator() = default;
    ~Oscillator() = default;

    void setSampleRate(double sampleRate);
    void setFrequency(float frequency);
    void setWaveform(Waveform waveform);

    float getNextSample();
    void reset();

private:
    double sampleRate_ = 44100.0;
    float frequency_ = 440.0f;
    Waveform waveform_ = Waveform::Sine;
    float phase_ = 0.0f;

    float generateSine();
    float generateSaw();
    float generateSquare();
    float generateTriangle();
};

} // namespace Core
