#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../DSP/ChorusDSP.h"
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
ChoroborosAudioProcessor::ChoroborosAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters(*this, nullptr, juce::Identifier("Choroboros"), createParameterLayout()),
    chorusDSP(std::make_unique<ChorusDSP>()),
    feedbackCollector(std::make_unique<FeedbackCollector>())
{
}

ChoroborosAudioProcessor::~ChoroborosAudioProcessor()
{
}

//==============================================================================
const juce::String ChoroborosAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ChoroborosAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ChoroborosAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ChoroborosAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ChoroborosAudioProcessor::getTailLengthSeconds() const
{
    return 0.1; // Small tail for delay
}

int ChoroborosAudioProcessor::getNumPrograms()
{
    return 6; // Classic, Vintage, Rich, Psychedelic, Duck, and Ouroboros
}

int ChoroborosAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void ChoroborosAudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    
    currentProgram = index;
    
    // Track preset load for feedback
    if (feedbackCollector)
    {
        feedbackCollector->trackPresetLoad(index, getProgramName(index));
    }
    
    // Apply preset values
    if (index == 0) // Classic preset (defaults)
    {
        // Rate: 0.5 Hz
        if (auto* param = parameters.getParameter(RATE_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(0.5f));
        
        // Depth: 50%
        if (auto* param = parameters.getParameter(DEPTH_ID))
            param->setValueNotifyingHost(0.5f);
        
        // Offset: 90°
        if (auto* param = parameters.getParameter(OFFSET_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(90.0f));
        
        // Width: 100%
        if (auto* param = parameters.getParameter(WIDTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(1.0f));
        
        // Color: 50%
        if (auto* param = parameters.getParameter(COLOR_ID))
            param->setValueNotifyingHost(0.5f);
        
        // Engine Color: Green (0)
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(0.0f);
        
        // HQ: Off
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(0.0f);
    }
    else if (index == 1) // Vintage preset
    {
        // Rate: 0.44 Hz
        if (auto* param = parameters.getParameter(RATE_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(0.44f));
        
        // Depth: 33% (convert actual value to normalized for skewed range)
        if (auto* param = parameters.getParameter(DEPTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(DEPTH_ID).convertTo0to1(0.33f));
        
        // Offset: 110°
        if (auto* param = parameters.getParameter(OFFSET_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(110.0f));
        
        // Width: 125%
        if (auto* param = parameters.getParameter(WIDTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(1.25f));
        
        // Color: 77%
        if (auto* param = parameters.getParameter(COLOR_ID))
            param->setValueNotifyingHost(0.77f);
        
        // Engine Color: Red (2) - for 3 choices, normalized value is approximately 0.666...
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
        {
            // AudioParameterChoice with 3 options: 0=0.0-0.33, 1=0.33-0.66, 2=0.66-1.0
            // Use middle of range for index 2: ~0.833
            param->setValueNotifyingHost(0.833f);
        }
        
        // HQ: On
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
    }
    else if (index == 2) // Rich preset
    {
        // Rate: 1.1 Hz
        if (auto* param = parameters.getParameter(RATE_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(1.1f));
        
        // Depth: 30%
        if (auto* param = parameters.getParameter(DEPTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(DEPTH_ID).convertTo0to1(0.30f));
        
        // Offset: 25°
        if (auto* param = parameters.getParameter(OFFSET_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(25.0f));
        
        // Width: 150%
        if (auto* param = parameters.getParameter(WIDTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(1.5f));
        
        // Color: 90%
        if (auto* param = parameters.getParameter(COLOR_ID))
            param->setValueNotifyingHost(0.90f);
        
        // Engine Color: Red (2)
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
            param->setValueNotifyingHost(0.8f); // Same as Vintage preset (Red = 0.8 normalized)
        
        // HQ: Off
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(0.0f);
    }
    else if (index == 3) // Psychedelic preset
    {
        // Rate: 0.13 Hz
        if (auto* param = parameters.getParameter(RATE_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(0.13f));
        
        // Depth: 64%
        if (auto* param = parameters.getParameter(DEPTH_ID))
            param->setValueNotifyingHost(0.64f);
        
        // Offset: 45°
        if (auto* param = parameters.getParameter(OFFSET_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(45.0f));
        
        // Width: 145%
        if (auto* param = parameters.getParameter(WIDTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(1.45f));
        
        // Color: 66%
        if (auto* param = parameters.getParameter(COLOR_ID))
            param->setValueNotifyingHost(0.66f);
        
        // Engine Color: Purple (3) - for 4 choices, normalized value is approximately 0.875
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
        {
            // AudioParameterChoice with 4 options: 0=0.0-0.25, 1=0.25-0.5, 2=0.5-0.75, 3=0.75-1.0
            // Use middle of range for index 3: ~0.875
            param->setValueNotifyingHost(0.875f);
        }
        
        // HQ: On
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
    }
    else if (index == 4) // Duck preset
    {
        // Rate: 10.0 Hz
        if (auto* param = parameters.getParameter(RATE_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(10.0f));
        
        // Depth: 14% (old percentage) - For Purple, this means 0.14 actual depth
        // Purple maps 0-1 to 0-0.45, so to get 0.14 actual: 0.14 / 0.45 = 0.311
        if (auto* param = parameters.getParameter(DEPTH_ID))
        {
            // Set to 0.311 so that after Purple mapping (0.311 * 0.45 = 0.14), we get 14% actual depth
            param->setValueNotifyingHost(0.311f);
        }
        
        // Offset: 50°
        if (auto* param = parameters.getParameter(OFFSET_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(50.0f));
        
        // Width: 50%
        if (auto* param = parameters.getParameter(WIDTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(0.5f));
        
        // Color: 10%
        if (auto* param = parameters.getParameter(COLOR_ID))
            param->setValueNotifyingHost(0.10f);
        
        // Engine Color: Purple (3)
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
        {
            // AudioParameterChoice with 4 options: 0=0.0-0.25, 1=0.25-0.5, 2=0.5-0.75, 3=0.75-1.0
            // Use middle of range for index 3: ~0.875
            param->setValueNotifyingHost(0.875f);
        }
        
        // HQ: On
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
        
        // Mix: 100%
        if (auto* param = parameters.getParameter(MIX_ID))
            param->setValueNotifyingHost(1.0f);
    }
    else if (index == 5) // Ouroboros preset
    {
        // Rate: 2.0 Hz
        if (auto* param = parameters.getParameter(RATE_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(RATE_ID).convertTo0to1(2.0f));
        
        // Depth: 11%
        if (auto* param = parameters.getParameter(DEPTH_ID))
            param->setValueNotifyingHost(0.11f);
        
        // Offset: 33°
        if (auto* param = parameters.getParameter(OFFSET_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(OFFSET_ID).convertTo0to1(33.0f));
        
        // Width: 33%
        if (auto* param = parameters.getParameter(WIDTH_ID))
            param->setValueNotifyingHost(parameters.getParameterRange(WIDTH_ID).convertTo0to1(0.33f));
        
        // Color: 65%
        if (auto* param = parameters.getParameter(COLOR_ID))
            param->setValueNotifyingHost(0.65f);
        
        // Engine Color: Blue (1)
        if (auto* param = parameters.getParameter(ENGINE_COLOR_ID))
        {
            // AudioParameterChoice with 4 options: 0=0.0-0.25, 1=0.25-0.5, 2=0.5-0.75, 3=0.75-1.0
            // Use middle of range for index 1: ~0.375
            param->setValueNotifyingHost(0.375f);
        }
        
        // HQ: On
        if (auto* param = parameters.getParameter(HQ_ID))
            param->setValueNotifyingHost(1.0f);
        
        // Mix: 100%
        if (auto* param = parameters.getParameter(MIX_ID))
            param->setValueNotifyingHost(1.0f);
    }
}

const juce::String ChoroborosAudioProcessor::getProgramName (int index)
{
    if (index == 0)
        return "Classic";
    else if (index == 1)
        return "Vintage";
    else if (index == 2)
        return "Rich";
    else if (index == 3)
        return "Psychedelic";
    else if (index == 4)
        return "Duck";
    else if (index == 5)
        return "Ouroboros";
    return {};
}

void ChoroborosAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ChoroborosAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    // Some hosts can deliver larger blocks than the initial samplesPerBlock.
    // Use a safety ceiling to avoid any chance of buffer underruns / asserts.
    constexpr juce::uint32 safetyMaxBlock = 4096; // bump to 8192 if you want ultra-safe
    spec.maximumBlockSize = juce::jmax(static_cast<juce::uint32>(samplesPerBlock), safetyMaxBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    
    chorusDSP->prepare(spec);
}

void ChoroborosAudioProcessor::releaseResources()
{
    chorusDSP->reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChoroborosAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
  #endif
}
#endif

void ChoroborosAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateDSPParameters();
    
    juce::dsp::AudioBlock<float> block(buffer);
    chorusDSP->process(block);
}

void ChoroborosAudioProcessor::updateDSPParameters()
{
    auto rateParam = parameters.getRawParameterValue(RATE_ID);
    auto depthParam = parameters.getRawParameterValue(DEPTH_ID);
    auto offsetParam = parameters.getRawParameterValue(OFFSET_ID);
    auto widthParam = parameters.getRawParameterValue(WIDTH_ID);
    auto colorParam = parameters.getRawParameterValue(COLOR_ID);
    auto engineColorParam = parameters.getRawParameterValue(ENGINE_COLOR_ID);
    auto hqParam = parameters.getRawParameterValue(HQ_ID);
    
    if (rateParam) chorusDSP->setRate(rateParam->load());
    if (depthParam) chorusDSP->setDepth(depthParam->load());
    if (offsetParam) chorusDSP->setOffset(offsetParam->load());
    if (widthParam) chorusDSP->setWidth(widthParam->load());
    if (colorParam) chorusDSP->setColor(colorParam->load());
    
    if (engineColorParam)
    {
        const int colorIndex = static_cast<int>(engineColorParam->load());
        chorusDSP->setEngineColor(colorIndex);
    }
    
    if (hqParam) chorusDSP->setQualityEnabled(hqParam->load() >= 0.5f);
    
    auto mixParam = parameters.getRawParameterValue(MIX_ID);
    if (mixParam) chorusDSP->setMix(mixParam->load());
}

//==============================================================================
bool ChoroborosAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ChoroborosAudioProcessor::createEditor()
{
    return new ChoroborosPluginEditor (*this);
}

//==============================================================================
void ChoroborosAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ChoroborosAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout ChoroborosAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Rate: 0.01 Hz → 10.0 Hz
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RATE_ID, "Rate",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.5f), // Skewed for more resolution at low end
        0.5f  // Default: 0.5 Hz
    ));
    
    // Depth: 0% → 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        DEPTH_ID, "Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 0.7f), // Slightly non-linear
        0.5f
    ));
    
    // Offset: 0° → 180°
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        OFFSET_ID, "Offset",
        juce::NormalisableRange<float>(0.0f, 180.0f, 1.0f),
        90.0f
    ));
    
    // Width: 0% → 200%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        WIDTH_ID, "Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f
    ));
    
    // Color: 0% → 100% (tone/saturation parameter)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        COLOR_ID, "Color",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));
    
    // Engine Color: 0=Green, 1=Blue, 2=Red, 3=Purple
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ENGINE_COLOR_ID, "Engine Color",
        juce::StringArray { "Green", "Blue", "Red", "Purple" },
        0 // Default: Green
    ));
    
    // HQ: High quality mode (false=Normal, true=HQ)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        HQ_ID, "HQ", false
    ));
    
    // Mix: Dry/wet mix (0.0 = dry, 1.0 = wet)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f  // Default: 50% wet
    ));
    
    return { params.begin(), params.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChoroborosAudioProcessor();
}
