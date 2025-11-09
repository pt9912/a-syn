# Lastenheft

## Projekt: Analoger Software-Synthesizer mit ADSR

**Zielplattform:** Linux (Desktop, Plugin-Formate: VST3, LV2, CLAP)  
**Programmiersprache:** C++20 oder höher  
**Framework:** JUCE 8.0.10

---

## 1. Zielsetzung

Ziel ist die Entwicklung eines plattformübergreifenden, analogen Software-Synthesizers mit klassischer subtraktiver Synthese, der sowohl standalone als auch als Plugin (VST3, LV2, CLAP) unter Linux lauffähig ist.

Der Synthesizer soll einen warmen, analogen Klangcharakter bieten und grundlegende Syntheseparameter (Oszillator, Filter, Hüllkurven, Verstärker) über eine grafische Oberfläche steuerbar machen.

---

## 2. Einsatzbereich

- Musikproduktion in Linux-basierten Digital Audio Workstations (DAWs)
- Einsatz als Lehr- und Testumgebung für digitale Signalverarbeitung und Synthese
- Standalone-Modus für Live-Performance oder Klangforschung

---

## 3. Produktübersicht

Der Synthesizer besteht aus folgenden Hauptkomponenten:

| Komponente                     | Beschreibung                                                                                         |
| ------------------------------ | ---------------------------------------------------------------------------------------------------- |
| **Audio-Engine**               | Realtime-DSP-Engine zur Erzeugung, Filterung und Verstärkung des Audiosignals                        |
| **Syntheseeinheit (DSP-Core)** | Enthält Oszillator, ADSR-Hüllkurve, Filter, Verstärker                                               |
| **Benutzeroberfläche (GUI)**   | Grafische Oberfläche mit Reglern, Anzeigen und Preset-Verwaltung                                     |
| **Plugin-Wrapper**             | Adapter-Schichten zur Bereitstellung der Formate VST3, LV2 und CLAP                                  |
| **Parameterverwaltung**        | Synchronisierung zwischen GUI und DSP (z. B. Attack, Decay, Sustain, Release, Cutoff, Resonanz etc.) |
| **Preset-System**              | Speichern und Laden von Sound-Einstellungen                                                          |

---

## 4. Funktionsanforderungen

| Nr.     | Funktion                    | Beschreibung                                                          |
| ------- | --------------------------- | --------------------------------------------------------------------- |
| **F1**  | Audio-Ausgabe               | Synthesizer erzeugt kontinuierlichen Ton basierend auf aktiver Stimme |
| **F2**  | MIDI-Steuerung              | Note-On/Off, Velocity, Pitchbend, Modulation                          |
| **F3**  | Oszillator                  | Grundformen: Sägezahn, Rechteck, Dreieck, Sinus                       |
| **F4**  | Filter                      | Tiefpass 24 dB/Oct mit Cutoff- und Resonanz-Steuerung                 |
| **F5**  | ADSR-Hüllkurve              | Steuerung von Amplitude (VCA) und Filterfrequenz (VCF)                |
| **F6**  | Polyphonie                  | Mind. 8 Stimmen gleichzeitig                                          |
| **F7**  | Preset-Management           | Laden/Speichern von benutzerdefinierten Sounds                        |
| **F8**  | GUI                         | Echtzeit-Darstellung aller Parameter mit Reglern/Slidern              |
| **F9**  | Audio-Plugin-Schnittstellen | Unterstützung der Standards VST3, LV2 und CLAP                        |
| **F10** | Standalone-Modus            | Direkter Betrieb ohne DAW                                             |
| **F11** | Parameter-Automation        | Automatisierbare Parameter in DAW (VST3/LV2/CLAP)                     |
| **F12** | Persistenz                  | Plugin speichert Zustand im Projekt (DAW-Snapshot-kompatibel)         |

---

## 5. Nicht-funktionale Anforderungen

| Kategorie           | Anforderung                                                                 |
| ------------------- | --------------------------------------------------------------------------- |
| **Performance**     | Latenz < 10 ms, CPU-Last bei 8 Stimmen ≤ 25 % auf Mittelklasse-CPU          |
| **Stabilität**      | Keine Dropouts oder Abstürze bei kontinuierlicher Nutzung                   |
| **Portabilität**    | Lauffähig unter Linux x86_64, Kompatibilität zu JUCE-Host und gängigen DAWs |
| **Bedienbarkeit**   | Intuitive GUI, keine tiefen Menüstrukturen                                  |
| **Erweiterbarkeit** | Architektur erlaubt spätere Ergänzung weiterer Module (z. B. LFOs, Effekte) |
| **Lizenz**          | Quelloffen (MIT oder GPLv3, Entscheidung offen)                             |

---

## 6. Schnittstellen

| Typ                       | Beschreibung                                             |
| ------------------------- | -------------------------------------------------------- |
| **Audio I/O**             | über JUCE (ALSA, JACK, PipeWire)                         |
| **MIDI I/O**              | JUCE-MIDI-Interface                                      |
| **Plugin-Schnittstellen** | VST3 SDK, LV2 Host API, CLAP SDK über JUCE-Erweiterungen |
| **Preset-Dateien**        | JSON-basiert, im Benutzerverzeichnis gespeichert         |

---

## 7. Qualitätsanforderungen

- Geringe Latenz und konstante Sample-Genauigkeit
- Testbarkeit einzelner DSP-Komponenten
- Reproduzierbares Verhalten über alle Plugin-Formate hinweg
- Quellcode klar dokumentiert (Doxygen-kompatibel)

---

## 8. Entwicklungsumgebung

| Komponente             | Beschreibung                                     |
| ---------------------- | ------------------------------------------------ |
| **Betriebssystem**     | Linux (Ubuntu, Fedora, Arch, o. ä.)              |
| **IDE/Editor**         | VS Code, CLion oder JUCE Projucer                |
| **Build-System**       | CMake                                            |
| **Abhängigkeiten**     | JUCE 8.0.10, VST3 SDK, clap-juce-extensions      |
| **Versionsverwaltung** | Git                                              |
| **Containerisierung**  | DevContainer (Docker) mit Audio- und Build-Tools |
| **Test-Framework**     | Catch2 oder GoogleTest                           |

---

## 9. Abnahmekriterien

Das Projekt gilt als erfolgreich abgeschlossen, wenn:

1. Der Synthesizer unter Linux als Standalone-Anwendung startet.
2. Das Plugin in einer DAW (z. B. Ardour, Bitwig, Reaper) als VST3, LV2 und CLAP geladen werden kann.
3. Ein Ton erzeugt wird, der auf ADSR reagiert.
4. Alle Parameter bedienbar, automatisierbar und speicherbar sind.
5. Keine Dropouts oder Abstürze bei Dauertest auftreten.