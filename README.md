# Vintage Guitar Suite

VST3 guitar amp & effects plugin built with JUCE (C++17) for Windows / Cubase 12.

## Signal chain (v1)

Uni-Vibe → Klon-style Overdrive → NAM Amp (Fender Dual Showman captures) → Cabinet IR → Lexicon-style Delay/Reverb

Each module has its own bypass.

## Roadmap

1. Basic plugin (input/output gain)
2. NAM amp
3. Cabinet IR
4. Pedals (Uni-Vibe, Klon-style OD)
5. Delay / reverb
6. Vintage GUI (black-panel Dual Showman + Lexicon-style rack)
7. Advanced features (dual IR, routing, presets)

## Build

Open the `.jucer` file in Projucer, export to Visual Studio 2022, and build the VST3 target.
