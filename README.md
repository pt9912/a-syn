# Analog Synth

Polyphoner subtraktiver Synthesizer als VST3/LV2/CLAP/Standalone-Plugin für Linux.

## Parameter

- Attack/Decay/Release: 0.001 s – 10 s, linear Attack/Decay, exponentieller Release-Decay mit Zeitkonstante τ (τ ≠ „Zeit bis 0“); ein 0.0001-Epsilon beendet den Release-State sauber.
- Sustain: 0.0 – 1.0 Level
- Filter-Cutoff: 20 Hz – 20 kHz, Resonanz 0 – 1.0
- Parameter-Setter clampen Eingaben auf die gültigen Bereiche und setzen bei nicht-finiten Werten auf die dokumentierten Defaults zurück.

## Features

- Polyphonie: 8 Stimmen
- Oszillator: Sine, Saw, Square, Triangle
- ADSR-Hüllkurve
- 24dB Tiefpassfilter
- VST3, LV2, CLAP und Standalone

## Build

### Voraussetzungen

- Debian Trixie (oder DevContainer)
- CMake ≥4.1.2
- C++20 Compiler (GCC/Clang)

### DevContainer (empfohlen)

```bash
# In VS Code: Reopen in Container
```

### Lokaler Build

```bash
# Abhängigkeiten
sudo apt-get install -y build-essential cmake ninja-build \
  libasound2-dev libjack-jackd2-dev libfreetype6-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  lv2-dev

# CMake 4.1.2 direkt von Kitware (falls nicht bereits installiert)
curl -LO https://github.com/Kitware/CMake/releases/download/v4.1.2/cmake-4.1.2-linux-x86_64.sh
chmod +x cmake-4.1.2-linux-x86_64.sh
sudo ./cmake-4.1.2-linux-x86_64.sh --skip-license --prefix=/usr/local

# Submodules
git submodule update --init --recursive

# Build (CLAP/LV2 SDK Pfade an JUCE übergeben)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DJUCE_CLAP_SDK_DIR=external/clap/include \
  -DJUCE_LV2_SDK_DIR=external/lv2
cmake --build build

# Installieren
# VST3
mkdir -p ~/.vst3
cp -r "build/AnalogSynth_artefacts/Release/VST3/Analog Synth.vst3" \
  ~/.vst3/

# CLAP
mkdir -p ~/.clap
cp "build/AnalogSynth_artefacts/Release/CLAP/Analog Synth.clap" \
  ~/.clap/

# LV2
mkdir -p ~/.lv2
cp -r "build/AnalogSynth_artefacts/Release/LV2/Analog Synth.lv2" \
  ~/.lv2/
```

## Tests

```bash
cd build
ctest --output-on-failure
```

## Verwendung

### Standalone
```bash
./build/AnalogSynth_artefacts/Release/Standalone/Analog\ Synth
```

### VST3
Plugin liegt nach dem Build unter `build/AnalogSynth_artefacts/Release/VST3/Analog Synth.vst3` und wird nach `~/.vst3/` kopiert.

### LV2
Plugin-Bundle wird nach `~/.lv2/` kopiert. Host-DAWs wie Ardour, Carla oder Qtractor erkennen es nach einem Plug-in-Scan.

### CLAP
Plugin liegt nach dem Build in `build/AnalogSynth_artefacts/Release/CLAP/Analog Synth.clap` und wird nach `~/.clap/` kopiert. Host-DAWs wie Bitwig oder Reaper erkennen es automatisch nach einem Plug-in-Scan.

## Entwicklung

- Architektur: `docs/Architecture.md` (High-Level), ergänzend `docs/Pflichtenheft.md`
- Sprint-Planung: `docs/Sprint-1.md`

## Lizenz

[Hier Lizenz einfügen]
