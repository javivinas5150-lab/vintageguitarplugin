#pragma once

#include <JuceHeader.h>
#include <array>

class VintageGuitarSuiteAudioProcessor;

//==============================================================================
// PresetManager - a bank of 24 rig presets.
//
// The bank is stored in one file shared by every instance of the plugin and
// by the standalone app:
//     Windows: %APPDATA%\Jemini Guitar Rig\Presets.xml
// A preset holds the whole rig (all parameters + NAM / IR file paths), but not
// the look (skin, window size) and never the tuner state.
// All methods are for the MESSAGE thread.
//==============================================================================
class PresetManager
{
public:
    static constexpr int numSlots = 24;

    explicit PresetManager (VintageGuitarSuiteAudioProcessor&);

    juce::String getName (int slot) const;
    bool isEmpty (int slot) const;

    bool load (int slot);                                  // false if the slot is empty
    void store (int slot, const juce::String& name);       // current rig -> slot
    void rename (int slot, const juce::String& name);
    void clear (int slot);

    int getCurrent() const;                                // -1 = none yet
    void markStateAsLoaded() { loadedFingerprint = fingerprint(); }
    bool isModified() const;                               // rig changed since load/save

    juce::File getFile() const;

private:
    struct Slot { juce::String name; juce::ValueTree state; };

    void readFile();
    void writeFile() const;
    void createFactoryBank();
    void setCurrent (int slot);
    juce::String fingerprint() const;

    VintageGuitarSuiteAudioProcessor& processor;
    std::array<Slot, numSlots> slots;
    juce::String loadedFingerprint;
};
