#pragma once
#include "SpaceTheme.h"
#include "../PluginProcessor.h"
namespace cracked::ui {
class PresetBrowser : public juce::Component,private juce::ListBoxModel {
public:
    explicit PresetBrowser(Cracked808sAudioProcessor& p):processor(p),list("Presets",this){
        setSize(390,410);addAndMakeVisible(search);search.setTextToShowWhenEmpty("Search presets or categories...",silver().withAlpha(.35f));search.onTextChange=[this]{filter();};addAndMakeVisible(list);list.setColour(juce::ListBox::backgroundColourId,juce::Colour(0xff080808));list.setRowHeight(31);filter();
    }
    void resized()override{search.setBounds(12,12,getWidth()-24,30);list.setBounds(8,54,getWidth()-16,getHeight()-64);}
    void paint(juce::Graphics& g)override{g.fillAll(juce::Colour(0xff080808));}
private:
    struct Row {juce::String name,category;bool user;};
    void filter(){rows.clear();auto q=search.getText().trim();auto all=processor.getFactoryPresetNames();for(int i=0;i<all.size();++i){auto cat=processor.factoryCategory(i);if(q.isEmpty()||all[i].containsIgnoreCase(q)||cat.containsIgnoreCase(q))rows.push_back({all[i],cat,false});}for(auto& n:processor.getUserPresetNames())if(q.isEmpty()||n.containsIgnoreCase(q))rows.push_back({n,"User",true});list.updateContent();list.repaint();}
    int getNumRows()override{return (int)rows.size();}
    void paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected)override{if(row<0||row>=(int)rows.size())return;if(selected){g.setColour(juce::Colour(0xff242424));g.fillRect(0,0,w,h);}g.setColour(silver().withAlpha(.86f));g.setFont(font(12,.03f));g.drawText(rows[(size_t)row].name,10,0,w-145,h,juce::Justification::centredLeft);g.setColour(silver().withAlpha(.4f));g.setFont(font(9,.02f));g.drawText(rows[(size_t)row].category,w-143,0,132,h,juce::Justification::centredRight);}
    void listBoxItemClicked(int row,const juce::MouseEvent&)override{choose(row);}
    void returnKeyPressed(int row)override{choose(row);}
    void choose(int row){if(row<0||row>=(int)rows.size())return;auto p=rows[(size_t)row];if(p.user)processor.loadUserPreset(p.name);else processor.loadFactoryPreset(p.name);if(auto* call=findParentComponentOfClass<juce::CallOutBox>())call->dismiss();}
    Cracked808sAudioProcessor& processor;juce::TextEditor search;juce::ListBox list;std::vector<Row> rows;
};
}
