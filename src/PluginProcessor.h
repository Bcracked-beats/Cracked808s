#pragma once
#include <JuceHeader.h>
#include "DSP/QualityProcessor.h"
#include "Analysis/AnalysisData.h"
#include "State/ParameterReader.h"

class Cracked808sAudioProcessor final : public juce::AudioProcessor {
public:
    Cracked808sAudioProcessor();
    ~Cracked808sAudioProcessor() override = default;
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override{return parameters.getParameter("bypass");}
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override{return true;}
    const juce::String getName() const override{return "Cracked808s";}
    bool acceptsMidi() const override{return false;}
    bool producesMidi() const override{return false;}
    bool isMidiEffect() const override{return false;}
    double getTailLengthSeconds() const override{return .1;}
    int getNumPrograms() override{return 1;}
    int getCurrentProgram() override{return 0;}
    void setCurrentProgram(int) override{}
    const juce::String getProgramName(int) override{return {};}
    void changeProgramName(int,const juce::String&) override{}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    void applyState(const juce::ValueTree&);
    void setParameter(const juce::String&,float,bool gesture=true);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void loadFactoryPreset(const juce::String&);
    juce::StringArray getFactoryPresetNames()const;
    juce::String factoryCategory(int)const;
    juce::StringArray getUserPresetNames()const;
    bool saveUserPreset(juce::String);
    bool loadUserPreset(const juce::String&);
    bool deleteUserPreset(const juce::String&);
    bool setVisualPng(const juce::MemoryBlock&);
    juce::String getVisualPng()const;
    void clearVisualPng();
    void toggleAB();void copyAB();
    bool isB()const{const juce::ScopedLock lock(metadataLock);return slotB;}
    juce::String getPresetName()const{const juce::ScopedLock lock(metadataLock);return presetName;}
    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState parameters;
    cracked::AnalysisData analysis;
private:
    void process(juce::AudioBuffer<float>&,bool);
    juce::File presetFolder()const;
    juce::ValueTree snapshot();
    mutable juce::CriticalSection metadataLock;
    juce::String presetName="Init";
    cracked::ParameterReader reader;
    std::unique_ptr<cracked::QualityProcessor> engine;
    juce::ValueTree slotAState,slotBState;bool slotB=false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Cracked808sAudioProcessor)
};
