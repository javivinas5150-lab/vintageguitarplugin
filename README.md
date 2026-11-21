# Vintage Guitar Suite

VST3 guitar amp & effects plugin built with JUCE (C++17) for Windows / Cubase 12.

## Signal chain (v1)

Uni-Vibe → Klon-style Overdrive → NAM Amp (Fender Dual Showman captures) → Cabinet IR → Lexicon-style Delay/Reverb

Each module has its own bypass.

## Third-party

See `ThirdParty/README.md` (NeuralAmpModelerCore - MIT, Eigen - MPL2, nlohmann/json - MIT).

## Roadmap

1. Basic plugin (input/output gain)
2. NAM amp
3. Cabinet IR
4. Pedals (Uni-Vibe, Klon-style OD)
5. Delay / reverb
6. Vintage GUI (black-panel Dual Showman + Lexicon-style rack)
7. Advanced features (dual IR, routing, presets)

## Build (Windows)

1. Open `VintageGuitarSuite.jucer` in Projucer.
2. Check that the JUCE global path is set: **File → Global Paths → Path to JUCE** (e.g. `C:\JUCE`) and **JUCE Modules** (e.g. `C:\JUCE\modules`).
3. Click **Save and Open in IDE** (Visual Studio 2026).
4. In Visual Studio, set the configuration to **Release / x64**, right-click **VintageGuitarSuite_VST3** → **Build**.
5. Copy `Builds\VisualStudio2026\x64\Release\VST3\VintageGuitarSuite.vst3` to `C:\Program Files\Common Files\VST3\`.
6. In Cubase: **Studio → VST Plug-in Manager → Rescan**.

## Project layout

```
VintageGuitarSuite.jucer   Projucer project
Source/
  PluginProcessor.h/.cpp   audio processing + parameters
  PluginEditor.h/.cpp      GUI
```
