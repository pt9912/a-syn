#include "PluginEditor.h"

AnalogSynthAudioProcessorEditor::AnalogSynthAudioProcessorEditor(AnalogSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 300);
}

AnalogSynthAudioProcessorEditor::~AnalogSynthAudioProcessorEditor()
{
}

void AnalogSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Analog Synth", getLocalBounds(), juce::Justification::centred, 1);
}

void AnalogSynthAudioProcessorEditor::resized()
{
}
