# S1-T1: Projekt-Setup & Entwicklungsumgebung

**Sprint:** 1
**Aufwand:** 4 Stunden
**Priorität:** 1 (Blocker)
**Stand:** 2025-11-08

---

## Ziel

Vollständige Entwicklungsumgebung aufsetzen mit:
- JUCE-Projekt mit CMake-Konfiguration
- DevContainer für reproduzierbare Builds
- CI/CD-Pipeline (GitHub Actions)
- Build-Validierung für alle Targets

**Ergebnis:** Build läuft lokal und in CI, DevContainer funktioniert

---

## Technische Spezifikation

### 1. Entwicklungsumgebung (gemäß Pflichtenheft Kapitel 6)

| Komponente              | Version                 | Zweck                            |
| ----------------------- | ----------------------- | -------------------------------- |
| **C++**                 | 20                      | Sprache                          |
| **JUCE**                | 8.0.10                  | Framework für Audio, GUI, Plugin |
| **CMake**               | 4.1.2 (Kitware Release) | Build-System                     |
| **CLAP SDK**            | 1.2.6                   | CLAP-Schnittstelle (via JUCE)    |
| **VST3 SDK**            | 3.7.14                  | VST3-Schnittstelle (via JUCE)    |
| **LV2 SDK**             | 1.18.10                 | LV2-Schnittstelle (via JUCE)     |
| **Catch2**              | 3.11.0                  | Unit-Tests                       |
| **Docker DevContainer** | Debian Trixie (via typescript-node:22) | einheitliche Umgebung            |
| **Ninja**               | Latest                  | Schneller Build-Generator        |
| **Valgrind**            | Latest                  | Memory-Leak-Detection            |

> **Hinweis:** Debian Trixie und Ubuntu 22.04/24.04 liefern standardmäßig nur ältere CMake-Versionen aus. Für die geforderte Version 4.1.2 wird daher das offizielle Kitware-Binary (siehe unten) in DevContainer, CI und lokalen Builds installiert.

---

## Aufgaben

### 1.1 Projektstruktur anlegen

```
a-syn/
├── .devcontainer/
│   ├── devcontainer.json
│   └── Dockerfile
├── .github/
│   └── workflows/
│       └── build.yml
├── cmake/
│   └── JUCEHelpers.cmake
├── external/
│   ├── JUCE/                  # JUCE Submodule
│   ├── Catch2/                # Catch2 Submodule
│   ├── clap/                  # CLAP SDK Submodule (for reference)
│   ├── clap-juce-extensions/  # CLAP-JUCE Bridge Submodule
│   └── lv2/                   # LV2 SDK Submodule
├── src/
│   ├── core/              # DSP Engine
│   ├── gui/               # UI Components
│   ├── plugin/            # AudioProcessor
│   └── main.cpp           # Standalone Entry
├── tests/
│   └── AdsrTests.cpp
├── .gitignore
├── CMakeLists.txt
└── README.md
```

**`.gitignore`:**
```gitignore
# Build-Verzeichnisse
build/
cmake-build-*/
.ccache/

# IDE
.vscode/
.idea/
*.user
*.swp
*.swo
*~

# Binaries
*.o
*.so
*.a
*.dylib
*.exe

# JUCE-spezifisch
JuceLibraryCode/
Builds/

# macOS
.DS_Store

# Submodules (werden via git submodule verwaltet)
!external/.gitkeep
```

### 1.2 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 4.1.2)
project(AnalogSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SDK paths must be set BEFORE adding JUCE
set(JUCE_LV2_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/lv2")

# JUCE Framework
add_subdirectory(external/JUCE)

# CLAP support via clap-juce-extensions
add_subdirectory(external/clap-juce-extensions EXCLUDE_FROM_ALL)

# Plugin-Formate
juce_add_plugin(AnalogSynth
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE YCOM
    PLUGIN_CODE ASyn
    FORMATS VST3 CLAP LV2 Standalone
    PRODUCT_NAME "Analog Synth"
    LV2URI "urn:yourcompany:analogsynth"

    # Synth-spezifische Flags
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    # Plugin-Eigenschaften
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(AnalogSynth)

# Add CLAP support
clap_juce_extensions_plugin(TARGET AnalogSynth
    CLAP_ID "com.yourcompany.analogsynth"
    CLAP_FEATURES instrument synthesizer)

target_sources(AnalogSynth PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp
    src/core/AdsrEnvelope.cpp
    src/core/Oscillator.cpp
)

target_compile_definitions(AnalogSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_compile_options(AnalogSynth PRIVATE
    -Wall -Wextra -Wpedantic
    # Disable -Werror for now due to JUCE warnings with newer GCC
    # $<$<CONFIG:Release>:-Werror>
)

target_link_libraries(AnalogSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)

# Tests
include(CTest)
enable_testing()

add_subdirectory(external/Catch2)
add_executable(AnalogSynthTests
    tests/AdsrTests.cpp
    # Source-Dateien für Tests
    src/core/AdsrEnvelope.cpp
)

target_include_directories(AnalogSynthTests PRIVATE src)

target_compile_definitions(AnalogSynthTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(AnalogSynthTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_dsp
)

add_test(NAME AdsrTests COMMAND AnalogSynthTests)
```

### 1.3 DevContainer

**`.devcontainer/Dockerfile`:**
```dockerfile
FROM mcr.microsoft.com/devcontainers/typescript-node:22

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    ninja-build \
    git \
    pkg-config \
    ccache \
    clang-18 \
    lld-18 \
    lldb-18 \
    clangd-18 \
    llvm-18 \
    llvm-18-dev \
    valgrind \
    gdb \
    curl \
    libasound2-dev \
    libjack-jackd2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libcurl4-openssl-dev \
    libwebkit2gtk-4.1-dev \
    mesa-common-dev \
    lv2-dev \
    && rm -rf /var/lib/apt/lists/*


# Aktuelles CMake direkt von Kitware installieren
ENV CMAKE_VERSION=4.1.2
WORKDIR /tmp
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh \
    && chmod +x cmake-${CMAKE_VERSION}-linux-x86_64.sh \
    && ./cmake-${CMAKE_VERSION}-linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm cmake-${CMAKE_VERSION}-linux-x86_64.sh

# Claude CLI installieren
RUN npm install -g @anthropic-ai/claude-code

# CCache konfigurieren
ENV CCACHE_DIR=/workspace/.ccache
ENV PATH="/usr/lib/ccache:${PATH}"

USER node
WORKDIR /workspace
```

**`.devcontainer/devcontainer.json`:**
```json
{
  "name": "Analog Synth Dev",
  "build": {
    "dockerfile": "Dockerfile"
  },
  "customizations": {
    "vscode": {
      "extensions": [
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools",
        "twxs.cmake",
        "llvm-vs-code-extensions.vscode-clangd",
        "vadimcn.vscode-lldb",
        "ms-vscode.makefile-tools"
      ],
      "settings": {
        "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
        "cmake.configureOnOpen": true,
        "cmake.generator": "Ninja"
      }
    }
  },
  "postCreateCommand": "git submodule update --init --recursive && ccache -M 2G",
  "mounts": [
    "source=${localWorkspaceFolder}/.ccache,target=/workspace/.ccache,type=bind,consistency=cached",
    "source=${localEnv:HOME}/.claude,target=/home/node/.claude,type=bind,consistency=cached",
    "source=${localEnv:HOME}/.claude.json,target=/home/node/.claude.json,type=bind,consistency=cached"
  ],
  "remoteUser": "node"
}
```

### 1.4 CI/CD Pipeline

**`.github/workflows/build.yml`:**
```yaml
name: Build & Test

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-linux:
    runs-on: ubuntu-latest
    container:
      image: debian:trixie

    steps:
    - uses: actions/checkout@v4
      with:
        submodules: recursive

    - name: Install Dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          libasound2-dev \
          libjack-jackd2-dev \
          libfreetype6-dev \
          libx11-dev \
          libxrandr-dev \
          libxinerama-dev \
          libxcursor-dev \
          lv2-dev

    - name: Install CMake 4.1.2
      run: |
        curl -LO https://github.com/Kitware/CMake/releases/download/v4.1.2/cmake-4.1.2-linux-x86_64.sh
        chmod +x cmake-4.1.2-linux-x86_64.sh
        sudo ./cmake-4.1.2-linux-x86_64.sh --skip-license --prefix=/usr/local

    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_CLAP_SDK_DIR=external/clap/include -DJUCE_LV2_SDK_DIR=external/lv2

    - name: Build
      run: cmake --build build --config Release --target AnalogSynth_Standalone AnalogSynth_VST3 AnalogSynth_CLAP AnalogSynth_LV2

    - name: Run Tests
      run: cd build && ctest --output-on-failure

    - name: Upload Artifacts
      uses: actions/upload-artifact@v4
      with:
        name: AnalogSynth-Linux
        path: |
          build/AnalogSynth_artefacts/Release/Standalone/
          build/AnalogSynth_artefacts/Release/VST3/
          build/AnalogSynth_artefacts/Release/CLAP/
          build/AnalogSynth_artefacts/Release/LV2/
```

### 1.5 JUCE Integration

**Externe Abhängigkeiten einbinden:**
```bash
cd a-syn
git submodule add -b 8.0.10 https://github.com/juce-framework/JUCE.git external/JUCE
git submodule add -b v3.11.0 https://github.com/catchorg/Catch2.git external/Catch2
git submodule add https://github.com/free-audio/clap.git external/clap
git submodule add https://github.com/lv2/lv2.git external/lv2
git submodule add https://github.com/free-audio/clap-juce-extensions.git external/clap-juce-extensions
git submodule update --init --recursive
```

> **Hinweis:** CLAP-Unterstützung wird über `clap-juce-extensions` bereitgestellt, da JUCE nativ noch keine vollständige CLAP-Unterstützung bietet. Das `clap` Submodule wird als Referenz behalten, aber `clap-juce-extensions` bringt seine eigene CLAP-SDK-Version mit.

---

## Validierung

### Lokaler Build-Test

```bash
# Im DevContainer
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Testen
cd build && ctest --output-on-failure

# Standalone ausführen
./build/AnalogSynth_artefacts/Release/Standalone/Analog\ Synth
```

### Erwartete Outputs

```
build/
└── AnalogSynth_artefacts/
    └── Release/
        ├── Standalone/
        │   └── Analog Synth                  # Binary (~9.4 MB)
        ├── VST3/
        │   └── Analog Synth.vst3/            # VST3 Bundle
        │       └── Contents/
        │           └── x86_64-linux/
        │               └── Analog Synth.so
        ├── CLAP/
        │   └── Analog Synth.clap             # CLAP Binary (~9.4 MB)
        └── LV2/
            └── Analog Synth.lv2/             # LV2 Bundle
                ├── manifest.ttl
                ├── dsp.ttl
                ├── ui.ttl
                └── libAnalog Synth.so
```

### Checkliste

- [x] DevContainer startet ohne Fehler
- [x] CMake-Konfiguration läuft durch
- [x] Build kompiliert alle Targets (Standalone, VST3, CLAP, LV2)
- [x] Standalone startet (auch wenn noch kein Audio)
- [x] VST3 wird korrekt gebaut (`Analog Synth.vst3`)
- [x] CLAP wird korrekt gebaut (`Analog Synth.clap`, via clap-juce-extensions)
- [x] LV2 wird korrekt gebaut (`Analog Synth.lv2`)
- [x] Tests laufen durch (ctest)
- [ ] CI-Pipeline ist grün
- [x] Keine kritischen Compiler-Warnungen (`-Wall -Wextra`)
- [x] clap-juce-extensions Submodule ist eingebunden

> **Hinweis:** `-Werror` ist deaktiviert aufgrund von Warnungen in JUCE Core mit GCC 14.2. Dies kann später aktiviert werden, wenn JUCE aktualisiert wird oder die Warnungen behoben sind.

> Bezug zu `docs/MVP.md`: Der MVP fordert explizit einen lauffähigen VST3/LV2/CLAP-Build. Die obige Validierung stellt sicher, dass alle Plug-in-Formate (`Analog Synth.vst3`, `Analog Synth.clap`, `Analog Synth.lv2`) parallel zum Standalone-Binary erzeugt und bereitgestellt werden.

---

## Troubleshooting

### Problem: JUCE Submodule nicht gefunden
```bash
git submodule update --init --recursive
```

### Problem: Missing Linux Headers
```bash
sudo apt-get install libasound2-dev libjack-jackd2-dev
```

### Problem: CMake findet JUCE nicht
Prüfen, ob `external/JUCE/CMakeLists.txt` existiert

### Problem: CLAP wird nicht gebaut
- Stelle sicher, dass `clap-juce-extensions` Submodule initialisiert ist
- Prüfe, ob `clap_juce_extensions_plugin()` im CMakeLists.txt aufgerufen wird
- JUCE allein bietet keine native CLAP-Unterstützung - `clap-juce-extensions` ist erforderlich

### Problem: Compiler-Warnungen in JUCE mit GCC 14+
- Bekanntes Problem: `-Wmaybe-uninitialized` in JUCE Core
- Lösung: `-Werror` temporär deaktivieren (bereits im CMakeLists.txt umgesetzt)
- Alternative: Auf ältere GCC-Version wechseln oder auf JUCE-Updates warten

---

## Abhängigkeiten

- **Blockt:** S1-T2, S1-T3, S1-T4 (alle anderen Tasks)
- **Blockiert von:** keine

---

## README.md Template

**`README.md`:**
```markdown
# Analog Synth

Polyphoner subtraktiver Synthesizer als VST3/LV2/CLAP/Standalone-Plugin für Linux.

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
sudo apt-get install -y build-essential cmake ninja-build lv2-dev \
  libasound2-dev libjack-jackd2-dev libfreetype6-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev

# CMake 4.1.2 direkt von Kitware (falls nicht bereits installiert)
curl -LO https://github.com/Kitware/CMake/releases/download/v4.1.2/cmake-4.1.2-linux-x86_64.sh
chmod +x cmake-4.1.2-linux-x86_64.sh
sudo ./cmake-4.1.2-linux-x86_64.sh --skip-license --prefix=/usr/local

# Submodules
git submodule update --init --recursive

# Build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Installieren
mkdir -p ~/.vst3 ~/.clap ~/.lv2
cp -r "build/AnalogSynth_artefacts/Release/VST3/Analog Synth.vst3" \
  ~/.vst3/
cp "build/AnalogSynth_artefacts/Release/CLAP/Analog Synth.clap" \
  ~/.clap/
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
Plugin wird nach `~/.vst3/Analog Synth.vst3` kopiert (via `COPY_PLUGIN_AFTER_BUILD`) und kann in DAWs geladen werden.

### CLAP
Plugin liegt nach dem Build in `build/AnalogSynth_artefacts/Release/CLAP/Analog Synth.clap`. CLAP-Unterstützung wird über `clap-juce-extensions` bereitgestellt. Nach dem Kopieren nach `~/.clap/` erkennen Host-DAWs wie Bitwig oder Reaper es automatisch nach einem Plug-in-Scan.

### LV2
Plugin liegt nach dem Build in `build/AnalogSynth_artefacts/Release/LV2/Analog Synth.lv2` und wird nach `~/.lv2/` kopiert (via `COPY_PLUGIN_AFTER_BUILD`). LV2-Hosts wie Ardour oder Carla finden es nach einem Plugin-Refresh.

## Entwicklung

- Architektur: Siehe `docs/Pflichtenheft.md`
- Sprint-Planung: Siehe `docs/Sprint-1.md`

## Lizenz

[Hier Lizenz einfügen]
```

---

## Definition of Done (S1-T1)

- ✅ DevContainer läuft auf Debian Trixie
- ✅ CMake-Projekt kompiliert Standalone + VST3 + CLAP + LV2
- ✅ CLAP-Unterstützung via clap-juce-extensions eingebunden
- ✅ Alle Unit-Tests laufen durch (ctest)
- ✅ AdsrEnvelope Bug behoben (epsilon-threshold für Release-Phase)
- [ ] CI-Pipeline (GitHub Actions) ist eingerichtet und grün
- ✅ Build-Dokumentation in README.md
- ✅ `-Wall -Wextra -Wpedantic` aktiviert (`-Werror` deaktiviert wegen JUCE-Warnungen)
- ✅ JUCE 8.0.10, Catch2 3.11.0, clap-juce-extensions als Submodules eingebunden
- ✅ .gitignore konfiguriert
- ✅ Memory-Leak-Tools (valgrind) verfügbar
- ✅ LV2URI korrekt gesetzt (`urn:yourcompany:analogsynth`)

---

## Referenzen

- Pflichtenheft Kapitel 6: Entwicklungsumgebung
- JUCE CMake API: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md
- JUCE AudioPlugin Tutorial: https://docs.juce.com/master/tutorial_create_projucer_basic_plugin.html
- CLAP-JUCE Extensions: https://github.com/free-audio/clap-juce-extensions
- CLAP Specification: https://github.com/free-audio/clap
