#pragma once

#include <JuceHeader.h>
#include "Modules/NamAmp.h"
#include "Modules/ToneStack.h"

//==============================================================================
// Vintage Guitar Suite - Phase 2
// Chain: input gain -> NAM amp -> (future modules) -> output gain
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

    //==========================================================================
    // NAM amp (call from the message thread). Returns "" on success.
    juce::String loadNamModel (const juce::File& file);
    void clearNamModel();
    const NamAmp& getNamAmp() const { return namAmp; }

private:
    // Raw pointers to the parameter values, read on the audio thread
    std::atomic<float>* inputGainDb  = nullptr;
    std::atomic<float>* outputGainDb = nullptr;
    std::atomic<float>* ampBypass    = nullptr;
    std::atomic<float>* ampGainDb    = nullptr;
    std::atomic<float>* ampBassDb    = nullptr;
    std::atomic<float>* ampMiddleDb  = nullptr;
    std::atomic<float>* ampTrebleDb  = nullptr;
    std::atomic<float>* ampVolumeDb  = nullptr;

    NamAmp namAmp;
    ToneStack toneStack;
    juce::SmoothedValue<float> ampGain, ampVolume;

    // Smoothing avoids clicks/zipper noise when a knob is moved
    juce::SmoothedValue<float> inputGain;
    juce::SmoothedValue<float> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VintageGuitarSuiteAudioProcessor)
};
