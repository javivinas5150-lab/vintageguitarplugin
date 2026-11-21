#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VintageGuitarSuiteAudioProcessor::VintageGuitarSuiteAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    inputGainDb  = apvts.getRawParameterValue ("inputGain");
    outputGainDb = apvts.getRawParameterValue ("outputGain");
    ampBypass    = apvts.getRawParameterValue ("ampBypass");
    ampGainDb    = apvts.getRawParameterValue ("ampGain");
    ampBassDb    = apvts.getRawParameterValue ("ampBass");
    ampMiddleDb  = apvts.getRawParameterValue ("ampMiddle");
    ampTrebleDb  = apvts.getRawParameterValue ("ampTreble");
    ampVolumeDb  = apvts.getRawParameterValue ("ampVolume");
}

//==============================================================================
namespace
{
    const juce::Identifier namModelPathId ("namModelPath");
}

juce::String VintageGuitarSuiteAudioProcessor::loadNamModel (const juce::File& file)
{
    auto error = namAmp.loadModel (file);

    if (error.isEmpty())
        apvts.state.setProperty (namModelPathId, file.getFullPathName(), nullptr);

    return error;
}

void VintageGuitarSuiteAudioProcessor::clearNamModel()
{
    namAmp.clearModel();
    apvts.state.removeProperty (namModelPathId, nullptr);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
VintageGuitarSuiteAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Range: -24 dB to +24 dB, steps of 0.1 dB, default 0 dB (unity)
    const juce::NormalisableRange<float> gainRange (-24.0f, 24.0f, 0.1f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 }, "Input Gain", gainRange, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 }, "Output Gain", gainRange, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ampBypass", 1 }, "Amp Bypass", false));

    // Amp controls
    const auto addDb = [&layout] (const juce::String& id, const juce::String& name, float range)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (-range, range, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));
    };

    addDb ("ampGain",   "Amp Gain",   20.0f);  // drive into the NAM model
    addDb ("ampBass",   "Amp Bass",   10.0f);
    addDb ("ampMiddle", "Amp Middle", 10.0f);
    addDb ("ampTreble", "Amp Treble", 10.0f);
    addDb ("ampVolume", "Amp Volume", 20.0f);

    return layout;
}

//==============================================================================
void VintageGuitarSuiteAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    namAmp.prepare (sampleRate, samplesPerBlock);
    toneStack.prepare (sampleRate);

    ampGain.reset   (sampleRate, 0.05);
    ampVolume.reset (sampleRate, 0.05);
    ampGain.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain (ampGainDb->load()));
    ampVolume.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (ampVolumeDb->load()));

    // 50 ms ramp when a knob changes
    inputGain.reset  (sampleRate, 0.05);
    outputGain.reset (sampleRate, 0.05);

    inputGain.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (inputGainDb->load()));
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputGainDb->load()));
}

//==============================================================================
bool VintageGuitarSuiteAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    // Mono or stereo only
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output
    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
void VintageGuitarSuiteAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    // Clear any output channels that have no matching input
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // Read knob values (dB) and convert to linear gain
    inputGain.setTargetValue  (juce::Decibels::decibelsToGain (inputGainDb->load()));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputGainDb->load()));

    // 1) Input gain
    inputGain.applyGain (buffer, numSamples);

    // 2) (Phase 4) Uni-Vibe -> Klon-style OD will go here

    // 3) Amp: GAIN -> NAM model -> BASS/MIDDLE/TREBLE -> VOLUME
    //    NAM models are mono: process the first channel,
    //    then copy the result to the other channel(s)
    if (ampBypass->load() < 0.5f && namAmp.hasModel())
    {
        auto* amp = buffer.getWritePointer (0);

        ampGain.setTargetValue (juce::Decibels::decibelsToGain (ampGainDb->load()));
        ampGain.applyGain (amp, numSamples);

        namAmp.process (amp, numSamples);

        toneStack.setGains (ampBassDb->load(), ampMiddleDb->load(), ampTrebleDb->load());
        toneStack.process (amp, numSamples);

        ampVolume.setTargetValue (juce::Decibels::decibelsToGain (ampVolumeDb->load()));
        ampVolume.applyGain (amp, numSamples);

        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
    }

    // 4) (Phase 3, 5) Cabinet IR -> Delay/Reverb will go here

    // 5) Output gain
    outputGain.applyGain (buffer, numSamples);
}

//==============================================================================
juce::AudioProcessorEditor* VintageGuitarSuiteAudioProcessor::createEditor()
{
    return new VintageGuitarSuiteAudioProcessorEditor (*this);
}

//==============================================================================
void VintageGuitarSuiteAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VintageGuitarSuiteAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));

        // Reload the NAM model that was saved with the project
        const auto path = apvts.state.getProperty (namModelPathId).toString();

        if (path.isNotEmpty())
            namAmp.loadModel (juce::File (path)); // if missing, the amp stays empty
        else
            namAmp.clearModel();
    }
}

//==============================================================================
// JUCE calls this to create the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VintageGuitarSuiteAudioProcessor();
}
