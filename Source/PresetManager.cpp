#include "PresetManager.h"
#include "PluginProcessor.h"

namespace
{
    const juce::Identifier bankTag   ("JeminiPresetBank");
    const juce::Identifier presetTag ("Preset");
    const juce::Identifier slotAttr  ("slot");
    const juce::Identifier nameAttr  ("name");
    const juce::Identifier currentId ("presetIndex");
}

PresetManager::PresetManager (VintageGuitarSuiteAudioProcessor& p) : processor (p)
{
    if (getFile().existsAsFile())
        readFile();
    else
        createFactoryBank();
}

juce::File PresetManager::getFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Jemini Guitar Rig")
               .getChildFile ("Presets.xml");
}

//==============================================================================
juce::String PresetManager::getName (int slot) const
{
    if (! juce::isPositiveAndBelow (slot, numSlots) || isEmpty (slot))
        return "-- EMPTY --";
    return slots[(size_t) slot].name;
}

bool PresetManager::isEmpty (int slot) const
{
    return ! juce::isPositiveAndBelow (slot, numSlots) || ! slots[(size_t) slot].state.isValid();
}

int PresetManager::getCurrent() const
{
    return (int) processor.apvts.state.getProperty (currentId, -1);
}

void PresetManager::setCurrent (int slot)
{
    processor.apvts.state.setProperty (currentId, slot, nullptr);
    loadedFingerprint = fingerprint();
}

juce::String PresetManager::fingerprint() const
{
    return processor.capturePreset().toXmlString();
}

bool PresetManager::isModified() const
{
    return getCurrent() >= 0 && fingerprint() != loadedFingerprint;
}

//==============================================================================
bool PresetManager::load (int slot)
{
    if (isEmpty (slot))
        return false;

    processor.applyPreset (slots[(size_t) slot].state);
    setCurrent (slot);
    return true;
}

void PresetManager::store (int slot, const juce::String& name)
{
    if (! juce::isPositiveAndBelow (slot, numSlots))
        return;

    auto& s = slots[(size_t) slot];
    s.name  = name.trim().isEmpty() ? juce::String ("PRESET " + juce::String (slot + 1)) : name.trim().toUpperCase().substring (0, 24);
    s.state = processor.capturePreset();
    setCurrent (slot);
    writeFile();
}

void PresetManager::rename (int slot, const juce::String& name)
{
    if (isEmpty (slot) || name.trim().isEmpty())
        return;

    slots[(size_t) slot].name = name.trim().toUpperCase().substring (0, 24);
    writeFile();
}

void PresetManager::clear (int slot)
{
    if (! juce::isPositiveAndBelow (slot, numSlots))
        return;

    slots[(size_t) slot] = {};
    if (getCurrent() == slot)
        processor.apvts.state.setProperty (currentId, -1, nullptr);
    writeFile();
}

//==============================================================================
void PresetManager::readFile()
{
    auto xml = juce::XmlDocument::parse (getFile());
    if (xml == nullptr || ! xml->hasTagName (bankTag.toString()))
        return;

    for (auto* e : xml->getChildWithTagNameIterator (presetTag.toString()))
    {
        const int slot = e->getIntAttribute (slotAttr.toString(), -1);
        if (! juce::isPositiveAndBelow (slot, numSlots))
            continue;

        if (auto* stateXml = e->getFirstChildElement())
        {
            slots[(size_t) slot].name  = e->getStringAttribute (nameAttr.toString());
            slots[(size_t) slot].state = juce::ValueTree::fromXml (*stateXml);
        }
    }
}

void PresetManager::writeFile() const
{
    juce::XmlElement bank (bankTag.toString());

    for (int i = 0; i < numSlots; ++i)
    {
        const auto& s = slots[(size_t) i];
        if (! s.state.isValid())
            continue;

        auto* e = bank.createNewChildElement (presetTag.toString());
        e->setAttribute (slotAttr.toString(), i);
        e->setAttribute (nameAttr.toString(), s.name);
        if (auto stateXml = s.state.createXml())
            e->addChildElement (stateXml.release());
    }

    const auto file = getFile();
    file.getParentDirectory().createDirectory();
    bank.writeTo (file);
}

//==============================================================================
// A few starting points (no NAM model / IR: those depend on the user's files)
void PresetManager::createFactoryBank()
{
    const auto initial = processor.capturePreset();

    auto set = [this] (const char* id, float plainValue)
    {
        if (auto* prm = processor.apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
    };

    auto make = [&] (int slot, const char* name, std::function<void()> tweak)
    {
        processor.applyPreset (initial);
        tweak();
        slots[(size_t) slot].name  = name;
        slots[(size_t) slot].state = processor.capturePreset();
        slots[(size_t) slot].state.setProperty ("factory", true, nullptr);   // keeps the user's NAM / IR files
    };

    make (0, "CLEAN", [&] { set ("revOn", 1); set ("revMix", 15); });

    make (1, "VIBE CLEAN", [&] { set ("vibeOn", 1); set ("vibeSpeed", 2.2f); set ("vibeIntensity", 75);
                                 set ("revOn", 1); set ("revMix", 20); });

    make (2, "GOLD CRUNCH", [&] { set ("odOn", 1); set ("odGain", 6); set ("odTreble", 5.5f); set ("odOutput", 6);
                                  set ("gateOn", 1); set ("revOn", 1); set ("revMix", 12); });

    make (3, "LEAD DELAY", [&] { set ("odOn", 1); set ("odGain", 7.5f); set ("odOutput", 6.5f); set ("gateOn", 1);
                                 set ("dlyOn", 1); set ("dlyTime", 420); set ("dlyFeedback", 38); set ("dlyMix", 28);
                                 set ("revOn", 1); set ("revDecay", 3.0f); set ("revMix", 18); });

    make (4, "AMBIENT SWELL", [&] { set ("dlyOn", 1); set ("dlyTime", 600); set ("dlyFeedback", 55); set ("dlyMix", 35);
                                    set ("dlyPingPong", 1); set ("dlyMod", 40);
                                    set ("revOn", 1); set ("revDecay", 8.0f); set ("revSize", 90); set ("revMix", 40); });

    make (5, "SLAP BACK", [&] { set ("dlyOn", 1); set ("dlyTime", 110); set ("dlyFeedback", 8); set ("dlyMix", 30);
                                set ("revOn", 1); set ("revMix", 10); });

    processor.applyPreset (initial);
    writeFile();
}
