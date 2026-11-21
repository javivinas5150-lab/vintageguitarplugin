#pragma once

#include <JuceHeader.h>

//==============================================================================
// Vintage Guitar Suite - Phase 1
// Basic plugin: input gain -> (future modules go here) -> output gain
//==============================================================================
class VintageGuitarSuiteAudioProcessor : public juce::AudioProcessor
{
public:
    VintageGuitarSuiteAudioProcessor();
    ~VintageGuitarSuiteAudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram (int) override                     {}
    const juce::String getProgramName (int) override          { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    // Saves / restores knob positions with the Cubase project
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Holds all plugin parameters (the editor connects its knobs to this)
    juce::AudioProcessorValueTreeState apvts;

private:
    // Raw pointers to the parameter values (in dB), read on the audio thread
    std::atomic<float>* inputGainDb  = nullptr;
    std::atomic<float>* outputGainDb = nullptr;

    // Smoothing avoids clicks/zipper noise when a knob is moved
    juce::SmoothedValue<float> inputGain;
    juce::SmoothedValue<float> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VintageGuitarSuiteAudioProcessor)
};
