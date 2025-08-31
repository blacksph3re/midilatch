/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <set>
#include <unordered_map>
#include <optional>

class ActiveNote
{
private:
    int note;
    int channel;
    float velocity;
 
public:
    ActiveNote(const int _note, const int _channel, const float _velocity)
        : note(_note), channel(_channel), velocity(_velocity)
    {}
    
    const int getNote() const { return note; }
    const int getChannel() const { return channel; }
    const float getVelocity() const { return velocity; }
    const juce::MidiMessage makeNoteOn() const { return juce::MidiMessage::noteOn(channel, note, velocity); }
    const juce::MidiMessage makeNoteOff() const { return juce::MidiMessage::noteOff(channel, note); }
 
    // Define an arbitrary comparison so it can be used in a set
    // Ignore the velocity as it does not make a note unique
    bool operator<(const ActiveNote& other) const
    {
        return (note < other.note) ||
               (note == other.note && channel < other.channel);
    }
};


class MidiMapping
{
private:
    // Stores a program change number as key and a vector of notes as value
    std::map<int, std::vector<int>> noteMap;
public:
    MidiMapping();
    
    void assign(const int programChange, std::vector<int> notes) { noteMap[programChange] = notes;}
    const bool contains(const int programChange) const { return noteMap.contains(programChange); }
    const std::vector<int> getNotes(const int programChange) const;
    const std::string getDisplayText() const;
    const std::string getStringSerialization() const;
    void parseStringSerialization(std::string text);
    const std::size_t size() const { return noteMap.size();}
    void transpose(int offset);
};


//==============================================================================
/**
*/
class MidilatchAudioProcessor  : public juce::AudioProcessor
{
    public:
    //==============================================================================
    MidilatchAudioProcessor();
    ~MidilatchAudioProcessor() override;
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
    
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    const juce::String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    std::string getInfo() const;
    
    MidiMapping& getMidiMapping() {return midiMapping;}
    
    const bool getStoring() const {return isStoring;}
    void toggleStoring() {isStoring=!isStoring;}
    void setRepaintFn(std::function<void()> newRepaint) {repaint=newRepaint;}
    
    private:
    std::vector<juce::MidiMessage> activateNote(int note, int channel, float velocity);
    std::vector<juce::MidiMessage> clearActiveNotes();
    void tryRepaint() { if(this->repaint.has_value()) { this->repaint.value()();} }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidilatchAudioProcessor)
    std::set<ActiveNote> activeNotes;
    MidiMapping midiMapping;
    bool isRecording;
    bool isStoring;
    std::optional<std::function<void()>> repaint;
};
