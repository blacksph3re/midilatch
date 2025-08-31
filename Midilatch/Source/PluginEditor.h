/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class MidilatchAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MidilatchAudioProcessorEditor (MidilatchAudioProcessor&);
    ~MidilatchAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateOutputText();
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MidilatchAudioProcessor& audioProcessor;
    juce::TextButton storeChordsButton;
    juce::TextButton exportButton;
    juce::TextButton importButton;
    juce::TextButton transposeUpButton;
    juce::TextButton transposeDownButton;
    juce::TextEditor exporterTextEditor;
    juce::TextEditor outputTextEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidilatchAudioProcessorEditor)
};
