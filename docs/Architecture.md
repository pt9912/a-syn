# Architekturübersicht

Dieses Dokument ergänzt Lasten-/Pflichtenheft sowie die Sprint-Designs und erklärt, wie der Synthesizer strukturiert ist, wie Audio/MIDI/Parameter durch das System laufen und welche Artefakte der Build erzeugt.

---

## 1. Ziele & Rahmen

- Plattform: Linux (Standalone + VST3/LV2/CLAP) via JUCE 8.0.10
- Syntheseprinzip: Polyphone subtraktive Synthese mit klassischer ADSR-Amplitudenhüllkurve, Tiefpassfilter und vier Grundwellenformen
- Codebasis: C++20, CMake ≥4.1.2, Catch2 für Tests

---

## 2. Systemübersicht

```
        GUI-Schicht (JUCE Components)
           │  Parameterbindung (AudioProcessorValueTreeState)
           ▼
  AudioProcessor-Schicht (JUCE::AudioProcessor)
           │  Host-Integration, MIDI/Event Routing
           ▼
        SynthEngine (Core)
    ┌─────────────┬─────────────┬─────────────┐
    │ SynthVoice1 │ SynthVoice2 │   ...       │  bis max. 8 Stimmen
    └─────────────┴─────────────┴─────────────┘
           │ pro Stimme: Oscillator → Filter → AdsrEnvelope
           ▼
        Stereo-Ausgabe
```

- Jede Stimme besitzt eigene Instanzen von Oscillator, Filter und AdsrEnvelope.
- Die SynthEngine verwaltet bis zu acht `SynthVoice`-Objekte parallel und führt Voice-Stealing aus, wenn mehr Noten anliegen als Stimmen verfügbar sind.
- GUI und DSP bleiben über `AudioProcessorValueTreeState` synchron, sodass Parameteränderungen automatisierbar und thread-sicher sind.

---

## 3. Hauptkomponenten

| Ebene | Komponente | Verantwortung | Hinweise |
| ----- | ---------- | ------------- | -------- |
| GUI | `MainComponent` (JUCE) | Regler/Slider für ADSR, Cutoff, Resonanz, Waveform; Visualisierung | Verwendet JUCE LookAndFeel, bindet an AudioProcessorParameter |
| AudioProcessor | `AnalogSynthAudioProcessor` | Host-Lifecycle (`prepareToPlay`, `processBlock`, `getStateInformation`), Parameter-Tree, Preset-State | Erstellt `juce::Synthesiser`, registriert Stimmen und Sound, routet MIDI |
| Core | `SynthEngine` | Wrapper um `juce::Synthesiser` bzw. eigenes Voice-Management | Setzt Stimmenanzahl, tauscht Parameterwerte in Voices |
| Core | `SynthVoice` | Verarbeitet Einzelstimme: NoteOn/Off, Rendern von Samples | Enthält Oscillator, Filter, AdsrEnvelope, Gain |
| Core | `Oscillator` | Erzeugt Sine/Saw/Square/Triangle, Sample-rate-aware Phase | PolyBLEP/Antialiasing (siehe Backlog für Erweiterungen) |
| Core | `AdsrEnvelope` | Attack/Decay/Release 0.001–10 s, Sustain 0–1.0, exponentielles Release mit Zeitkonstante tau, epsilon 0.0001 | Parameter-Clamping + Sample-genaue State-Machine |
| Core | `Filter` | 24 dB Tiefpass (z. B. Ladder/StateVariable) mit Cutoff/Resonanz | Verarbeitung pro Sample/Block, optional Key Tracking |
| Glue | Plugin Targets | Wrapper für Standalone/VST3/LV2/CLAP | Generiert durch `juce_add_plugin` inkl. `COPY_PLUGIN_AFTER_BUILD` |

---

## 4. Datenflüsse

### 4.1 Audio

1. Host ruft `prepareToPlay(sampleRate, blockSize)` → Voices erhalten Sample-Rate und setzen ihre internen Koeffizienten (Oscillator, Filter, AdsrEnvelope).
2. Pro Block ruft Host `processBlock(AudioBuffer, MidiBuffer)`.
3. MIDI-Ereignisse werden vom `juce::Synthesiser` sequenziell an Stimmen verteilt (NoteOn → Stimme sucht freien Slot oder stealt, NoteOff → Release-Phase).
4. Jede aktive Stimme:
   - Generiert Rohsignal im Oscillator.
   - Formt Frequenzspektrum im Filter (Cutoff/Resonanz).
   - Multipliziert Amplitude mit ADSR (sampleweise).
5. Summiertes Ergebnis gelangt zurück zum Host (Stereo Float32).

### 4.2 Parameter & Automation

- Alle editierbaren Parameter wohnen im `AudioProcessorValueTreeState`.
- GUI-Controls binden sich via `SliderAttachment` an denselben Tree; Änderungen landen sofort im DSP.
- Hosts automatisieren Parameter, indem sie AudioProcessorParameter-Werte setzen → ValueTree benachrichtigt DSP & GUI.
- Setter in `AdsrEnvelope`, Filter usw. clampen Eingaben, tauschen NaN/Inf gegen Default-Werte, vermeiden so Instabilitäten im Audio-Thread.

### 4.3 Preserving State

- `getStateInformation`/`setStateInformation` serialisieren den ValueTree als XML/Binary; DAWs speichern diesen Snapshot im Projekt.
- Für Standalone/Presets ist ein JSON-basiertes System (`Utils::PresetManager`) vorgesehen, jedoch im MVP deaktiviert.

### 4.4 Sequenzen

**Note-On bis Audio-Output**

```
DAW Host      Plugin (AudioProcessor)      SynthEngine        SynthVoice
   | noteOn           |                        |                   |
   |----------------->| enqueue MIDI           |                   |
   |                  | processBlock()         |                   |
   |                  |----------------------->| allocate voice    |
   |                  |                        |------------------>|
   |                  |                        |   start attack    |
   |                  |                        |<------------------|
   |                  |<-----------------------| sum voice audio   |
   | mix / output     | write to AudioBuffer   |                   |
```

1. Host ruft `processBlock` und liefert aktuelle MIDI-Events.
2. AudioProcessor gibt Events an den SynthEngine/`juce::Synthesiser`.
3. Freie Stimme startet Attack-Phase, generiert Samples und addiert sie in den gemeinsamen Buffer.
4. Bei NoteOff durchläuft dieselbe Stimme Decay/Release, bis Epsilon erreicht ist und sie wieder freigegeben wird.

**Parameter-Automation**

```
DAW Automation   AudioProcessorValueTreeState     DSP/Voices
        | setParameter()        |                      |
        |---------------------->| updates ValueTree    |
        |                       | notify parameter     |
        |                       |--------------------->| setter + clamping
        |                       |                      | recompute coeffs
```

1. Host ändert einen Parameterwert (z. B. Cutoff).
2. ValueTree übernimmt den Wert und benachrichtigt GUI + DSP.
3. DSP-Objekte (Filter, ADSR) clampen Werte und aktualisieren Koeffizienten sample-sicher.

---

## 5. Build & Deployment

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DJUCE_CLAP_SDK_DIR=external/clap/include \
  -DJUCE_LV2_SDK_DIR=external/lv2
cmake --build build
```

Erzeugte Artefakte (Release-Konfiguration):

| Ziel | Pfad | Zweck |
| ---- | ---- | ----- |
| Standalone | `build/AnalogSynth_artefacts/Release/Standalone/Analog Synth` | Desktop-App (ALSA/JACK/PipeWire) |
| VST3 | `build/AnalogSynth_artefacts/Release/VST3/Analog Synth.vst3` | DAW-Plugin; nach `~/.vst3/` kopieren |
| CLAP | `build/AnalogSynth_artefacts/Release/CLAP/Analog Synth.clap` | CLAP-Host (Bitwig, Reaper) |
| LV2 | `build/AnalogSynth_artefacts/Release/LV2/Analog Synth.lv2/` | Ardour, Carla, etc. |
| Tests | `build/AnalogSynthTests` | Catch2 Unit-Tests (ADSR etc.) |

CI/DevContainer bauen dieselben Targets; `COPY_PLUGIN_AFTER_BUILD` kann genutzt werden, um Artefakte automatisch in Host-Verzeichnisse zu spiegeln (lokal optional).

---

## 6. Erweiterbarkeit

- Weitere Modulatoren (LFOs, zusätzliche Envelopes) lassen sich durch neue Core-Module ergänzen und in `SynthVoice` einklinken.
- Zusätzliche Effekte können hinter der Filter/Envelope-Kette eingefügt werden, solange sie im DSP-Core bleiben.
- Mehr Stimmen: `SynthEngine` erhält eine konfigurierbare Stimmenzahl; GUI + Parameter-Tree bleiben unverändert.
- Preset-System kann aktiviert werden, sobald JSON-Speicherlogik fertig ist; ValueTree bleibt die Quelle der Wahrheit.

---

## 7. Referenzen

- `docs/Pflichtenheft.md` – detaillierte Anforderungen & Parameter
- `docs/S1-T1-Design.md` – Projektsetup, Build- und DevContainer-Details
- `docs/S1-T2-Design.md` – ADSR-Implementierung & Teststrategie
