# Sprint 1 – Ziel & Aufgaben

## Sprint-Ziel
Erster lauffähiger Standalone-Synthesizer mit ADSR-Steuerung (Audioausgabe in Echtzeit).

---

## Umfang (2 Wochen)

| Prio | Aufgabe   | Beschreibung                                | Ergebnis                           | Std. |
| ---- | --------- | ------------------------------------------- | ---------------------------------- | ---- |
| 1    | **S1-T1** | JUCE-Projekt aufsetzen, CMake, DevContainer | Build läuft, CI-Pipeline aktiv     | 4    |
| 2    | **S1-T2** | ADSR-Klasse implementieren & testen         | Unit-Tests (Catch2) bestanden      | 6    |
| 3    | **S1-T3** | Oszillator (Sine + Saw) implementieren      | Waveforms generiert, aliasing-frei | 5    |
| 4    | **S1-T4** | AudioProcessor (Standalone) verbinden       | Ton erzeugt, ADSR wirkt            | 4    |
| 5    | **S1-T5** | GUI mit Reglern (ADSR)                      | Live-Kontrolle möglich             | 6    |
| 6    | **S1-T6** | Export als VST3                             | Plugin lädt in Reaper (Linux)      | 3    |
| 7    | **S1-T7** | Smoke-Test (Audio/MIDI)                     | Klang, keine Crashes, kein Leaks   | 2    |

**Gesamt:** ~30 Stunden

---

## Technische Details

### S1-T1: Setup
- CMake-Konfiguration mit JUCE 8.x
- DevContainer mit Build-Tools
- GitHub Actions CI (Build + Test)

### S1-T2: ADSR Tests
- Framework: Catch2
- Mindestens 80% Code-Coverage
- Tests: Envelope-Kurven, Edge-Cases

### S1-T3: Oszillator
- Bandlimited Synthesis (PolyBLEP/MinBLEP)
- Sample-Rate: 44.1/48 kHz
- Float32-Processing

### S1-T6: VST3 Export
- Target-Plattform: Linux (primär)
- Test-DAW: Reaper
- Validierung mit pluginval

---

## Definition of Done

- ✅ Synth erzeugt ADSR-gesteuerten Ton
- ✅ GUI-Regler wirken direkt
- ✅ Plugin & Standalone laufen stabil
- ✅ Unit-Tests bestanden (>80% Coverage)
- ✅ Keine kritischen Compiler-Warnungen
- ✅ Memory-Leaks geprüft (valgrind/sanitizer)
- ✅ Code reviewed
- ✅ Build-Dokumentation aktualisiert