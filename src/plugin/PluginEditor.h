#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>

class AnalogSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AnalogSynthAudioProcessorEditor(AnalogSynthAudioProcessor&);
    ~AnalogSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AnalogSynthAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogSynthAudioProcessorEditor)
};
