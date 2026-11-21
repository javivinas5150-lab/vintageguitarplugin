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

    gateOn        = apvts.getRawParameterValue ("gateOn");
    gateThreshold = apvts.getRawParameterValue ("gateThreshold");
    gateRelease   = apvts.getRawParameterValue ("gateRelease");

    cabBypass  = apvts.getRawParameterValue ("cabBypass");
    cabLowCut  = apvts.getRawParameterValue ("cabLowCut");
    cabHighCut = apvts.getRawParameterValue ("cabHighCut");

    vibeOn        = apvts.getRawParameterValue ("vibeOn");
    vibeSpeed     = apvts.getRawParameterValue ("vibeSpeed");
    vibeIntensity = apvts.getRawParameterValue ("vibeIntensity");
    vibeMode      = apvts.getRawParameterValue ("vibeMode");

    odOn     = apvts.getRawParameterValue ("odOn");
    odGain   = apvts.getRawParameterValue ("odGain");
    odType     = apvts.getRawParameterValue ("odType");
    pnamInput  = apvts.getRawParameterValue ("pnamInput");
    pnamTone   = apvts.getRawParameterValue ("pnamTone");
    pnamOutput = apvts.getRawParameterValue ("pnamOutput");
    pedalNam.setLoudnessNormalisation (false);   // keep the pedal's real level
    odTreble = apvts.getRawParameterValue ("odTreble");
    odOutput = apvts.getRawParameterValue ("odOutput");

    dlyOn       = apvts.getRawParameterValue ("dlyOn");
    dlyTime     = apvts.getRawParameterValue ("dlyTime");
    dlyFeedback = apvts.getRawParameterValue ("dlyFeedback");
    dlyTone     = apvts.getRawParameterValue ("dlyTone");
    dlyMod      = apvts.getRawParameterValue ("dlyMod");
    dlyMix      = apvts.getRawParameterValue ("dlyMix");
    dlyPingPong = apvts.getRawParameterValue ("dlyPingPong");

    revOn       = apvts.getRawParameterValue ("revOn");
    revDecay    = apvts.getRawParameterValue ("revDecay");
    revSize     = apvts.getRawParameterValue ("revSize");
    revPreDelay = apvts.getRawParameterValue ("revPreDelay");
    revTone     = apvts.getRawParameterValue ("revTone");
    revMod      = apvts.getRawParameterValue ("revMod");
    revMix      = apvts.getRawParameterValue ("revMix");

    irvOn       = apvts.getRawParameterValue ("irvOn");
    irvMix      = apvts.getRawParameterValue ("irvMix");
    irvPreDelay = apvts.getRawParameterValue ("irvPreDelay");
    irvLowCut   = apvts.getRawParameterValue ("irvLowCut");
    irvHighCut  = apvts.getRawParameterValue ("irvHighCut");
    irvLevel    = apvts.getRawParameterValue ("irvLevel");
}

double VintageGuitarSuiteAudioProcessor::getTailLengthSeconds() const
{
    // How long the sound keeps ringing after you stop playing (used when Cubase renders)
    double tail = 0.0;

    if (revOn->load() > 0.5f)
        tail = juce::jmax (tail, (double) revDecay->load() + revPreDelay->load() * 0.001);

    if (dlyOn->load() > 0.5f)
    {
        // Repeats until they're ~60 dB down
        const double fb = juce::jlimit (0.01, 0.95, (double) dlyFeedback->load() / 100.0);
        const double repeats = std::ceil (-3.0 / std::log10 (fb));
        tail = juce::jmax (tail, repeats * dlyTime->load() * 0.001);
    }

    if (irvOn->load() > 0.5f && irVerb.hasIR())
        tail = juce::jmax (tail, irVerb.getIRLengthSeconds() + irvPreDelay->load() * 0.001);

    return juce::jmin (tail, 30.0);
}

//==============================================================================
namespace
{
    const juce::Identifier namModelPathId ("namModelPath");
    const juce::Identifier cabIRPathId    ("cabIRPath");
    const juce::Identifier irVerbPathId   ("irVerbPath");
    const juce::Identifier pedalNamPathId ("pedalNamPath");
}

juce::String VintageGuitarSuiteAudioProcessor::loadPedalModel (const juce::File& file)
{
    auto error = pedalNam.loadModel (file);

    if (error.isEmpty())
        apvts.state.setProperty (pedalNamPathId, file.getFullPathName(), nullptr);

    return error;
}

void VintageGuitarSuiteAudioProcessor::clearPedalModel()
{
    pedalNam.clearModel();
    apvts.state.removeProperty (pedalNamPathId, nullptr);
}

juce::String VintageGuitarSuiteAudioProcessor::loadIRVerb (const juce::File& file)
{
    auto error = irVerb.loadIR (file);

    if (error.isEmpty())
        apvts.state.setProperty (irVerbPathId, file.getFullPathName(), nullptr);

    return error;
}

void VintageGuitarSuiteAudioProcessor::clearIRVerb()
{
    irVerb.clearIR();
    apvts.state.removeProperty (irVerbPathId, nullptr);
}

juce::String VintageGuitarSuiteAudioProcessor::loadCabIR (const juce::File& file)
{
    auto error = cabIR.loadIR (file);

    if (error.isEmpty())
        apvts.state.setProperty (cabIRPathId, file.getFullPathName(), nullptr);

    return error;
}

void VintageGuitarSuiteAudioProcessor::clearCabIR()
{
    cabIR.clearIR();
    apvts.state.removeProperty (cabIRPathId, nullptr);
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

    // Noise gate
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "gateOn", 1 }, "Gate On", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gateThreshold", 1 }, "Gate Threshold",
        juce::NormalisableRange<float> (-90.0f, -20.0f, 0.5f), -60.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    juce::NormalisableRange<float> releaseRange (10.0f, 500.0f, 1.0f);
    releaseRange.setSkewForCentre (100.0f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gateRelease", 1 }, "Gate Release", releaseRange, 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    // Cabinet
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "cabBypass", 1 }, "Cab Bypass", false));

    juce::NormalisableRange<float> lowCutRange (20.0f, 400.0f, 1.0f);
    lowCutRange.setSkewForCentre (90.0f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cabLowCut", 1 }, "Cab Low Cut", lowCutRange, 20.0f,     // 20 Hz = off
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    juce::NormalisableRange<float> highCutRange (2000.0f, 20000.0f, 10.0f);
    highCutRange.setSkewForCentre (7000.0f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cabHighCut", 1 }, "Cab High Cut", highCutRange, 20000.0f, // 20 kHz = off
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    // Vibe pedal
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "vibeOn", 1 }, "Vibe On", false));

    juce::NormalisableRange<float> speedRange (0.5f, 12.0f, 0.01f);
    speedRange.setSkewForCentre (3.0f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vibeSpeed", 1 }, "Vibe Speed", speedRange, 3.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vibeIntensity", 1 }, "Vibe Intensity",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "vibeMode", 1 }, "Vibe Mode",
        juce::StringArray { "Chorus", "Vibrato" }, 0));

    // Gold Drive pedal (knobs 0..10 like the real pedal)
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "odOn", 1 }, "Drive On", false));

    const auto addKnob10 = [&layout] (const juce::String& id, const juce::String& name, float def)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), def));
    };

    addKnob10 ("odGain",   "Drive Gain",   3.0f);
    addKnob10 ("odTreble", "Drive Treble", 5.0f);
    addKnob10 ("odOutput", "Drive Output", 5.0f);

    // Drive slot type: built-in Gold Drive or a NAM capture of a drive pedal
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "odType", 1 }, "Drive Type",
        juce::StringArray { "Gold Drive", "NAM Pedal" }, 0));

    const auto addDbRange = [&layout] (const juce::String& id, const juce::String& name, float min, float max, float def)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (min, max, 0.1f), def,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));
    };

    addDbRange ("pnamInput",  "NAM Pedal Input",  -18.0f, 18.0f, 0.0f);
    addDbRange ("pnamTone",   "NAM Pedal Tone",   -12.0f, 12.0f, 0.0f);
    addDbRange ("pnamOutput", "NAM Pedal Output", -24.0f, 12.0f, 0.0f);

    // Rack: Delay
    const auto addFloat = [&layout] (const juce::String& id, const juce::String& name,
                                     float min, float max, float def, float centre,
                                     const juce::String& label)
    {
        juce::NormalisableRange<float> range (min, max, 0.01f);
        if (centre > 0.0f)
            range.setSkewForCentre (centre);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def,
            juce::AudioParameterFloatAttributes().withLabel (label)));
    };

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "dlyOn", 1 }, "Delay On", false));
    addFloat ("dlyTime",     "Delay Time",     20.0f,   2000.0f,  380.0f,  400.0f, "ms");
    addFloat ("dlyFeedback", "Delay Feedback",  0.0f,     95.0f,   35.0f,    0.0f, "%");
    addFloat ("dlyTone",     "Delay Tone",    1000.0f, 20000.0f, 6000.0f, 5000.0f, "Hz");
    addFloat ("dlyMod",      "Delay Mod",       0.0f,    100.0f,   20.0f,    0.0f, "%");
    addFloat ("dlyMix",      "Delay Mix",       0.0f,    100.0f,   25.0f,    0.0f, "%");
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "dlyPingPong", 1 }, "Delay Ping-Pong", false));

    // Rack: Reverb
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "revOn", 1 }, "Reverb On", false));
    addFloat ("revDecay",    "Reverb Decay",     0.2f,     20.0f,    2.5f,    2.5f, "s");
    addFloat ("revSize",     "Reverb Size",      0.0f,    100.0f,   60.0f,    0.0f, "%");
    addFloat ("revPreDelay", "Reverb Pre-Delay", 0.0f,    200.0f,   20.0f,   40.0f, "ms");
    addFloat ("revTone",     "Reverb Tone",    1500.0f, 16000.0f, 6000.0f, 5000.0f, "Hz");
    addFloat ("revMod",      "Reverb Mod",       0.0f,    100.0f,   40.0f,    0.0f, "%");
    addFloat ("revMix",      "Reverb Mix",       0.0f,    100.0f,   20.0f,    0.0f, "%");

    // Rack: IR Verb (convolution)
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "irvOn", 1 }, "IR Verb On", false));
    addFloat ("irvMix",      "IR Verb Mix",       0.0f,    100.0f,    30.0f,    0.0f, "%");
    addFloat ("irvPreDelay", "IR Verb Pre-Delay", 0.0f,    200.0f,     0.0f,   40.0f, "ms");
    addFloat ("irvLowCut",   "IR Verb Low Cut",  20.0f,   1000.0f,    20.0f,  150.0f, "Hz");   // 20 = off
    addFloat ("irvHighCut",  "IR Verb High Cut", 2000.0f, 20000.0f, 20000.0f, 7000.0f, "Hz");  // 20k = off
    addFloat ("irvLevel",    "IR Verb Level",   -24.0f,    12.0f,      0.0f,    0.0f, "dB");

    return layout;
}

//==============================================================================
void VintageGuitarSuiteAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    noiseGate.prepare (sampleRate, samplesPerBlock);
    vibe.prepare (sampleRate);
    goldDrive.prepare (sampleRate, samplesPerBlock);
    cabIR.prepare (sampleRate, samplesPerBlock);

    // Tell Cubase about the overdrive's oversampling delay so it compensates
    setLatencySamples (goldDrive.getLatencySamples());

    stereoDelay.prepare (sampleRate, samplesPerBlock);
    reverb.prepare (sampleRate, samplesPerBlock);
    irVerb.prepare (sampleRate, samplesPerBlock);
    monoScratch.assign ((size_t) juce::jmax (1, samplesPerBlock) * 2, 0.0f);
    namAmp.prepare (sampleRate, samplesPerBlock);
    pedalNam.prepare (sampleRate, samplesPerBlock);
    pedalTone.prepare (sampleRate);
    pedalInGain.reset  (sampleRate, 0.05);
    pedalOutGain.reset (sampleRate, 0.05);
    pedalInGain.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (pnamInput->load()));
    pedalOutGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pnamOutput->load()));
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

    // The whole guitar chain is mono: it runs on the first channel and is
    // copied to the other channel(s) at the end.
    auto* mono = buffer.getWritePointer (0);

    // 1) Input gain
    inputGain.applyGain (mono, numSamples);

    // 2) Noise gate, part 1: listen to the clean guitar (before pedals and amp)
    const bool gateEnabled = gateOn->load() > 0.5f;

    if (gateEnabled)
    {
        noiseGate.setParameters (gateThreshold->load(), gateRelease->load());
        noiseGate.analyse (mono, numSamples);
    }

    // 3) Pedals: VIBE -> GOLD DRIVE
    if (vibeOn->load() > 0.5f)
    {
        vibe.setParameters (vibeSpeed->load(), vibeIntensity->load() / 100.0f, vibeMode->load() > 0.5f);
        vibe.process (mono, numSamples);
    }

    // Drive slot: Gold Drive or NAM pedal (footswitch = odOn).
    // Gold Drive always runs (oversampling latency must stay constant);
    // when it's off or the NAM pedal is selected it just passes through clean.
    const bool driveOn  = odOn->load() > 0.5f;
    const bool namPedal = odType->load() > 0.5f;

    goldDrive.setParameters (odGain->load(), odTreble->load(), odOutput->load());
    goldDrive.process (mono, numSamples, driveOn && ! namPedal);

    pedalInGain.setTargetValue  (juce::Decibels::decibelsToGain (pnamInput->load()));
    pedalOutGain.setTargetValue (juce::Decibels::decibelsToGain (pnamOutput->load()));

    if (driveOn && namPedal && pedalNam.hasModel())
    {
        pedalInGain.applyGain (mono, numSamples);
        pedalNam.process (mono, numSamples);

        pedalTone.setGains (0.0f, 0.0f, pnamTone->load());
        pedalTone.process (mono, numSamples);

        pedalOutGain.applyGain (mono, numSamples);
    }
    else
    {
        pedalInGain.skip (numSamples);
        pedalOutGain.skip (numSamples);
    }

    // 4) Amp: GAIN -> NAM model -> BASS/MIDDLE/TREBLE -> VOLUME
    if (ampBypass->load() < 0.5f && namAmp.hasModel())
    {
        ampGain.setTargetValue (juce::Decibels::decibelsToGain (ampGainDb->load()));
        ampGain.applyGain (mono, numSamples);

        namAmp.process (mono, numSamples);

        toneStack.setGains (ampBassDb->load(), ampMiddleDb->load(), ampTrebleDb->load());
        toneStack.process (mono, numSamples);

        ampVolume.setTargetValue (juce::Decibels::decibelsToGain (ampVolumeDb->load()));
        ampVolume.applyGain (mono, numSamples);
    }

    // 5) Noise gate, part 2: silence after the amp (removes amp hiss too)
    if (gateEnabled)
        noiseGate.apply (mono, numSamples);

    // 6) Cabinet IR + Low Cut / High Cut
    if (cabBypass->load() < 0.5f)
    {
        cabIR.setFilters (cabLowCut->load(), cabHighCut->load());
        cabIR.process (mono, numSamples);
    }

    // Mono -> all channels
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    // 7) Rack effects in STEREO: DELAY -> REVERB
    //    (on a mono track they still run, and the left side is kept)
    if ((int) monoScratch.size() < numSamples)
        monoScratch.resize ((size_t) numSamples); // only if the host breaks its promised block size

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : monoScratch.data();

    if (buffer.getNumChannels() == 1)
        std::copy (left, left + numSamples, right);

    if (dlyOn->load() > 0.5f)
    {
        StereoDelay::Params p;
        p.timeMs   = dlyTime->load();
        p.feedback = dlyFeedback->load() / 100.0f;
        p.toneHz   = dlyTone->load();
        p.mod      = dlyMod->load() / 100.0f;
        p.mix      = dlyMix->load() / 100.0f;
        p.pingPong = dlyPingPong->load() > 0.5f;
        stereoDelay.setParameters (p);
        stereoDelay.process (left, right, numSamples);
    }

    if (revOn->load() > 0.5f)
    {
        LexReverb::Params p;
        p.decaySec   = revDecay->load();
        p.size       = revSize->load() / 100.0f;
        p.preDelayMs = revPreDelay->load();
        p.toneHz     = revTone->load();
        p.mod        = revMod->load() / 100.0f;
        p.mix        = revMix->load() / 100.0f;
        reverb.setParameters (p);
        reverb.process (left, right, numSamples);
    }

    if (irvOn->load() > 0.5f)
    {
        IRVerb::Params p;
        p.mix        = irvMix->load() / 100.0f;
        p.preDelayMs = irvPreDelay->load();
        p.lowCutHz   = irvLowCut->load();
        p.highCutHz  = irvHighCut->load();
        p.levelDb    = irvLevel->load();
        irVerb.setParameters (p);
        irVerb.process (left, right, numSamples);
    }

    // 8) Output gain
    outputGain.applyGain (buffer, numSamples);

    // Peak levels for the OUTPUT meter (the GUI reads and resets them)
    for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
    {
        const float peak = buffer.getMagnitude (ch, 0, numSamples);
        auto& slot = outputPeak[(size_t) ch];
        float prev = slot.load();
        while (peak > prev && ! slot.compare_exchange_weak (prev, peak)) {}
    }

    if (buffer.getNumChannels() == 1)
        outputPeak[1].store (outputPeak[0].load());
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

        // ...and the NAM drive pedal
        const auto pedalPath = apvts.state.getProperty (pedalNamPathId).toString();

        if (pedalPath.isNotEmpty())
            pedalNam.loadModel (juce::File (pedalPath));
        else
            pedalNam.clearModel();

        // ...and the cabinet IR
        const auto irPath = apvts.state.getProperty (cabIRPathId).toString();

        if (irPath.isNotEmpty())
            cabIR.loadIR (juce::File (irPath));
        else
            cabIR.clearIR();

        // ...and the IR Verb
        const auto verbPath = apvts.state.getProperty (irVerbPathId).toString();

        if (verbPath.isNotEmpty())
            irVerb.loadIR (juce::File (verbPath));
        else
            irVerb.clearIR();
    }
}

//==============================================================================
// JUCE calls this to create the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VintageGuitarSuiteAudioProcessor();
}
