/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

MidiMapping::MidiMapping() : noteMap()
{
}

const std::vector<int> MidiMapping::getNotes(const int programChange) const
{
    auto mapping_it = noteMap.find(programChange);
    if (mapping_it == noteMap.end())
    {
        return std::vector<int>();
    }
    return mapping_it->second;
}

const std::string MidiMapping::getDisplayText() const
{
    std::stringstream stream;
    for (auto& it : noteMap)
    {
        stream << it.first << ": ";
        bool has_previous = false;
        for (int note : it.second)
        {
            if (has_previous) { stream << ", "; }
            has_previous = true;
            stream << juce::MidiMessage::getMidiNoteName(note, true, true, 4);
        }
        stream << "\n";
    }
    return stream.str();
}

const std::string MidiMapping::getStringSerialization() const
{
    std::stringstream stream;
    for (auto& it : noteMap)
    {
        stream << it.first << ':';
        for (int note : it.second)
        {
            stream << note << ',';
        }
        stream << ';';
    }
    return stream.str();
}

void MidiMapping::parseStringSerialization(std::string text)
{
    std::map<int, std::vector<int>> oldMap = noteMap;
    try {
        noteMap.clear();
        std::stringstream buf;
        int curKey = -1;
        std::vector<int> curValues;
        for(const char& c : text)
        {
            if(c == ':')
            {
                curKey = std::stoi(buf.str());
                buf.str(std::string());
            }
            else if (c == ',')
            {
                curValues.push_back(std::stoi(buf.str()));
                buf.str(std::string());
            }
            else if (c == ';')
            {
                noteMap[curKey] = curValues;
                curValues.clear();
                buf.str(std::string());
            }
            else
            {
                buf << c;
            }
        }
        if (buf.str().size()) {
            throw "did not expect any residual string, cancelling serialization";
        }
    } catch (...) {
        noteMap = oldMap;
    }
}

void MidiMapping::transpose(int offset)
{
    std::map<int, std::vector<int>> newMap;
    for (auto& it : noteMap)
    {
        std::vector<int> newNotes;
        for (int note : it.second)
        {
            int newNote = (note + offset) % 128;
            if (newNote < 0) {newNote = 128 + newNote;}
            newNotes.push_back(newNote);
        }
        newMap[it.first] = newNotes;
    }
    noteMap = newMap;
}

//==============================================================================
MidilatchAudioProcessor::MidilatchAudioProcessor()
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
    isRecording = false;
    isStoring = false;
}

MidilatchAudioProcessor::~MidilatchAudioProcessor()
{
}

//==============================================================================
const juce::String MidilatchAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidilatchAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MidilatchAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MidilatchAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MidilatchAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidilatchAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MidilatchAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MidilatchAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MidilatchAudioProcessor::getProgramName (int index)
{
    return {};
}

void MidilatchAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MidilatchAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void MidilatchAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidilatchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

std::vector<juce::MidiMessage> MidilatchAudioProcessor::clearActiveNotes()
{
    std::vector<juce::MidiMessage> returns;
    for (const auto oldmessage : this->activeNotes)
    {
        returns.push_back(oldmessage.makeNoteOff());
    }
    this->activeNotes.clear();
    return returns;
}

std::vector<juce::MidiMessage> MidilatchAudioProcessor::activateNote(int note, int channel, float velocity)
{
    std::vector<juce::MidiMessage> returns;
    if (!this->isRecording)
    {
        // If we are not recording a chord, send a noteoff for all old active notes and clear the buffer
        auto cleared = this->clearActiveNotes();
        returns.insert(returns.begin(), cleared.begin(), cleared.end());
    }
    this->isRecording = true;
    this->activeNotes.emplace(note, channel, velocity);
    returns.emplace_back(juce::MidiMessage::noteOn(channel, note, velocity));
    return returns;
}

void MidilatchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::MidiBuffer processedMidi;
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            auto newMidiEvents = this->activateNote(message.getNoteNumber(), message.getChannel(), message.getFloatVelocity());
            for(const auto& midiEvent: newMidiEvents)
            {
                processedMidi.addEvent(midiEvent, metadata.samplePosition);
            }
            tryRepaint();
        }
        else if (message.isNoteOff())
        {
            this->isRecording = false;
            // Do not add the noteoff (that's the whole point of the plugin)
        }
        else if (message.isAllNotesOff())
        {
            // On all notes off, clear latched notes - no need to send individual noteoff commands
            this->activeNotes.clear();
            processedMidi.addEvent(message, metadata.samplePosition);
            tryRepaint();
        }
        else if (message.isProgramChange())
        {
            // When in storing mode, the current notes are assigned to the midi map
            if (isStoring)
            {
                std::vector<int> activeNoteIds;
                for (const auto& activeNote : activeNotes)
                {
                    activeNoteIds.push_back(activeNote.getNote());
                }
                midiMapping.assign(message.getProgramChangeNumber(), activeNoteIds);
            }
            else
            {
                // Clear active notes
                for(const auto& midiEvent: this->clearActiveNotes())
                {
                    processedMidi.addEvent(midiEvent, metadata.samplePosition);
                }
                // Record the chord in the mapping and play NoteOn messages
                this->isRecording = true;
                for (int newNote : midiMapping.getNotes(message.getProgramChangeNumber()))
                {
                    for(const auto& midiEvent: this->activateNote(newNote, 0, 1.0f))
                    {
                        processedMidi.addEvent(midiEvent, metadata.samplePosition);
                    }
                }
                this->isRecording = false;
//                processedMidi.addEvent(message, metadata.samplePosition);
            }
            tryRepaint();
        }
//        else if(message.isController())
//        {
//            // Remap channel 27 to modwheel (1) because I was too stupid to reassign it directly on the board
//            if (message.getControllerNumber() == 27)
//                processedMidi.addEvent(juce::MidiMessage::controllerEvent(message.getChannel(), 1, message.getControllerValue()), metadata.samplePosition);
//            else
//                processedMidi.addEvent(message, metadata.samplePosition);
//        }
        else
        {
            processedMidi.addEvent(message, metadata.samplePosition);
        }
    }
    midiMessages.swapWith (processedMidi);
}

std::string MidilatchAudioProcessor::getInfo() const
{
    std::stringstream buf;
    bool has_previous = false;
    for (const auto note : this->activeNotes)
    {
        if (has_previous) {buf << ", ";}
        has_previous = true;
        buf << juce::MidiMessage::getMidiNoteName(note.getNote(), true, true, 4);
    }
    return buf.str();
}

//==============================================================================
bool MidilatchAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MidilatchAudioProcessor::createEditor()
{
    return new MidilatchAudioProcessorEditor (*this);
}

//==============================================================================
void MidilatchAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    std::string textMidiMapping = midiMapping.getStringSerialization();
    destData.append(textMidiMapping.c_str(), textMidiMapping.size());
}

void MidilatchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::string textMidiMapping((const char*)data, sizeInBytes);
    midiMapping.parseStringSerialization(textMidiMapping);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidilatchAudioProcessor();
}
