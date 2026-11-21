#pragma once

#include <JuceHeader.h>
#include "Modules/NamAmp.h"
#include "Modules/ToneStack.h"
#include "Modules/NoiseGate.h"
#include "Modules/CabIR.h"
#include "Modules/Vibe.h"
#include "Modules/GoldDrive.h"
#include "Modules/StereoDelay.h"
#include "Modules/LexReverb.h"
#include "Modules/IRVerb.h"
#include "Modules/Tuner.h"
#include "PresetManager.h"
#include "Modules/Recorder.h"

//==============================================================================
// Jemini Guitar Rig
// Chain: [mono] input -> gate detect -> VIBE -> GOLD DRIVE -> NAM amp -> gate -> CAB IR
//        [stereo] -> DELAY -> REVERB -> IR VERB -> output
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
    double getTailLengthSeconds() const override;   // delay + reverb tails

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

    // NAM drive pedal (same slot as the Gold Drive). Returns "" on success.
    juce::String loadPedalModel (const juce::File& file);
    void clearPedalModel();
    const NamAmp& getPedalNam() const { return pedalNam; }

    // Chromatic tuner (GUI thread). Output is muted while the tuner is on.
    Tuner::Reading readTuner() { return tuner.analyse (tunerRef->load()); }
    bool isTunerOn() const { return tunerOn->load() > 0.5f; }

    // Which interface input the guitar is on (0 = input 1, 1 = input 2).
    // In the standalone app the selection is made on the audio device itself
    // (see StandaloneApp.cpp), which installs these two hooks.
    std::function<int()> hostGetInput;
    std::function<void (int)> hostSetInput;

    int getInputChannel() const
    {
        if (hostGetInput) return hostGetInput();
        return inputSelect->load() > 0.5f ? 1 : 0;
    }

    void setInputChannel (int index)
    {
        if (hostSetInput) { hostSetInput (index); index = 0; }   // device delivers it on channel 1

        if (auto* prm = apvts.getParameter ("inputSelect"))
            prm->setValueNotifyingHost (index > 0 ? 1.0f : 0.0f);
    }
    // Input peak since the last call (after the INPUT knob), for the signal LED
    float takeInputPeak() { return inputPeak.exchange (0.0f); }

    // Recorder (standalone app): records the processed output to WAV
    Recorder& getRecorder() { return recorder; }
    bool isStandalone() const { return wrapperType == juce::AudioProcessor::wrapperType_Standalone; }

    // Presets (message thread)
    PresetManager& getPresets() { return *presets; }
    juce::ValueTree capturePreset();                    // rig only (no look / tuner)
    void applyPreset (const juce::ValueTree& preset);   // keeps the look

    // Skin index (GUI look), saved with the project / app settings
    int getSkin() const { return (int) apvts.state.getProperty ("skin", 0); }
    void setSkin (int index) { apvts.state.setProperty ("skin", index, nullptr); }
    bool isGateClosing() const { return gateOn->load() > 0.5f && noiseGate.isGating(); }

    // Cabinet IR (message thread). Returns "" on success.
    juce::String loadCabIR (const juce::File& file);
    void clearCabIR();
    const CabIR& getCabIR() const { return cabIR; }

    // IR Verb (message thread). Returns "" on success.
    juce::String loadIRVerb (const juce::File& file);
    void clearIRVerb();
    const IRVerb& getIRVerb() const { return irVerb; }

    // Output meter: highest peak since the last call (GUI thread), then reset
    float takeOutputPeak (int channel) { return outputPeak[(size_t) juce::jlimit (0, 1, channel)].exchange (0.0f); }

    // Effect ON states for the GUI
    bool isParamOn (const juce::String& id) const
    {
        if (auto* v = apvts.getRawParameterValue (id))
            return v->load() > 0.5f;
        return false;
    }

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

    std::atomic<float>* gateOn        = nullptr;
    std::atomic<float>* gateThreshold = nullptr;
    std::atomic<float>* gateRelease   = nullptr;

    std::atomic<float>* cabBypass  = nullptr;
    std::atomic<float>* cabLowCut  = nullptr;
    std::atomic<float>* cabHighCut = nullptr;

    std::atomic<float>* vibeOn        = nullptr;
    std::atomic<float>* vibeSpeed     = nullptr;
    std::atomic<float>* vibeIntensity = nullptr;
    std::atomic<float>* vibeMode      = nullptr;

    std::atomic<float>* odOn     = nullptr;
    std::atomic<float>* odGain   = nullptr;
    std::atomic<float>* odTreble = nullptr;
    std::atomic<float>* odOutput = nullptr;
    std::atomic<float>* odType   = nullptr;   // 0 = Gold Drive, 1 = NAM pedal
    std::atomic<float>* tunerOn  = nullptr;
    std::atomic<float>* inputSelect = nullptr;
    std::atomic<float>* cabLevel    = nullptr;
    juce::SmoothedValue<float> cabGain;
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float>* tunerRef = nullptr;
    Tuner tuner;
    std::atomic<float>* pnamInput  = nullptr;
    std::atomic<float>* pnamTone   = nullptr;
    std::atomic<float>* pnamOutput = nullptr;

    std::atomic<float>* dlyOn = nullptr;
    std::atomic<float>* dlyTime = nullptr;
    std::atomic<float>* dlyFeedback = nullptr;
    std::atomic<float>* dlyTone = nullptr;
    std::atomic<float>* dlyMod = nullptr;
    std::atomic<float>* dlyMix = nullptr;
    std::atomic<float>* dlyPingPong = nullptr;

    std::atomic<float>* revOn = nullptr;
    std::atomic<float>* revDecay = nullptr;
    std::atomic<float>* revSize = nullptr;
    std::atomic<float>* revPreDelay = nullptr;
    std::atomic<float>* revTone = nullptr;
    std::atomic<float>* revMod = nullptr;
    std::atomic<float>* revMix = nullptr;

    std::atomic<float>* irvOn = nullptr;
    std::atomic<float>* irvMix = nullptr;
    std::atomic<float>* irvPreDelay = nullptr;
    std::atomic<float>* irvLowCut = nullptr;
    std::atomic<float>* irvHighCut = nullptr;
    std::atomic<float>* irvLevel = nullptr;

    StereoDelay stereoDelay;
    LexReverb reverb;
    IRVerb irVerb;
    std::vector<float> monoScratch;   // right channel stand-in on mono tracks
    std::array<std::atomic<float>, 2> outputPeak { 0.0f, 0.0f };

    void restoreFiles (bool onlyIfChanged);   // reload NAM / IR files named in the state

    std::unique_ptr<PresetManager> presets;   // created at the end of the constructor
    Recorder recorder;

    NoiseGate noiseGate;
    Vibe vibe;
    GoldDrive goldDrive;
    CabIR cabIR;
    NamAmp namAmp;
    ToneStack toneStack;
    NamAmp pedalNam;                 // NAM capture of a drive pedal
    ToneStack pedalTone;             // its TONE knob (treble shelf)
    juce::SmoothedValue<float> pedalInGain, pedalOutGain;
    juce::SmoothedValue<float> ampGain, ampVolume;

    // Smoothing avoids clicks/zipper noise when a knob is moved
    juce::SmoothedValue<float> inputGain;
    juce::SmoothedValue<float> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VintageGuitarSuiteAudioProcessor)
};
