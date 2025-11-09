# S1-T2: ADSR-Envelope Implementation & Testing

**Sprint:** 1
**Aufwand:** 6 Stunden
**Priorität:** 2
**Abhängig von:** S1-T1 (Projekt-Setup)
**Stand:** 2025-11-08

---

## Ziel

Vollständige Implementierung einer ADSR-Envelope-Klasse (Attack, Decay, Sustain, Release) mit umfassenden Unit-Tests.

**Ergebnis:**
- ADSR-Envelope-Klasse mit konfigurierbaren Parametern
- Mindestens 80% Code-Coverage durch Unit-Tests
- Alle Edge-Cases getestet
- Dokumentierte API

---

## Technische Spezifikation

### 1. ADSR-Envelope Grundlagen

Eine ADSR-Envelope besteht aus vier Phasen:

```
Level
  1.0 ┤     ╱╲
      │    ╱  ╲___________  Sustain Level
      │   ╱                ╲
  0.0 ┤  ╱                  ╲___
      └──┴────┴────┴────┴────┴───> Time
         A    D    S    R
```

**Phasen:**
- **Attack (A):** Zeit von 0.0 bis 1.0 (Peak)
- **Decay (D):** Zeit von 1.0 bis Sustain-Level
- **Sustain (S):** Hält Sustain-Level, solange Note gehalten wird
- **Release (R):** Exponentieller Abfall mit Zeitkonstante τ vom aktuellen Level in Richtung 0.0 (nach noteOff)

### 2. Parameter-Spezifikation

| Parameter | Typ | Bereich | Einheit | Default | Beschreibung |
|-----------|-----|---------|---------|---------|--------------|
| **Attack** | `float` | 0.001 - 10.0 | Sekunden | 0.01 | Zeit bis Peak (1.0) - Linear |
| **Decay** | `float` | 0.001 - 10.0 | Sekunden | 0.2 | Zeit von Peak zu Sustain - Linear |
| **Sustain** | `float` | 0.0 - 1.0 | Level | 0.7 | Haltepegel |
| **Release** | `float` | 0.001 - 10.0 | Sekunden | 0.5 | **Zeitkonstante τ** (nicht Zeit-bis-Null!) - Exponentiell |
| **Sample Rate** | `double` | 8000.0 - 192000.0 | Hz | 44100.0 | Audio-Sample-Rate |

**Parameter-Validierung (Clamping-Strategie):**

Alle Setter verwenden **Silent Clamping** ohne Rückgabewerte oder Exceptions:

```cpp
void setAttack(float attackTimeSeconds)
{
    // NaN/Inf-Check: Nicht-finite Werte auf Default zurücksetzen
    if (!std::isfinite(attackTimeSeconds))
        attackTime_ = 0.01f;  // Default-Wert
    else
        attackTime_ = std::clamp(attackTimeSeconds, 0.001f, 10.0f);
}

void setDecay(float decayTimeSeconds)
{
    // NaN/Inf-Check: Nicht-finite Werte auf Default zurücksetzen
    if (!std::isfinite(decayTimeSeconds))
        decayTime_ = 0.2f;  // Default-Wert
    else
        decayTime_ = std::clamp(decayTimeSeconds, 0.001f, 10.0f);
}

void setSustain(float sustainLevel)
{
    // NaN/Inf-Check: Nicht-finite Werte auf Default zurücksetzen
    if (!std::isfinite(sustainLevel))
        sustainLevel_ = 0.7f;  // Default-Wert
    else
        sustainLevel_ = std::clamp(sustainLevel, 0.0f, 1.0f);
}

void setRelease(float releaseTimeSeconds)
{
    // NaN/Inf-Check: Nicht-finite Werte auf Default zurücksetzen
    if (!std::isfinite(releaseTimeSeconds))
        releaseTime_ = 0.5f;  // Default-Wert
    else
        releaseTime_ = std::clamp(releaseTimeSeconds, 0.001f, 10.0f);

    updateReleaseCoefficient();  // Release-Zeitkonstante aktuell halten
}

void setSampleRate(double sampleRate)
{
    // NaN/Inf-Check: Nicht-finite Werte auf Default zurücksetzen
    if (!std::isfinite(sampleRate))
        sampleRate_ = 44100.0;  // Default-Wert
    else
        sampleRate_ = std::clamp(sampleRate, 8000.0, 192000.0);

    updateReleaseCoefficient();  // τ muss Sample-Rate-Änderungen folgen
}
```

> **NaN/Inf-Behandlung:** Nicht-finite Eingaben (`NaN`, `±Inf`) werden durch `std::isfinite()` erkannt und auf die dokumentierten Default-Werte zurückgesetzt, damit Host-Fehler oder uninitialisierte Variablen nicht bis in den Audio-Thread durchschlagen. Dies verhindert undefiniertes Verhalten in Berechnungen.

**Rationale:**
- **Silent Clamping** statt Fehler: Audio-Plugins sollten nie crashen
- **Keine Rückgabewerte:** Vereinfacht Host-Integration (DAWs erwarten void-Setter)
- **Keine Exceptions:** Real-Time-Audio-Code vermeidet Exceptions
- **Keine Logging:** Envelope-Berechnungen sind Performance-kritisch

**Besonderheiten:**
- Sustain ist ein **Level** (0.0-1.0), keine Zeit
- Zeiten haben Minimum von 0.001s (1ms) um Division-by-Zero zu vermeiden
- Sample-Rate hat Minimum von 8000 Hz (professionelle Audio-Standards: 8-192 kHz)
- Release-Phase verwendet **exponentiellen Decay** mit Epsilon-Threshold (0.0001)

**Release-Charakteristik (WICHTIG):**

Der Release-Parameter ist eine **Zeitkonstante τ (Tau)**, NICHT die Zeit bis der Level 0 erreicht!

- Nach **τ** Sekunden: Level ist auf **~36.8%** (1/e) des Startlevels gefallen
- Nach **3τ** Sekunden: Level ist auf **~5%** gefallen
- Nach **5τ** Sekunden: Level ist auf **~0.67%** gefallen
- **Nie exakt 0:** Epsilon-Threshold (0.0001) beendet Release vorzeitig

**Mathematik:**
```
level(t) = level₀ · e^(-t/τ)

wobei:
  level₀ = Startlevel beim noteOff
  τ = releaseTime (Zeitkonstante)
  t = verstrichene Zeit seit noteOff
```

In der diskreten Implementierung bedeutet das, dass jedes Sample mit einem festen Faktor

```
releaseCoeff = exp(-1 / (τ · sampleRate))
```

multipliziert wird. Dadurch bleibt τ unabhängig von der Sample-Rate wirklich eine Zeitkonstante,
anstatt (wie bei linearem Subtrahieren) implizit an die Schrittweite gekoppelt zu sein.

Die Zeit bis zum Erreichen des Epsilon-Schwellwerts ergibt sich zu

```
t_idle = τ · ln(level₀ / epsilon)
```

Beispiel: level₀ = 0.7, τ = 0.1 s und epsilon = 0.0001 ⇒ t_idle ≈ 0.885 s. Tests müssen daher
mehrere Zeitkonstanten simulieren, um den Idle-State deterministisch zu erreichen.

**Rationale für exponentiellen Decay:**
- ✅ Natürlicher Klang (wie echte Instrumente)
- ✅ Schneller am Anfang, sanfter am Ende
- ✅ Standard in den meisten Synthesizern (analog und digital)
- ❌ **NICHT linear** (Attack/Decay sind linear für einfachere Implementierung)

> **Alternative:** Für "Zeit bis 0" würde man Linear-Decay verwenden: `decrement = currentLevel / (releaseTime * sampleRate)`. Die aktuelle Implementierung weicht davon ab und verwendet Exponential-Decay!

**Warum Attack/Decay linear statt exponentiell?**
- ✅ Einfachere Implementierung (kein std::exp() für Attack/Decay nötig)
- ✅ Vorhersagbare, berechenbare Zeiten (≈ `attackTime` Sekunden bei unveränderter Sample-Rate)
- ✅ Ausreichend für erste Implementation
- ⚠️ **Limitation:** Weniger natürlicher Klang als exponentielle Kurven
- 🔮 **Zukünftig:** Exponentielle Attack/Decay-Kurven (siehe "Nächste Schritte")

> **Hinweis:** Die reale Dauer ist durch `ceil(attackTime · sampleRate)` bzw. `ceil(decayTime · sampleRate)` Samples bestimmt und verändert sich sofort, wenn die Sample-Rate während einer Phase geändert wird. Angaben in Sekunden sind somit Näherungswerte.

### 3. Klassen-Design

**Header: `src/core/AdsrEnvelope.h`**

```cpp
#pragma once

namespace Core
{

class AdsrEnvelope
{
public:
    AdsrEnvelope()
    {
        updateReleaseCoefficient();  // Default-Parameter sofort nutzbar machen
    }
    ~AdsrEnvelope() = default;

    // Konfiguration
    void setSampleRate(double sampleRate);
    void setAttack(float attackTimeSeconds);
    void setDecay(float decayTimeSeconds);
    void setSustain(float sustainLevel);      // 0.0 - 1.0
    void setRelease(float releaseTimeSeconds);

    // Parameter-Abfrage (für Tests und GUI)
    float getAttack() const { return attackTime_; }
    float getDecay() const { return decayTime_; }
    float getSustain() const { return sustainLevel_; }
    float getRelease() const { return releaseTime_; }
    double getSampleRate() const { return sampleRate_; }

    // Steuerung (siehe "Retrigger-Verhalten" unten)
    void noteOn();
    void noteOff();
    void reset();  // ⚠️ Zurück zu Idle (hart) - WARNUNG: Kann Clicks erzeugen bei Level > 0!

    // Processing
    float getNextSample();
    bool isActive() const;
    float getCurrentLevel() const { return currentLevel_; }  // Für Tests

private:
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

    // Parameter (in Sekunden bzw. Level)
    float attackTime_ = 0.01f;
    float decayTime_ = 0.2f;
    float sustainLevel_ = 0.7f;
    float releaseTime_ = 0.5f;
    float releaseCoeff_ = 1.0f;  // Cached exp(-1/(τ·SR)) für Performance

    void updateReleaseCoefficient();
};

} // namespace Core
```

### 4. Retrigger-Verhalten (noteOn/noteOff-Semantik)

**WICHTIG:** Die folgenden Regeln definieren das Verhalten bei mehrfachen noteOn/noteOff-Calls.

#### 4.1 noteOn() - Retrigger-Modus

**Verhalten:** noteOn() **resettet die Envelope** und startet Attack **von Level 0** neu.

```cpp
void AdsrEnvelope::noteOn()
{
    // HARD RESET: Level immer auf 0, State immer auf Attack
    currentLevel_ = 0.0f;
    state_ = State::Attack;
}
```

**Szenarien:**

| Aktueller State | noteOn()-Effekt | Neuer Level | Neuer State |
|----------------|-----------------|-------------|-------------|
| Idle | Start Attack | 0.0 | Attack |
| Attack | **Reset Attack** | **0.0** (zurückgesetzt!) | Attack |
| Decay | Reset Attack | 0.0 | Attack |
| Sustain | Reset Attack | 0.0 | Attack |
| Release | Reset Attack | 0.0 | Attack |

**Rationale:**
- ✅ Einfachste Implementierung
- ✅ Vorhersagbares Verhalten
- ⚠️ Kann Clicks erzeugen, weil der Level hart auf 0.0 springt
- ❌ Nicht ideal für legato (Alternative: soft-retrigger von currentLevel)

> **Alternative (NICHT implementiert):** "Soft Retrigger" würde Attack von `currentLevel_` statt von 0.0 starten. Dies erfordert komplexere Logik und kann Clicks erzeugen.

#### 4.2 noteOff() - Release-Trigger

**Verhalten:** noteOff() wechselt zu Release, **außer** wenn bereits Idle.

```cpp
void AdsrEnvelope::noteOff()
{
    if (state_ != State::Idle)
        state_ = State::Release;
    // currentLevel_ bleibt erhalten!
}
```

**Szenarien:**

| Aktueller State | noteOff()-Effekt | Level-Änderung | Neuer State |
|----------------|------------------|----------------|-------------|
| Idle | **Ignoriert** | Keine | Idle |
| Attack | → Release | **Keine** (behält aktuellen Level) | Release |
| Decay | → Release | Keine | Release |
| Sustain | → Release | Keine | Release |
| Release | **Ignoriert** | Keine | Release |

**Wichtig:**
- noteOff während Attack/Decay: **Keine Clicks**, da Level erhalten bleibt
- noteOff kann mehrfach aufgerufen werden (idempotent in Release)
- noteOff ohne vorheriges noteOn: Keine Wirkung (Idle bleibt Idle)

#### 4.3 reset() - Hard Stop

**Verhalten:** Sofortiger Stop, unabhängig vom State.

```cpp
void AdsrEnvelope::reset()
{
    state_ = State::Idle;
    currentLevel_ = 0.0f;
}
```

**Verwendung:**
- Voice-Stealing (Stimme wird hart beendet)
- All-Notes-Off (Panic-Button)
- **⚠️ WARNUNG:** Kann Clicks erzeugen, wenn Level > 0!
- **Best Practice:** Nur in Ausnahmesituationen verwenden (Voice Stealing, Panic)
- **Alternative:** Für normale Stop-Operationen immer `noteOff()` verwenden

### 5. Implementierungs-Details

**Attack-Phase (Linear):**
```cpp
float increment = 1.0f / (attackTime_ * sampleRate_);
currentLevel_ += increment;
if (currentLevel_ >= 1.0f)
{
    currentLevel_ = 1.0f;
    state_ = State::Decay;
}
```

> **Floating-Point-Hinweis:** Bei langen Attack-Zeiten (z.B. 10s @ 44.1kHz = 441000 Samples)
> kann sich Rundungsfehler akkumulieren. Tests verwenden daher `Approx().margin(0.01f)` (1% Toleranz).

**Decay-Phase (Linear):**
```cpp
float decrement = (1.0f - sustainLevel_) / (decayTime_ * sampleRate_);
currentLevel_ -= decrement;
if (currentLevel_ <= sustainLevel_)
{
    currentLevel_ = sustainLevel_;
    state_ = State::Sustain;
}
```

> **Edge-Case `sustain=1.0`:** Wenn `sustainLevel_ == 1.0f`, wird `decrement = 0`.
> Implementierung sollte diesen Fall optimieren (direkt zu Sustain springen nach Attack).

**Sustain-Phase:**
```cpp
// Hält currentLevel_ konstant
return currentLevel_;
```

**Reset-Methode:**
```cpp
void AdsrEnvelope::reset()
{
    state_ = State::Idle;
    currentLevel_ = 0.0f;
}
```

> **Verwendung:** `reset()` erzwingt sofortigen Übergang zu Idle, unabhängig vom aktuellen State. Nützlich für Hard-Stops oder beim Löschen von Stimmen im Voice-Management.

**Release-Phase (Exponentiell mit Epsilon):**
```cpp
// ⚠️ NAIVE VERSION (NUR ZUR ILLUSTRATION - NICHT SO IMPLEMENTIEREN!)
// Problem: std::exp() wird pro Sample aufgerufen → Performance-Killer!
const float releaseCoeff = std::exp(-1.0f / (releaseTime_ * static_cast<float>(sampleRate_)));
currentLevel_ *= releaseCoeff;

// Epsilon-Threshold für Floating-Point-Präzision
constexpr float epsilon = 0.0001f;
if (currentLevel_ <= epsilon)
{
    currentLevel_ = 0.0f;
    state_ = State::Idle;
}
```

> **✅ TATSÄCHLICHE Implementierung (siehe Klassen-Design Zeile 208):**
> ```cpp
> // In der Klasse: float releaseCoeff_;
>
> // Bei setSampleRate() oder setRelease():
> void updateReleaseCoefficient()
> {
>     releaseCoeff_ = std::exp(-1.0f / (releaseTime_ * static_cast<float>(sampleRate_)));
> }
>
> // In getNextSample() während Release-Phase:
> currentLevel_ *= releaseCoeff_;  // Gecachter Wert - kein std::exp() pro Sample!
> ```
>
> **Wichtig:** `releaseCoeff_` MUSS neu berechnet werden bei:
> - `setSampleRate()` wird aufgerufen
> - `setRelease()` wird aufgerufen
>
> Der Konstruktor ruft `updateReleaseCoefficient()` einmalig auf, damit die Default-Konfiguration sofort korrekt abfällt. Andernfalls ändert sich die Zeitkonstante τ unbeabsichtigt!

> **Wichtig:** Die Release-Phase verwendet exponentiellen Decay. Da dieser mathematisch nie exakt 0.0 erreicht, verwenden wir einen Epsilon-Schwellwert (0.0001), um in den Idle-State zurückzukehren.

---


### 6. Thread-Safety & Echtzeit-Richtlinien

**⚠️ NICHT THREAD-SAFE:**

Die `AdsrEnvelope`-Klasse bietet **KEINE** interne Synchronisation (keine Locks, keine Atomics). Der Host **MUSS** sicherstellen, dass:

1. **Keine gleichzeitigen Zugriffe aus mehreren Threads:**
   - `getNextSample()` wird IMMER nur im Audio-Thread aufgerufen
   - Parameter-Setter (`setAttack()`, etc.) werden ENTWEDER:
     - Nur im Audio-Thread aufgerufen ODER
     - Nur vor dem Start des Audio-Threads aufgerufen ODER
     - Über einen Thread-sicheren Übergabe-Mechanismus synchronisiert (siehe unten)

2. **Gleichzeitiges Lesen/Schreiben ist undefiniertes Verhalten:**
   - `setSampleRate()` während `getNextSample()` → **UB** (Data Race)
   - `setRelease()` während `getNextSample()` → **UB** (Data Race)

**Empfohlenes Pattern für GUI → Audio-Thread Parameter-Übergabe:**

```cpp
// Im Host-Code (außerhalb von AdsrEnvelope):

// GUI-Thread schreibt neue Parameter:
std::atomic<float> pendingAttackTime{0.1f};
std::atomic<float> pendingDecayTime{0.2f};
std::atomic<float> pendingSustainLevel{0.7f};
std::atomic<float> pendingReleaseTime{0.5f};

// Audio-Thread liest Parameter VOR dem Processing-Block:
void processAudioBlock(AudioBuffer& buffer)
{
    // Parameter EINMAL pro Block lesen (nicht pro Sample!)
    float attack = pendingAttackTime.load(std::memory_order_relaxed);
    float decay = pendingDecayTime.load(std::memory_order_relaxed);
    float sustain = pendingSustainLevel.load(std::memory_order_relaxed);
    float release = pendingReleaseTime.load(std::memory_order_relaxed);

    // JETZT sicher in AdsrEnvelope übernehmen (kein anderer Thread greift zu)
    envelope.setAttack(attack);
    envelope.setDecay(decay);
    envelope.setSustain(sustain);
    envelope.setRelease(release);

    // Processing
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        buffer.setSample(0, i, envelope.getNextSample());
}
```

**Alternative: JUCE AudioProcessorValueTreeState (APVTS):**
JUCE bietet Thread-sichere Parameter-Synchronisation via `AudioProcessorValueTreeState`, die automatisch GUI ↔ Audio-Thread Übergabe handhabt.

**Echtzeit-Sicherheit:**
- ✅ **Alle Methoden sind Echtzeit-sicher:** Keine Allocations, keine Locks, keine System-Calls
- ✅ **Lock-free:** Parameter-Setter verwenden nur einfache Zuweisungen
- ⚠️ **std::exp() in `updateReleaseCoefficient()`:** Wird nur bei Parameter-Änderungen aufgerufen, NICHT pro Sample
- ⚠️ **std::isfinite() in Settern:** Potentiell nicht deterministisch (FPU-Status), aber akzeptabel für Robustheit

**Wichtig:** Nicht-finite Werte (`NaN`, `±Inf`) werden intern auf Default-Werte zurückgesetzt, um Fehlkonfigurationen aus UI-Layern oder uninitialisierte Parameter-Übergaben deterministisch zu entschärfen.

---

## Test-Spezifikation

### 1. Test-Framework

- **Framework:** Catch2 v3.11.0
- **Ziel-Coverage:** ≥ 80%
- **Test-Dateien:** `tests/AdsrTests.cpp`

### 2. Test-Cases

#### TC1: Initialisierung & State
```cpp
TEST_CASE("AdsrEnvelope initial state", "[adsr]")
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(44100.0);

    SECTION("Initial state is idle")
    {
        REQUIRE_FALSE(envelope.isActive());
        REQUIRE(envelope.getCurrentLevel() == 0.0f);  // Peek ohne State-Änderung

        // Verify getNextSample returns 0 in Idle
        float sample = envelope.getNextSample();
        REQUIRE(sample == 0.0f);
        REQUIRE_FALSE(envelope.isActive());  // Bleibt Idle
    }

    SECTION("Note on activates envelope")
    {
        envelope.noteOn();
        REQUIRE(envelope.isActive());

        // Envelope sollte > 0 nach erstem Sample sein
        float sample = envelope.getNextSample();
        REQUIRE(sample > 0.0f);
    }
}
```

#### TC2: Attack-Phase
```cpp
TEST_CASE("ADSR Attack phase", "[adsr]")
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(44100.0);
    envelope.setAttack(0.1f);  // 100ms

    SECTION("Attack increases level linearly")
    {
        envelope.noteOn();
        float prev = 0.0f;
        bool peakReached = false;

        // Sample über Attack-Phase
        // Nach 4410 Samples (100ms @ 44.1kHz) sollte Peak erreicht sein
        for (int i = 0; i < 4410; ++i)
        {
            float current = envelope.getNextSample();

            if (current >= 1.0f)
            {
                peakReached = true;
                break;  // Peak erreicht, State wechselt zu Decay
            }

            REQUIRE(current > prev);  // Monoton steigend während Attack
            prev = current;
        }

        REQUIRE(peakReached);  // Peak muss während der Schleife erreicht worden sein

        // Nach Peak: State ist Decay (oder Sustain, falls Decay sehr kurz)
        // Level kann bereits unter 1.0 sein
        float afterPeak = envelope.getCurrentLevel();
        REQUIRE(afterPeak <= 1.0f);
        REQUIRE(afterPeak > 0.0f);
    }

    SECTION("Attack reaches exactly 1.0 at peak")
    {
        envelope.setDecay(10.0f);  // Sehr lang, um Decay-Übergang zu vermeiden
        envelope.noteOn();

        // Sample bis kurz vor Ende der Attack-Phase
        int attackSamples = static_cast<int>(0.1f * 44100.0f);
        for (int i = 0; i < attackSamples - 1; ++i)
            envelope.getNextSample();

        // Letztes Attack-Sample sollte Peak sein
        // Hinweis: 1% Toleranz (0.01) für Floating-Point-Akkumulation über 4410 Samples
        float lastAttack = envelope.getNextSample();
        REQUIRE(lastAttack == Approx(1.0f).margin(0.01f));
    }
}
```

#### TC3: Decay-Phase
```cpp
TEST_CASE("ADSR Decay phase", "[adsr]")
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(44100.0);
    envelope.setAttack(0.001f);  // Sehr kurz
    envelope.setDecay(0.1f);
    envelope.setSustain(0.5f);

    SECTION("Decay reaches sustain level")
    {
        envelope.noteOn();

        // Attack überspringen
        for (int i = 0; i < 100; ++i)
            envelope.getNextSample();

        // Decay-Phase
        for (int i = 0; i < 5000; ++i)
            envelope.getNextSample();

        // Sustain erreicht
        // Hinweis: 5% Toleranz (0.05) - größer als Attack wegen längerer Decay-Zeit
        float level = envelope.getNextSample();
        REQUIRE(level == Approx(0.5f).margin(0.05f));
    }
}
```

#### TC4: Sustain-Phase
```cpp
TEST_CASE("ADSR Sustain phase", "[adsr]")
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(44100.0);
    envelope.setAttack(0.001f);
    envelope.setDecay(0.001f);
    envelope.setSustain(0.7f);

    SECTION("Sustain level is maintained")
    {
        envelope.noteOn();

        // Zu Sustain springen
        for (int i = 0; i < 500; ++i)
            envelope.getNextSample();

        // Sustain sollte konstant sein
        float level1 = envelope.getNextSample();
        float level2 = envelope.getNextSample();
        float level3 = envelope.getNextSample();

        REQUIRE(level1 == Approx(0.7f).margin(0.1f));
        REQUIRE(level1 == level2);
        REQUIRE(level2 == level3);
    }
}
```

#### TC5: Release-Phase
```cpp
TEST_CASE("ADSR Release phase", "[adsr]")
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(44100.0);
    envelope.setAttack(0.001f);
    envelope.setDecay(0.001f);
    envelope.setSustain(0.7f);
    envelope.setRelease(0.1f);

    SECTION("Release decreases level to zero")
    {
        envelope.noteOn();

        // Zu Sustain
        for (int i = 0; i < 500; ++i)
            envelope.getNextSample();

        envelope.noteOff();

        float before = envelope.getNextSample();
        float after = envelope.getNextSample();

        REQUIRE(after < before);  // Fallend
    }

    SECTION("Envelope returns to idle after release")
    {
        envelope.noteOn();
        for (int i = 0; i < 500; ++i)
            envelope.getNextSample();

        envelope.noteOff();

        // Faktor 10: e^-10 ≈ 4.5e-5 < epsilon (0.0001) → garantiert Idle-Erreichen
        // Bei releaseTime=0.1s und SR=44100: maxSamples = 44100 Samples ≈ 1 Sekunde
        const int maxSamples = static_cast<int>(envelope.getRelease() * envelope.getSampleRate() * 10.0f);
        bool becameIdle = false;

        for (int i = 0; i < maxSamples; ++i)
        {
            envelope.getNextSample();
            if (!envelope.isActive())
            {
                becameIdle = true;
                break;
            }
        }

        REQUIRE(becameIdle);
        REQUIRE(envelope.getNextSample() == 0.0f);
    }
}
```

> **Hinweis:** Der Faktor `10 · τ` stellt sicher, dass der Pegel unabhängig vom Startlevel sicher
> unter den Epsilon-Schwellwert fällt (`e^-10 ≈ 4.5e-5`). Damit bleibt der Test robust gegenüber
> Änderungen der Release-Zeit, solange τ korrekt als Zeitkonstante interpretiert wird.

#### TC6: Parameter-Validierung
```cpp
TEST_CASE("ADSR Parameter validation", "[adsr]")
{
    AdsrEnvelope envelope;

    SECTION("Sustain level is clamped to [0, 1]")
    {
        // Test unterer Grenzwert
        envelope.setSustain(-0.5f);
        REQUIRE(envelope.getSustain() == 0.0f);

        // Test oberer Grenzwert
        envelope.setSustain(1.5f);
        REQUIRE(envelope.getSustain() == 1.0f);

        // Test gültiger Wert
        envelope.setSustain(0.7f);
        REQUIRE(envelope.getSustain() == 0.7f);
    }

    SECTION("Time parameters have minimum values")
    {
        // Attack
        envelope.setAttack(0.0f);
        REQUIRE(envelope.getAttack() == 0.001f);

        envelope.setAttack(-1.0f);
        REQUIRE(envelope.getAttack() == 0.001f);

        // Decay
        envelope.setDecay(0.0f);
        REQUIRE(envelope.getDecay() == 0.001f);

        // Release
        envelope.setRelease(0.0f);
        REQUIRE(envelope.getRelease() == 0.001f);
    }

    SECTION("Time parameters have maximum values")
    {
        envelope.setAttack(100.0f);
        REQUIRE(envelope.getAttack() == 10.0f);

        envelope.setDecay(50.0f);
        REQUIRE(envelope.getDecay() == 10.0f);

        envelope.setRelease(999.0f);
        REQUIRE(envelope.getRelease() == 10.0f);
    }

    SECTION("Sample rate is clamped to valid range")
    {
        // Zu niedrig
        envelope.setSampleRate(1000.0);
        REQUIRE(envelope.getSampleRate() == 8000.0);

        // Zu hoch
        envelope.setSampleRate(500000.0);
        REQUIRE(envelope.getSampleRate() == 192000.0);

        // Gültig
        envelope.setSampleRate(48000.0);
        REQUIRE(envelope.getSampleRate() == 48000.0);
    }
}
```

#### TC7: Edge-Cases
```cpp
TEST_CASE("ADSR Edge cases", "[adsr]")
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(44100.0);

    SECTION("Note off during attack")
    {
        envelope.setAttack(1.0f);  // Lang (1 Sekunde)
        envelope.setRelease(0.1f);
        envelope.noteOn();

        // Fortschritt in Attack-Phase
        for (int i = 0; i < 100; ++i)
            envelope.getNextSample();

        float levelBeforeOff = envelope.getCurrentLevel();
        REQUIRE(levelBeforeOff > 0.0f);
        REQUIRE(levelBeforeOff < 1.0f);  // Noch nicht am Peak

        envelope.noteOff();  // Während Attack

        // Sollte in Release übergehen
        REQUIRE(envelope.isActive());

        // Level sollte jetzt fallen
        float levelAfterOff1 = envelope.getNextSample();
        float levelAfterOff2 = envelope.getNextSample();

        REQUIRE(levelAfterOff2 < levelAfterOff1);  // Fallend
    }

    SECTION("Multiple noteOn calls restart attack")
    {
        envelope.setAttack(0.1f);
        envelope.noteOn();

        // Fortschritt in Attack
        for (int i = 0; i < 1000; ++i)
            envelope.getNextSample();

        float levelMid = envelope.getCurrentLevel();
        REQUIRE(levelMid > 0.0f);

        // Zweites noteOn sollte Attack neu starten
        envelope.noteOn();

        // Level sollte zurückgesetzt werden
        float levelAfterRestart = envelope.getNextSample();
        REQUIRE(levelAfterRestart < levelMid);  // Zurück auf niedrigeren Wert
        REQUIRE(envelope.isActive());
    }

    SECTION("NoteOff without noteOn does nothing")
    {
        envelope.noteOff();  // Ohne vorheriges noteOn

        // Sollte Idle bleiben
        REQUIRE_FALSE(envelope.isActive());
        REQUIRE(envelope.getCurrentLevel() == 0.0f);

        // Mehrfache noteOff-Calls
        envelope.noteOff();
        envelope.noteOff();

        REQUIRE_FALSE(envelope.isActive());
    }

    SECTION("setSampleRate during active envelope")
    {
        envelope.setAttack(0.1f);
        envelope.setSampleRate(44100.0);
        envelope.noteOn();

        // Fortschritt
        for (int i = 0; i < 100; ++i)
            envelope.getNextSample();

        float levelBefore = envelope.getCurrentLevel();

        // Sample-Rate ändern
        envelope.setSampleRate(48000.0);

        // Envelope sollte weiter funktionieren
        REQUIRE(envelope.isActive());

        float levelAfter = envelope.getNextSample();
        REQUIRE(levelAfter >= levelBefore);  // Sollte weiter steigen (Attack)
    }

    SECTION("setSampleRate during Release updates releaseCoeff")
    {
        envelope.setAttack(0.001f);
        envelope.setDecay(0.001f);
        envelope.setSustain(0.7f);
        envelope.setRelease(0.1f);
        envelope.setSampleRate(44100.0);

        envelope.noteOn();

        // Zu Sustain springen
        for (int i = 0; i < 500; ++i)
            envelope.getNextSample();

        envelope.noteOff();

        // Ein paar Release-Samples
        for (int i = 0; i < 10; ++i)
            envelope.getNextSample();

        float levelBefore = envelope.getCurrentLevel();

        // Sample-Rate ändern während Release
        envelope.setSampleRate(48000.0);

        // Level sollte weiter fallen (releaseCoeff wurde neu berechnet)
        float sample1 = envelope.getNextSample();
        float sample2 = envelope.getNextSample();

        REQUIRE(envelope.isActive());
        REQUIRE(sample2 < sample1);  // Weiterhin fallend
        REQUIRE(sample1 < levelBefore);
    }

    SECTION("Sustain level = 1.0 skips decay phase")
    {
        envelope.setAttack(0.01f);
        envelope.setDecay(0.5f);  // Lang, aber sollte übersprungen werden
        envelope.setSustain(1.0f);
        envelope.setSampleRate(44100.0);

        envelope.noteOn();

        // Attack-Phase durchlaufen
        int attackSamples = static_cast<int>(0.01f * 44100.0f) + 100;
        for (int i = 0; i < attackSamples; ++i)
            envelope.getNextSample();

        // Sollte bei 1.0 bleiben (Sustain), nicht fallen (Decay)
        float level1 = envelope.getNextSample();
        float level2 = envelope.getNextSample();
        float level3 = envelope.getNextSample();

        REQUIRE(level1 == Approx(1.0f).margin(0.01f));
        REQUIRE(level2 == Approx(1.0f).margin(0.01f));
        REQUIRE(level3 == Approx(1.0f).margin(0.01f));
        REQUIRE(envelope.isActive());
    }

    SECTION("reset() forces idle state")
    {
        envelope.noteOn();
        envelope.getNextSample();

        REQUIRE(envelope.isActive());

        envelope.reset();

        REQUIRE_FALSE(envelope.isActive());
        REQUIRE(envelope.getCurrentLevel() == 0.0f);
    }
}
```

#### TC8: Release-Zeitkonstante während Release ändern
```cpp
SECTION("setRelease during release recalculates coefficient")
{
    // Release läuft bereits
    float slowDelta = before - beforeNext;
    envelope.setRelease(0.01f);
    float fastDelta = after - afterNext;
    REQUIRE(fastDelta > slowDelta * 2.0f);
}
```
Überprüft, dass `updateReleaseCoefficient()` sofort greift und der Pegel monotonic weiter fällt.

#### TC9: Sustain-Level = 0.0
```cpp
SECTION("Sustain level zero holds silence")
{
    envelope.setSustain(0.0f);
    // Nach Attack/Decay ist currentLevel ≈ 0 und bleibt es bis noteOff()
}
```
Stellt sicher, dass Decay deterministisch bis 0.0 läuft und Sustain keine negativen Werte erzeugt.

#### TC10: NaN/Inf-Inputs
```cpp
TEST_CASE("ADSR NaN/Inf input handling", "[adsr]")
{
    AdsrEnvelope envelope;

    SECTION("NaN inputs fall back to defaults")
    {
        envelope.setAttack(std::numeric_limits<float>::quiet_NaN());
        REQUIRE(envelope.getAttack() == Approx(0.01f));

        envelope.setDecay(std::numeric_limits<float>::quiet_NaN());
        REQUIRE(envelope.getDecay() == Approx(0.2f));

        envelope.setSustain(std::numeric_limits<float>::quiet_NaN());
        REQUIRE(envelope.getSustain() == Approx(0.7f));

        envelope.setRelease(std::numeric_limits<float>::quiet_NaN());
        REQUIRE(envelope.getRelease() == Approx(0.5f));

        envelope.setSampleRate(std::numeric_limits<double>::quiet_NaN());
        REQUIRE(envelope.getSampleRate() == Approx(44100.0));
    }

    SECTION("Infinity inputs fall back to defaults")
    {
        envelope.setAttack(std::numeric_limits<float>::infinity());
        REQUIRE(envelope.getAttack() == Approx(0.01f));

        envelope.setDecay(-std::numeric_limits<float>::infinity());
        REQUIRE(envelope.getDecay() == Approx(0.2f));

        envelope.setSustain(std::numeric_limits<float>::infinity());
        REQUIRE(envelope.getSustain() == Approx(0.7f));

        envelope.setRelease(std::numeric_limits<float>::infinity());
        REQUIRE(envelope.getRelease() == Approx(0.5f));

        envelope.setSampleRate(std::numeric_limits<double>::infinity());
        REQUIRE(envelope.getSampleRate() == Approx(44100.0));
    }

    SECTION("Envelope remains functional after NaN input")
    {
        envelope.setAttack(std::numeric_limits<float>::quiet_NaN());
        envelope.setSampleRate(44100.0);
        envelope.noteOn();

        // Sollte normal funktionieren (mit Default-Attack-Zeit)
        float sample1 = envelope.getNextSample();
        float sample2 = envelope.getNextSample();

        REQUIRE(sample2 > sample1);  // Level steigt
        REQUIRE(envelope.isActive());
    }
}
```
Tests für resiliente Parameter-Setter, die nicht-finite Eingaben (`NaN`, `±Inf`) auf Default-Werte zurücksetzen. Stellt sicher, dass die Envelope auch nach ungültigen Eingaben weiterhin funktionsfähig bleibt.

#### TC11: Performance-Benchmark
```cpp
TEST_CASE("AdsrEnvelope performance benchmark", "[adsr][performance]")
{
    BENCHMARK("process one second of audio")
    {
        envelope.reset();
        envelope.noteOn();
        // 48k Samples Attack/Sustain + Release
    };
}
```
Catch2-Benchmark, der grob zwei Sekunden Audio-Verarbeitung misst und Performance-Regressionen aufdeckt (kein klassisches Pass/Fail, aber Build bricht bei extrem langsamer Ausführung).

### 3. Coverage-Ziel & Messplan

**Mindest-Coverage: 80%**

#### 3.1 Scope

**Zu messende Dateien:**
- `src/core/AdsrEnvelope.cpp` (Haupt-Implementierung)
- `src/core/AdsrEnvelope.h` (Inline-Methoden)

**⚠️ Einschränkung bei Inline-Methoden:**
- Inline-Getter (z.B. `getAttack()`, `getSustain()`) werden von gcov oft nicht erfasst
- Dies ist ein bekanntes Limit von Coverage-Tools bei Header-Only-Code
- **Lösung:** Inline-Methoden werden durch Tests indirekt validiert (Parameter-Validierung Tests)
- **Coverage-Bewertung:** Fokus auf `.cpp`-Datei, Header-Inline-Code gilt als implizit getestet

**Ausgeschlossen:**
- Test-Dateien selbst (`tests/AdsrTests.cpp`)
- JUCE-Framework-Code
- Externe Dependencies

#### 3.2 Coverage-Kategorien

**Line Coverage (Zeilen-Coverage):**
- Ziel: ≥ 80%
- Jede ausführbare Code-Zeile mindestens einmal durchlaufen

**Branch Coverage (Zweig-Coverage):**
- Ziel: ≥ 75%
- Alle if/else-Zweige und switch-cases getestet
- Besonders wichtig: State-Machine-Transitions

**Function Coverage:**
- Ziel: 100%
- Alle public und private Methoden aufgerufen

#### 3.3 Zu deckende Bereiche

- ✅ **Alle State-Übergänge:**
  - Idle → Attack (via noteOn)
  - Attack → Decay (Peak erreicht)
  - Decay → Sustain (Sustain-Level erreicht)
  - Sustain → Release (via noteOff)
  - Release → Idle (Epsilon-Threshold)
  - Attack → Release (noteOff während Attack)

- ✅ **Parameter-Clamping:**
  - Attack: [0.001, 10.0]
  - Decay: [0.001, 10.0]
  - Sustain: [0.0, 1.0]
  - Release: [0.001, 10.0]
  - Sample Rate: [8000.0, 192000.0]

- ✅ **Edge-Cases:**
  - noteOff während Attack/Decay
  - Mehrfache noteOn-Calls
  - noteOff ohne noteOn
  - setSampleRate während aktivem Envelope
  - reset() während beliebiger Phase
- ✅ **Release-Parameter-Änderungen während Release**
- ✅ **Sustain = 0.0**
- ✅ **NaN/Inf-Inputs**

- ✅ **Epsilon-Threshold in Release-Phase**
- ✅ **Sample-Rate-Abhängigkeit**

#### 3.4 Floating-Point-Toleranz-Policy

- Attack-Phase: `Approx(...).margin(0.01f)` (≈1%) wegen hoher Sample-Anzahl.
- Decay/Sustain: max. 5% relative Toleranz (`margin(0.05f)`), da abhängig von `sustainLevel`.
- Release: Vergleiche über Ratios/Delta-Verhältnisse statt absolute Werte, um Sample-Rate-Änderungen zu tolerieren.
- Zero-Checks: `margin(0.001f)` zur Abdeckung von Floating-Point-Residuen.

#### 3.5 Messung mit gcov/lcov

**Schritt 1: Build mit Coverage-Flags**
```bash
# CMake mit Coverage-Support konfigurieren
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage"

# Build
cmake --build build --target AnalogSynthTests
```

**Schritt 2: Tests ausführen**
```bash
cd build
ctest --output-on-failure --verbose

# Oder direkt:
./AnalogSynthTests
```

**Schritt 3: Coverage-Daten sammeln**
```bash
# gcov ausführen (im build-Verzeichnis)
cd build

# Coverage für AdsrEnvelope.cpp generieren
gcov CMakeFiles/AnalogSynthTests.dir/src/core/AdsrEnvelope.cpp.gcda

# Zeigt Line-Coverage an:
# Lines executed: XX.XX% of YY
```

**Schritt 4: HTML-Report generieren (optional)**
```bash
# lcov installieren (falls nicht vorhanden)
sudo apt-get install lcov

# Coverage-Daten sammeln
lcov --capture \
     --directory . \
     --output-file coverage.info \
     --no-external

# Filter nur AdsrEnvelope
lcov --extract coverage.info \
     '*/src/core/AdsrEnvelope.*' \
     --output-file coverage_filtered.info

# HTML-Report generieren
genhtml coverage_filtered.info \
        --output-directory coverage_report \
        --title "ADSR Envelope Coverage"

# Report öffnen
firefox coverage_report/index.html
```

**Schritt 5: Coverage verifizieren**
```bash
# Minimale Coverage prüfen
lcov --summary coverage_filtered.info

# Erwartete Ausgabe:
#   lines......: 80.0% (XX of YY lines)
#   functions..: 100.0% (X of X functions)
#   branches...: 75.0% (XX of YY branches)
```

#### 3.5 Akzeptanzkriterien

**PASS:** Test-Suite gilt als bestanden, wenn:
- ✅ Line Coverage ≥ 80%
- ✅ Function Coverage = 100%
- ✅ Branch Coverage ≥ 75%
- ✅ Alle Tests bestehen (0 failures)

**FAIL:** Wenn eine der Coverage-Metriken unter dem Schwellwert liegt

#### 3.6 CI-Integration (zukünftig)

```yaml
# .github/workflows/coverage.yml
- name: Generate Coverage
  run: |
    cmake -B build -DCMAKE_CXX_FLAGS="--coverage"
    cmake --build build --target AnalogSynthTests
    cd build && ctest

- name: Upload to Codecov
  uses: codecov/codecov-action@v3
  with:
    files: ./build/coverage.info
```

---

## Implementierungs-Schritte

### Phase 0: CMake-Integration (15 Min)
1. ✅ `CMakeLists.txt` anpassen (siehe S1-T1 für Struktur)
2. ✅ `src/core/AdsrEnvelope.cpp` zu `SOURCES` hinzufügen
3. ✅ Namespace `Core::` in CMake berücksichtigen

**CMake-Änderungen:**
```cmake
# In CMakeLists.txt (Root-Level)
set(SOURCES
    # ... bestehende Dateien ...
    src/core/AdsrEnvelope.cpp  # ← Neu hinzufügen
)

# Header werden automatisch gefunden (include_directories bereits gesetzt)
```

**Verifizierung:**
```bash
cmake --build build --target AnalogSynthTests
# Sollte AdsrEnvelope.cpp kompilieren
```

### Phase 1: Core Implementation (2h)
1. ✅ Header-Datei erstellen (`src/core/AdsrEnvelope.h`)
2. ✅ Implementation (`src/core/AdsrEnvelope.cpp`)
3. ✅ Parameter-Setter mit Clamping
4. ✅ State-Machine für ADSR-Phasen
5. ✅ getNextSample() mit allen Phasen
6. ✅ `releaseCoeff_` Caching-Logik in `setSampleRate()` und `setRelease()`

### Phase 2: Unit-Tests (3h)
1. ✅ Test-Datei anlegen (`tests/AdsrTests.cpp`)
2. ✅ Basis-Tests (TC1-TC3)
3. ✅ Erweiterte Tests (TC4-TC5)
4. ✅ Parameter-Validierung (TC6)
5. ✅ Edge-Cases (TC7)

### Phase 3: Bug-Fixing & Optimierung (1h)
1. ✅ Epsilon-Threshold für Release-Phase
2. ✅ Alle Tests zum Laufen bringen
3. ✅ Coverage-Report generieren
4. ✅ Floating-Point-Präzision in Tests berücksichtigen
5. ✅ Edge-Case `sustain=1.0` behandeln (Decay-Skip-Logik)
6. Performance-Profiling (falls nötig)

---

## Validierung

### Build & Test
```bash
# Build
cmake --build build --target AnalogSynthTests

# Tests ausführen
cd build && ctest --output-on-failure --verbose

# Einzelnen Test ausführen
./AnalogSynthTests "[adsr]"
```

### Coverage-Report (Optional)
```bash
# Mit gcov/lcov
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build
cd build && ctest
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

### Erwartete Ergebnisse
```
===============================================================================
All tests passed (XX assertions in Y test cases)
```

---

## Definition of Done (S1-T2)

- ✅ `AdsrEnvelope.h` und `AdsrEnvelope.cpp` implementiert
- ✅ Alle ADSR-Phasen funktionieren korrekt
- ✅ Parameter-Validierung (Clamping) implementiert
- ✅ `AdsrTests.cpp` mit mindestens 7 Test-Cases
- ✅ Alle Tests bestehen (100% pass rate)
- ✅ Code-Coverage ≥ 80%
- ✅ Epsilon-Threshold für Release-Phase implementiert
- ✅ Dokumentation in Header-Kommentaren
- [ ] Code-Review durchgeführt
- ✅ Keine Compiler-Warnungen

---

## Bekannte Probleme & Lösungen

### Problem 1: Release-Phase erreicht nie Idle
**Symptom:** `isActive()` gibt nach Release immer noch `true` zurück

**Ursache:** Exponentieller Decay erreicht mathematisch nie exakt 0.0

**Lösung:** Epsilon-Threshold (0.0001) einführen
```cpp
constexpr float epsilon = 0.0001f;
if (currentLevel_ <= epsilon)
{
    currentLevel_ = 0.0f;
    state_ = State::Idle;
}
```

### Problem 2: Division-by-Zero bei attackTime = 0
**Symptom:** Crash oder inf-Werte

**Ursache:** `increment = 1.0f / (attackTime_ * sampleRate_)` mit attackTime = 0

**Lösung:** Clamping auf gültigen Bereich (konsistent mit Spezifikation)
```cpp
void AdsrEnvelope::setAttack(float attackTimeSeconds)
{
    attackTime_ = std::clamp(attackTimeSeconds, 0.001f, 10.0f);
}
```

**Wichtig:** Alle Zeit-Parameter (Attack, Decay, Release) verwenden identisches Clamping auf [0.001, 10.0], wie in der Parameter-Spezifikation definiert.

---

## Nächste Schritte



**Zukünftige Verbesserungen (Backlog):**
- **Exponentielle Attack/Decay-Kurven:** Natürlicherer Klang (aktuell: linear)
- **Velocity-Skalierung:** ADSR-Parameter abhängig von Note-Velocity
- **Curve-Parameter:** Anpassbare Kurven-Charakteristik (linear ↔ exponentiell)
- **Per-Voice Modulation:** Sample-genaue Parameter-Änderungen ohne Artifacts

---

## Referenzen

- ADSR Theory: https://en.wikipedia.org/wiki/Envelope_(music)
- Catch2 Documentation: https://github.com/catchorg/Catch2/tree/devel/docs
- JUCE ADSR Class: https://docs.juce.com/master/classADSR.html (Referenz, nicht verwendet)
- Exponential Decay: https://www.music.mcgill.ca/~gary/307/week4/adsr.html
