/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>

//==============================================================================
MidilatchAudioProcessorEditor::MidilatchAudioProcessorEditor (MidilatchAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), storeChordsButton("Record"), exportButton("Export"), importButton("Import"), transposeUpButton("Transpose Up"), transposeDownButton("Transpose Down")
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (430, 500);
    
    storeChordsButton.onClick = [this]{
        this->audioProcessor.toggleStoring();
        this->storeChordsButton.setButtonText(this->audioProcessor.getStoring() ? "Recording" : "Record");
    };
    
    storeChordsButton.setSize(130, 30);
    storeChordsButton.setTopLeftPosition(10, 10);
    storeChordsButton.setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
    addAndMakeVisible(storeChordsButton);
    
    exportButton.onClick = [this]{
        this->exporterTextEditor.setText(this->audioProcessor.getMidiMapping().getStringSerialization());
    };
    exportButton.setSize(130, 30);
    exportButton.setTopLeftPosition(150, 10);
    addAndMakeVisible(exportButton);
    
    importButton.onClick = [this]{
        this->audioProcessor.getMidiMapping().parseStringSerialization(this->exporterTextEditor.getText().toStdString());
    };
    importButton.setSize(130, 30);
    importButton.setTopLeftPosition(290, 10);
    addAndMakeVisible(importButton);
    
    exporterTextEditor.setTopLeftPosition(10, 50);
    exporterTextEditor.setSize(410, 30);
    addAndMakeVisible(exporterTextEditor);
    
    transposeDownButton.onClick = [this]{
        this->audioProcessor.getMidiMapping().transpose(-1);
    };
    transposeDownButton.setSize(200, 30);
    transposeDownButton.setTopLeftPosition(10, 90);
    addAndMakeVisible(transposeDownButton);
    
    transposeUpButton.onClick = [this]{
        this->audioProcessor.getMidiMapping().transpose(1);
    };
    transposeUpButton.setSize(200, 30);
    transposeUpButton.setTopLeftPosition(220, 90);
    addAndMakeVisible(transposeUpButton);
    
    outputTextEditor.setTopLeftPosition(10, 130);
    outputTextEditor.setSize(410, 360);
    outputTextEditor.setReadOnly(true);
    outputTextEditor.setScrollbarsShown(true);
    updateOutputText();
    addAndMakeVisible(outputTextEditor);
    
    audioProcessor.setRepaintFn([this](){this->repaint();});
}

MidilatchAudioProcessorEditor::~MidilatchAudioProcessorEditor()
{
}

void MidilatchAudioProcessorEditor::updateOutputText()
{
    std::stringstream message;
    message << "Latched Notes: " << this->audioProcessor.getInfo() << "\n";
    message << "Mapped Notes: \n";
    message << this->audioProcessor.getMidiMapping().getDisplayText();
    outputTextEditor.setText(message.str());
}

//==============================================================================
void MidilatchAudioProcessorEditor::paint (juce::Graphics& g)
{
    updateOutputText();
}

void MidilatchAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
