# ⚙️ Pflichtenheft

**Projektname:** Analog Synth – Linux VST3/LV2/CLAP Synthesizer  
**Version:** 1.0 (MVP)  
**Erstellt von:** Entwicklungsteam  
**Stand:** 2025-11-08

---

## 1. Ziel und Abgrenzung

Das Pflichtenheft beschreibt die technische Umsetzung des im Lastenheft definierten Synthesizers.  
Ziel ist eine polyphone subtraktive Synthese-Engine mit ADSR, die in Linux als Standalone- und Plugin-Version (VST3/LV2/CLAP) betrieben werden kann.

### Nicht enthalten:
- Erweiterte Modulation (LFOs, Envelopes > 1)
- Effekte (Delay, Reverb)
- Preset-Management mit Kategorien
- Multitimbralität oder MPE

---

## 2. Systemarchitektur

### Übersicht

```
┌────────────────────────────────────────┐
│              GUI-Schicht               │
│ (JUCE Components, Parameterbindung)    │
└────────────────────────────────────────┘
                  │
                  ▼
┌────────────────────────────────────────┐
│          AudioProcessor-Schicht        │
│ (JUCE::AudioProcessor, Parameter, Host)│
└────────────────────────────────────────┘
                  │
                  ▼
┌────────────────────────────────────────┐
│           DSP Core (Engine)            │
│  - SynthVoice (1 Stimme)               │
│  - Oscillator (VCO)                    │
│  - Filter (VCF)                        │
│  - AdsrEnvelope (VCA/VCF)              │
└────────────────────────────────────────┘
```

- JUCE verwaltet Audio-IO, Parameter, GUI und Host-Integration.
- DSP-Code ist unabhängig vom Host-Format (wird in allen Plugins verwendet).
- Über JUCE CMake werden mehrere Targets erzeugt:
  - `AnalogSynth_Standalone`
  - `AnalogSynth_VST3`
  - `AnalogSynth_LV2`
  - `AnalogSynth_CLAP`

---

## 3. Hauptmodule

| Modul                      | Beschreibung                     | Technische Details                                                     |
| -------------------------- | -------------------------------- | ---------------------------------------------------------------------- |
| **Core::Oscillator**       | Erzeugt periodische Wellenformen | Bandlimited Saw, Square, Triangle, Sine; SampleRate-aware Phase-Update |
| **Core::AdsrEnvelope**     | Steuert Amplitude & Filter       | Attack/Decay/Sustain/Release; Sample-genaue Berechnung                 |
| **Core::Filter**           | Tiefpass-VCF (24dB)              | Ladder- oder StateVariableFilter                                       |
| **Core::Voice**            | Kombiniert VCO, VCF, VCA         | Eigene ADSR-Instanz, NoteOn/NoteOff                                    |
| **Core::SynthEngine**      | Voice-Management, Polyphonie     | Max. 8 Stimmen, einfache Voice-Stealing-Logik; mehrere `SynthVoice`-Instanzen laufen parallel |
| **Gui::MainComponent**     | Oberfläche mit Slidern           | Attack, Decay, Sustain, Release, Cutoff, Resonanz, Waveform            |
| **Plugin::AudioProcessor** | Host-Schnittstelle               | VST3/LV2/CLAP-kompatibel, Parameterbindung                             |
| **Plugin::AudioEditor**    | JUCE-UI-Host                     | GUI-Rendering + Parameter Sync                                         |
| **Utils::PresetManager**   | JSON-basierte Presets            | Optional im MVP deaktiviert                                            |

---

## 4. Parameter & Automatisierung

| Parameter     | Typ   | Wertebereich                | Default | Automation |
| ------------- | ----- | --------------------------- | ------- | ---------- |
| **attack**    | Float | 0.001–10.0 s                | 0.01    | ja         |
| **decay**     | Float | 0.001–10.0 s                | 0.2     | ja         |
| **sustain**   | Float | 0–1.0                       | 0.7     | ja         |
| **release**   | Float | 0.001–10.0 s                | 0.5     | ja         |
| **cutoff**    | Float | 20–20000 Hz                 | 800     | ja         |
| **resonance** | Float | 0–1.0                       | 0.2     | ja         |
| **waveform**  | Enum  | SINE, SAW, SQUARE, TRIANGLE | SAW     | ja         |

**Release-Charakteristik:** Der Release-Parameter ist als Zeitkonstante τ eines exponentiellen Abklingens definiert (τ ≠ „Zeit bis 0“). Nach τ Sekunden ist das Level auf ~36.8 %, nach 3τ auf ~5 % gefallen. Ein Epsilon-Schwellenwert von 0.0001 beendet die Release-Phase, damit Stimmen zuverlässig in den Idle-State wechseln.

---

## 5. Audio- und MIDI-Handling

- Audio-Thread verarbeitet Frames in `processBlock()`
- MIDI-Eingänge werden in Echtzeit verarbeitet (noteOn, noteOff, pitchWheel)
- Für jede Note wird `SynthVoice` aktiviert oder gestohlen
- ADSR-Berechnung sampleweise; Envelope multipliziert Signalpegel
- Audio-Output in Stereo

---

## 6. Entwicklungsumgebung

| Komponente              | Version      | Zweck                            |
| ----------------------- | ------------ | -------------------------------- |
| **C++**                 | 20           | Sprache                          |
| **JUCE**                | 8.0.10       | Framework für Audio, GUI, Plugin |
| **CMake**               | ≥4.1.2       | Build-System                     |
| **CLAP SDK**            | 1.2.6        | CLAP-Schnittstelle (via JUCE)    |
| **VST3 SDK**            | 3.7.14       | VST3-Schnittstelle (via JUCE)    |
| **Catch2**              | 3.11.0       | Unit-Tests                       |
| **Docker DevContainer** | Debian Trixie (via typescript-node:22) | einheitliche Umgebung            |

---

## 7. Build-Konfiguration

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target AnalogSynth_VST3
```

### Outputs:
- `build/AnalogSynth_artefacts/Release/Standalone/`
- `build/AnalogSynth_artefacts/Release/VST3/`
- `build/AnalogSynth_artefacts/Release/LV2/`
- `build/AnalogSynth_artefacts/Release/CLAP/`

---

## 8. Tests

| Testart                 | Inhalt                                           |
| ----------------------- | ------------------------------------------------ |
| **Unit-Tests (Catch2)** | ADSR-Parameter, Hüllkurvenverlauf, Polyphonie    |
| **Integrationstest**    | SynthVoice + Engine-Kopplung                     |
| **Plugin-Test (Host)**  | Laden in Ardour/Bitwig, Audioausgabe prüfen      |
| **Performance-Test**    | CPU-Auslastung bei 8 Stimmen, Sample Rate 48 kHz |
