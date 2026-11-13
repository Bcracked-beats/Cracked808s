#pragma once
#include "PluginProcessor.h"
#include "UI/SpaceTheme.h"
#include "UI/SpectrumDisplay.h"
#include "UI/AdvancedPanel.h"
#include "Updates/UpdateClient.h"

class Cracked808sAudioProcessorEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit Cracked808sAudioProcessorEditor(Cracked808sAudioProcessor&);
    ~Cracked808sAudioProcessorEditor()override;
    void paint(juce::Graphics&)override;
    void resized()override;
    void showAdvanced(bool);
    void showVisualSettings();
    void showVisualizerPopout(bool);
private:
    void timerCallback()override;
    void savePreset(bool);
    void presetStep(int);
    void menu();
    void drawMeter(juce::Graphics&,float x,bool output);
    void configureFullSize(juce::Point<int> wanted);
    Cracked808sAudioProcessor& processor;
    cracked::ui::SpaceTheme theme;
    juce::Component content;
    cracked::ui::SpectrumDisplay spectrum;
    cracked::ui::AdvancedPanel advanced;
    cracked::updates::Client updateClient;
    juce::TextButton updateNotice{"UPDATE AVAILABLE"};double lastUpdateUi=0;
    juce::Point<int> fullSize{1120,840};
    std::array<cracked::ui::ValueKnob,6> knobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,6> attachments;
    juce::TextButton preset{"INIT"},previous{"<"},next{">"},save{"SAVE"},ab{"A / B"},copy{"COPY"},undo{"UNDO"},redo{"REDO"},settings{"ADVANCED"},power{"ON"};
    juce::ToggleButton low,mono;
    juce::ComboBox visualMode;
    juce::TextButton importImage{"IMPORT PNG"},clearImage{"RESET"},visualSettings{"CUSTOMIZE"},popOut{"VISUAL ONLY"},fullUI{"FULL UI"};juce::Slider visualAmount;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> visualModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> visualAmountAttachment;
    juce::Slider lowCut,pan;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lowAttachment,monoAttachment,powerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutAttachment,panAttachment;
    juce::TooltipWindow tooltip{this,450};
    std::array<float,4> meters{};std::array<int,4> clip{};
    bool expanded=false,visualOnly=false;float scale=1;int fps=30;
    float displayedLimiter=-1;
    int displayedSafety=-1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Cracked808sAudioProcessorEditor)
};
