#pragma once

namespace Core
{

class AdsrEnvelope
{
public:
    AdsrEnvelope();
    ~AdsrEnvelope() = default;

    // Konfiguration
    void setSampleRate(double sampleRate);
    void setAttack(float attackTimeSeconds);
    void setDecay(float decayTimeSeconds);
    void setSustain(float sustainLevel);
    void setRelease(float releaseTimeSeconds);

    float getAttack() const { return attackTime_; }
    float getDecay() const { return decayTime_; }
    float getSustain() const { return sustainLevel_; }
    float getRelease() const { return releaseTime_; }
    double getSampleRate() const { return sampleRate_; }

    // Steuerung
    void noteOn();
    void noteOff();
    void reset();

    // Verarbeitung / Status
    float getNextSample();
    bool isActive() const;
    float getCurrentLevel() const { return currentLevel_; }

private:
    void updateReleaseCoefficient();

    enum class State
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    double sampleRate_ = 44100.0;
    State state_ = State::Idle;
    float currentLevel_ = 0.0f;

    float attackTime_ = 0.01f;
    float decayTime_ = 0.2f;
    float sustainLevel_ = 0.7f;
    float releaseTime_ = 0.5f;
    float releaseCoeff_ = 1.0f;
};

} // namespace Core
