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

    return layout;
}

//==============================================================================
void VintageGuitarSuiteAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
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

    // 2) Future chain goes here (Phase 2+):
    //    Uni-Vibe -> Klon-style OD -> NAM amp -> Cabinet IR -> Delay/Reverb

    // 3) Output gain
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
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// JUCE calls this to create the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VintageGuitarSuiteAudioProcessor();
}
