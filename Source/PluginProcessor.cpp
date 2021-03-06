/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // In order to prepare filters for use, a ProcessSpec object needs to be passed to the chains
    // This ProcessSpec object will then be passed to each link in the chain
    juce::dsp::ProcessSpec spec;

    // The maximum number of samples to be processed at a given time also needs to be defined
    spec.maximumBlockSize = samplesPerBlock;

    // The number of channels also needs to be defined (mono chains can only handle one channel)
    spec.numChannels = 1;

    // The audio sample rate also needs to be defined
    spec.sampleRate = sampleRate;

    // Each chain then needs to be prepared for processing
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    // The getChainSettings method returns a ChainSettings data structure which contains all plugin parameters
    auto chainSettings = getChainSettings(apvts);

    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                                                chainSettings.peakFreq,
                                                                                chainSettings.peakQuality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    // Set up the peak filter to make audible changes when the gain is not zero
    // However, this will not make the sliders change these coefficients, as this needs to be done in the ProcessBlock
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    // Use a helper function to design a highpass Butterworth filter
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                                       sampleRate,
                                                                                                       2 * (chainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    // Looking at the designIIRHighpassHighOrderButterworthMethod method, 
    switch (chainSettings.lowCutSlope)
    {
        case Slope_12:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            break;
        case Slope_24:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[1];
            leftLowCut.setBypassed<0>(false);
            break;
        case Slope_36:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[1];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[2];
            leftLowCut.setBypassed<0>(false);
            break;
        case Slope_48:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[1];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[2];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[3];
            leftLowCut.setBypassed<0>(false);
            break;
    }

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    // Looking at the designIIRHighpassHighOrderButterworthMethod method, 
    switch (chainSettings.lowCutSlope)
    {
        case Slope_12:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            break;
        case Slope_24:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[1];
            rightLowCut.setBypassed<0>(false);
            break;
        case Slope_36:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[1];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[2];
            rightLowCut.setBypassed<0>(false);
            break;
        case Slope_48:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[1];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[2];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[3];
            rightLowCut.setBypassed<0>(false);
            break;
    }
}
    
void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // The getChainSettings method returns a ChainSettings data structure which contains all plugin parameters
    auto chainSettings = getChainSettings(apvts);

    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    // Set up the peak filter to make audible changes when the gain is not zero
    // However, this will not make the sliders change these coefficients, as this needs to be done in the ProcessBlock
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    // Use a helper function to design a highpass Butterworth filter
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                                       getSampleRate(),
                                                                                                       2 * (chainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    // Looking at the designIIRHighpassHighOrderButterworthMethod method, 
    switch (chainSettings.lowCutSlope)
    {
        case Slope_12:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            break;
        case Slope_24:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[1];
            leftLowCut.setBypassed<0>(false);
            break;
        case Slope_36:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[1];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[2];
            leftLowCut.setBypassed<0>(false);
            break;
        case Slope_48:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[1];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[2];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<0>().coefficients = *cutCoefficients[3];
            leftLowCut.setBypassed<0>(false);
            break;
    }

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    // Looking at the designIIRHighpassHighOrderButterworthMethod method, 
    switch (chainSettings.lowCutSlope)
    {
        case Slope_12:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            break;
        case Slope_24:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[1];
            rightLowCut.setBypassed<0>(false);
            break;
        case Slope_36:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[1];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[2];
            rightLowCut.setBypassed<0>(false);
            break;
        case Slope_48:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[1];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[2];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<0>().coefficients = *cutCoefficients[3];
            rightLowCut.setBypassed<0>(false);
            break;
    }

    // The left and right channels need to be extracted from the audio buffer provided by the host
    juce::dsp::AudioBlock<float> block(buffer);

    // Get audio blocks that represent the left and right audio channels
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    // Create wrappers around the audio blocks that can be passed to the processing chain
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    // Pass the contexts to the mono filter chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    // return new SimpleEQAudioProcessorEditor (*this);

    // This returns a new UI component that displays the parameters of an AudioProcessor as a simple list of sliders, combo boxes, and switches
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    // There are two ways to get parameter values from the APVTS
    //    1. Use the getValue method, which will return NORMALIZED values: apvts.getParameter("LowCut Freq")->getValue();
    //    2. Use the getRawParameterValue method, which will not return normalized values: apvts.getRawParameterValue(StringRef parameterID);
    settings.lowCutFreq         = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq        = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq           = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality        = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope        = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope       = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Add normalized range slider parameter controls to the layout
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                           "Peak Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           750.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.0f));

    // Create a string array object that will contain choices for rolloff per octave
    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    // Add choice box parameter controls to the layout
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
