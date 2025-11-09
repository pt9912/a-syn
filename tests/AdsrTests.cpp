#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <limits>

#include "../src/core/AdsrEnvelope.h"

using namespace Core;

TEST_CASE("AdsrEnvelope basic functionality", "[adsr]")
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(44100.0);

    SECTION("Initial state is idle")
    {
        REQUIRE_FALSE(envelope.isActive());
        REQUIRE(envelope.getNextSample() == 0.0f);
    }

    SECTION("Note on activates envelope")
    {
        envelope.noteOn();
        REQUIRE(envelope.isActive());
    }

    SECTION("Attack phase increases level")
    {
        envelope.setAttack(0.1f);
        envelope.noteOn();

        float firstSample = envelope.getNextSample();
        float secondSample = envelope.getNextSample();

        REQUIRE(secondSample > firstSample);
        REQUIRE(firstSample > 0.0f);
    }

    SECTION("Sustain level is maintained")
    {
        envelope.setAttack(0.001f);
        envelope.setDecay(0.001f);
        envelope.setSustain(0.5f);
        envelope.noteOn();

        // Skip attack and decay phases
        for (int i = 0; i < 1000; ++i)
            envelope.getNextSample();

        // Check sustain level
        float sustainSample = envelope.getNextSample();
        REQUIRE(sustainSample == Catch::Approx(0.5f).margin(0.1f));
    }

    SECTION("Release phase decreases level")
    {
        envelope.setAttack(0.001f);
        envelope.setDecay(0.001f);
        envelope.setSustain(0.7f);
        envelope.setRelease(0.1f);

        envelope.noteOn();

        // Reach sustain
        for (int i = 0; i < 500; ++i)
            envelope.getNextSample();

        envelope.noteOff();

        float beforeRelease = envelope.getNextSample();
        float afterRelease = envelope.getNextSample();

        REQUIRE(afterRelease < beforeRelease);
    }

    SECTION("setRelease during release recalculates coefficient")
    {
        envelope.setAttack(0.001f);
        envelope.setDecay(0.001f);
        envelope.setSustain(0.8f);
        envelope.setRelease(0.5f);
        envelope.noteOn();

        for (int i = 0; i < 800; ++i)
            envelope.getNextSample();

        envelope.noteOff();

        float levelBeforeChange = envelope.getNextSample();
        float levelBeforeChangeNext = envelope.getNextSample();
        float slowDelta = levelBeforeChange - levelBeforeChangeNext;

        envelope.setRelease(0.01f);

        float levelAfterChange = envelope.getNextSample();
        float levelAfterChangeNext = envelope.getNextSample();
        float fastDelta = levelAfterChange - levelAfterChangeNext;

        REQUIRE(levelAfterChangeNext < levelAfterChange);
        REQUIRE(fastDelta > slowDelta * 2.0f);
    }

    SECTION("Sustain level zero holds silence")
    {
        envelope.setAttack(0.001f);
        envelope.setDecay(0.001f);
        envelope.setSustain(0.0f);
        envelope.setRelease(0.01f);
        envelope.noteOn();

        for (int i = 0; i < 1000; ++i)
            envelope.getNextSample();

        float sustainSample = envelope.getNextSample();
        REQUIRE(sustainSample == Catch::Approx(0.0f).margin(0.001f));

        envelope.noteOff();
        for (int i = 0; i < 5000; ++i)
            envelope.getNextSample();

        REQUIRE_FALSE(envelope.isActive());
        REQUIRE(envelope.getNextSample() == 0.0f);
    }

    SECTION("Envelope returns to idle after release")
    {
        envelope.setAttack(0.001f);
        envelope.setDecay(0.001f);
        envelope.setSustain(0.5f);
        envelope.setRelease(0.001f);

        envelope.noteOn();

        // Complete cycle
        for (int i = 0; i < 500; ++i)
            envelope.getNextSample();

        envelope.noteOff();

        // Complete release
        for (int i = 0; i < 500; ++i)
            envelope.getNextSample();

        REQUIRE_FALSE(envelope.isActive());
        REQUIRE(envelope.getNextSample() == 0.0f);
    }
}

TEST_CASE("AdsrEnvelope parameter validation", "[adsr]")
{
    AdsrEnvelope envelope;

    SECTION("Sustain level is clamped to [0, 1]")
    {
        envelope.setSustain(-0.5f);
        REQUIRE(envelope.getSustain() == Catch::Approx(0.0f));

        envelope.setSustain(1.5f);
        REQUIRE(envelope.getSustain() == Catch::Approx(1.0f));
    }

    SECTION("Time parameters have minimum values")
    {
        envelope.setAttack(0.0f);
        envelope.setDecay(0.0f);
        envelope.setRelease(0.0f);

        REQUIRE(envelope.getAttack() == Catch::Approx(0.001f));
        REQUIRE(envelope.getDecay() == Catch::Approx(0.001f));
        REQUIRE(envelope.getRelease() == Catch::Approx(0.001f));
    }

    SECTION("NaN inputs fall back to defaults")
    {
        envelope.setSampleRate(std::numeric_limits<double>::quiet_NaN());
        envelope.setAttack(std::numeric_limits<float>::quiet_NaN());
        envelope.setDecay(std::numeric_limits<float>::quiet_NaN());
        envelope.setSustain(std::numeric_limits<float>::quiet_NaN());
        envelope.setRelease(std::numeric_limits<float>::quiet_NaN());

        REQUIRE(envelope.getSampleRate() == Catch::Approx(44100.0));
        REQUIRE(envelope.getAttack() == Catch::Approx(0.01f));
        REQUIRE(envelope.getDecay() == Catch::Approx(0.2f));
        REQUIRE(envelope.getSustain() == Catch::Approx(0.7f));
        REQUIRE(envelope.getRelease() == Catch::Approx(0.5f));
    }
}

TEST_CASE("AdsrEnvelope performance benchmark", "[adsr][performance]")
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(48000.0);
    envelope.setAttack(0.01f);
    envelope.setDecay(0.05f);
    envelope.setSustain(0.6f);
    envelope.setRelease(0.2f);
    BENCHMARK("process one second of audio")
    {
        envelope.reset();
        envelope.noteOn();
        float sample = 0.0f;
        for (int i = 0; i < 48000; ++i)
            sample = envelope.getNextSample();
        envelope.noteOff();
        for (int i = 0; i < 48000; ++i)
            sample = envelope.getNextSample();
        return sample;
    };
}
