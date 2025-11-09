#include "PluginProcessor.h"
#include "PluginEditor.h"

AnalogSynthAudioProcessor::AnalogSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

AnalogSynthAudioProcessor::~AnalogSynthAudioProcessor()
{
}

const juce::String AnalogSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnalogSynthAudioProcessor::acceptsMidi() const
{
    return true;
}

bool AnalogSynthAudioProcessor::producesMidi() const
{
    return false;
}

bool AnalogSynthAudioProcessor::isMidiEffect() const
{
    return false;
}

double AnalogSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AnalogSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int AnalogSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AnalogSynthAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AnalogSynthAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AnalogSynthAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void AnalogSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void AnalogSynthAudioProcessor::releaseResources()
{
}

bool AnalogSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void AnalogSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

bool AnalogSynthAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AnalogSynthAudioProcessor::createEditor()
{
    return new AnalogSynthAudioProcessorEditor(*this);
}

void AnalogSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void AnalogSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogSynthAudioProcessor();
}
