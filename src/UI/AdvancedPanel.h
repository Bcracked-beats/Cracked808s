#pragma once
#include "SpaceTheme.h"
#include "../PluginProcessor.h"

namespace cracked::ui {
class ParameterControl : public juce::Component {
public:
    ParameterControl(juce::AudioProcessorValueTreeState& state,const juce::String& id,const juce::String& title,bool compact=false):label({},title){
        setName(title);addAndMakeVisible(label);label.setFont(font(10.5f,.02f));label.setColour(juce::Label::textColourId,silver().withAlpha(.7f));
        auto* p=state.getParameter(id);
        if(auto* choice=dynamic_cast<juce::AudioParameterChoice*>(p)){addAndMakeVisible(combo);combo.addItemList(choice->choices,1);ca=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state,id,combo);kind=1;}
        else if(dynamic_cast<juce::AudioParameterBool*>(p)){addAndMakeVisible(toggle);ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state,id,toggle);kind=2;}
        else {addAndMakeVisible(slider);slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,compact?38:64,20);sa=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,id,slider);}
        setTooltipFor(title);
    }
    void resized()override{label.setBounds(0,0,getWidth(),17);juce::Rectangle<int> r(0,19,getWidth(),28);slider.setBounds(r);combo.setBounds(r.reduced(0,1));toggle.setBounds(r);}
private:
    void setTooltipFor(const juce::String& t){slider.setTooltip(t+" | Shift: fine adjustment");combo.setTooltip(t);toggle.setTooltip(t);}
    int kind=0;juce::Label label;juce::Slider slider;juce::ComboBox combo;juce::ToggleButton toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
};

class AdvancedPanel : public juce::Component {
public:
    explicit AdvancedPanel(Cracked808sAudioProcessor& p):processor(p){
        addAndMakeVisible(viewport);viewport.setViewedComponent(&form,false);viewport.setScrollBarsShown(true,false);viewport.setScrollBarThickness(12);
        form.addAndMakeVisible(stepsTitle);stepsTitle.setText("SEQUENCE / 16 STEPS",juce::dontSendNotification);stepsTitle.setFont(font(10,.06f));stepsTitle.setColour(juce::Label::textColourId,silver().withAlpha(.7f));
        form.addAndMakeVisible(updateCheck);form.addAndMakeVisible(updatePage);form.addAndMakeVisible(autoUpdate);form.addAndMakeVisible(updateStatus);
        updateCheck.setButtonText("CHECK FOR UPDATES");updateCheck.setName("Check for updates");updatePage.setButtonText("DOWNLOAD PAGE");autoUpdate.setButtonText("Check daily (optional)");autoUpdate.setName("Automatic update checks");
        autoUpdate.setTooltip("Off by default. When enabled, checks the first-party version manifest at most once daily while the plugin is open. No audio or projects are sent.");
        updateStatus.setFont(font(10,.01f));updateStatus.setColour(juce::Label::textColourId,silver().withAlpha(.7f));updateStatus.setJustificationType(juce::Justification::centredLeft);
        const char* titles[]={"BANDS","CHARACTER","MODULATION","STEREO","SETTINGS","VISUALS","COLORS"};
        for(size_t i=0;i<tabs.size();++i){tabs[i].setButtonText(titles[i]);tabs[i].onClick=[this,i]{page=(int)i;rebuild();};addAndMakeVisible(tabs[i]);}
        for(int i=1;i<=6;++i)selection.addItem("Band "+juce::String(i),i);selection.setSelectedId(1);selection.onChange=[this]{selectedBand=selection.getSelectedId()-1;rebuild();};
        add.setButtonText("+ SPLIT");remove.setButtonText("- MERGE");add.onClick=[this]{changeBand(true);};remove.onClick=[this]{changeBand(false);};
        form.addAndMakeVisible(selection);form.addAndMakeVisible(add);form.addAndMakeVisible(remove);rebuild();
    }
    void paint(juce::Graphics& g)override{
        g.setColour(juce::Colour(0xff050505));g.fillRoundedRectangle(getLocalBounds().toFloat(),10);g.setColour(silver().withAlpha(.16f));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),10,1);
        const char* titles[]={"BANDS","CHARACTER","MODULATION","STEREO","SETTINGS","VISUALS","COLORS"};
        const char* hints[]={"Shape each frequency band.","Tune the selected band's character.","Add movement to the sound.","Place the sound. Protect the low end.","Quality, routing and performance.","Set the way your music moves.","Choose how colors respond to your music."};
        text(g,"ADVANCED / "+juce::String(titles[page]),{16,13,(float)getWidth()-32,22},12,.92f,.07f);
        text(g,hints[page],{16,39,(float)getWidth()-32,20},10,.55f,.01f);
        const auto footer=juce::Rectangle<float>(16,(float)getHeight()-34,(float)getWidth()-32,22);
        if(page==3)text(g,"CORRELATION  "+juce::String(processor.analysis.correlation.load(),2),footer,10,.65f,.05f);
        else text(g,"CRACKED VAULT / "+juce::String(JucePlugin_VersionString),footer,9,.4f,.08f);
    }
    void resized()override{
        const int tabWidth=std::max(60,(getWidth()-44)/3);for(size_t i=0;i<tabs.size();++i)tabs[i].setBounds(16+(int)(i%3)*(tabWidth+6),72+(int)(i/3)*29,tabWidth,24);
        viewport.setBounds(16,174,std::max(1,getWidth()-32),std::max(1,getHeight()-222));
        const int width=std::max(160,viewport.getWidth()-viewport.getScrollBarThickness()),cell=(width-14)/2,start=selection.isVisible()?44:0;
        selection.setBounds(0,0,width-198,28);add.setBounds(width-190,0,90,28);remove.setBounds(width-90,0,90,28);
        for(size_t i=0;i<controls.size();++i)controls[i]->setBounds((int)(i%2)*(cell+14),start+(int)(i/2)*62,cell,52);
        int bottom=start+(int)((controls.size()+1)/2)*62;
        stepsTitle.setBounds(0,bottom,width,22);if(!steps.empty())bottom+=28;
        const int stepWidth=(width-24)/4;for(size_t i=0;i<steps.size();++i)steps[i]->setBounds((int)(i%4)*(stepWidth+8),bottom+(int)(i/4)*54,stepWidth,48);
        if(!steps.empty())bottom+=(int)((steps.size()+3)/4)*54;form.setSize(width,std::max(viewport.getHeight(),bottom));
        if(page==4){autoUpdate.setBounds(0,bottom+8,width,28);updateCheck.setBounds(0,bottom+42,width/2-7,28);updatePage.setBounds(width/2+7,bottom+42,width/2-7,28);updateStatus.setBounds(0,bottom+76,width,40);form.setSize(width,std::max(viewport.getHeight(),bottom+124));}
    }
    std::function<void(int)> onPage;
    int selectedBand=0,page=0;
    juce::TextButton updateCheck,updatePage;juce::ToggleButton autoUpdate;juce::Label updateStatus;
    void setUpdateStatus(const juce::String& message,bool available,bool checking){hasUpdate=available;updateStatus.setText(message,juce::dontSendNotification);updateCheck.setEnabled(!checking);updatePage.setVisible(page==4&&available);}
    void refresh(){if((int)processor.parameters.getRawParameterValue("bands")->load()!=displayedBands)rebuild();}
    void rebuild(){
        controls.clear();steps.clear();int count=(int)processor.parameters.getRawParameterValue("bands")->load();displayedBands=count;selectedBand=juce::jlimit(0,count-1,selectedBand);selection.setSelectedId(selectedBand+1,juce::dontSendNotification);for(int i=1;i<=6;++i)selection.setItemEnabled(i,i<=count);
        bool isBand=page==0||page==1;selection.setVisible(isBand);add.setVisible(page==0);remove.setVisible(page==0);add.setEnabled(count<6);remove.setEnabled(count>1);stepsTitle.setVisible(page==2);
        updateCheck.setVisible(page==4);autoUpdate.setVisible(page==4);updateStatus.setVisible(page==4);updatePage.setVisible(page==4&&hasUpdate);
        for(size_t i=0;i<tabs.size();++i)tabs[i].setToggleState((int)i==page,juce::dontSendNotification);
        auto prefix="b"+juce::String(selectedBand)+"_";
        auto c=[&](juce::String id,juce::String title){auto control=std::make_unique<ParameterControl>(processor.parameters,id,title);form.addAndMakeVisible(*control);controls.push_back(std::move(control));};
        if(page==0){c(prefix+"model","DISTORTION MODEL");c(prefix+"drive","BAND DRIVE / dB");c(prefix+"mix","BAND MIX");c(prefix+"level","BAND LEVEL / dB");c(prefix+"dynamics","EXPAND < DYNAMICS > COMPRESS");c(prefix+"feedback","FEEDBACK");c(prefix+"mute","MUTE");c(prefix+"solo","SOLO");c(prefix+"bypass","BAND BYPASS");c(prefix+"midSide","MID / SIDE");if(selectedBand<count-1)c("crossover"+juce::String(selectedBand),"UPPER CROSSOVER / Hz");else if(count>1)c("crossover"+juce::String(selectedBand-1),"LOWER CROSSOVER / Hz");}
        else if(page==1){c(prefix+"model","DISTORTION MODEL");c(prefix+"bass","BASS / dB");c(prefix+"mid","MID / dB");c(prefix+"treble","TREBLE / dB");c(prefix+"presence","PRESENCE / dB");c(prefix+"feedbackHz","FEEDBACK FREQUENCY / Hz");c(prefix+"driveBalance","L-R / M-S DRIVE BALANCE");c(prefix+"levelBalance","L-R / M-S LEVEL BALANCE");c("compensate","AUTO GAIN");c("preserveSub","PRESERVE DEEP SUB");c(prefix+"tone","BAND TONE");c(prefix+"midSide","MID / SIDE");}
        else if(page==2){c("modWave","MODULATION SOURCE");c("modDest","DESTINATION");c("modRate","RATE / Hz");c("modAmount","AMOUNT / POLARITY");c("modPhase","PHASE OFFSET");c("modSync","TEMPO SYNC");c("modDivision","SYNC DIVISION");c("modAttack","FOLLOWER ATTACK / ms");c("modRelease","FOLLOWER RELEASE / ms");c("modSensitivity","FOLLOWER SENSITIVITY");for(int i=0;i<16;++i){auto s=std::make_unique<ParameterControl>(processor.parameters,"step"+juce::String(i),juce::String(i+1),true);form.addAndMakeVisible(*s);steps.push_back(std::move(s));}}
        else if(page==3){c("monoBass","LOW BYPASS FREQUENCY / Hz");c("focus","STEREO FOCUS");c("stereoMix","WIDENING MIX");c("balance","STEREO BALANCE");c("monoSafe","MONO SAFE");c("monoSub","MONO SUB");c("monoCheck","MONO CHECK");c("synthetic","GENERATE SIDE FROM MONO");c("lowEnabled","LOW BYPASS");c("width","WIDTH 0 - 200%");c("widthCompensate","WIDTH LEVEL MATCH");}
        else if(page==4){c("quality","PROCESSING QUALITY");c("routing","PROCESSING ORDER");c("globalMix","GLOBAL PLUGIN MIX");c("protect","LOOKAHEAD OUTPUT SAFETY");c("showPre","SHOW PRE SPECTRUM");c("smoothing","SPECTRUM SMOOTHING");c("reducedMotion","REDUCED MOTION");c("fps","ANIMATION QUALITY");c("glow","KNOB GLOW INTENSITY");}
        else if(page==5){c("visualMode","VISUALIZER");c("visualAmount","REACTION AMOUNT");c("visualSensitivity","AUDIO SENSITIVITY");c("visualAttack","ATTACK / ms");c("visualRelease","RELEASE / ms");c("visualMotion","MOVEMENT");c("visualMotionDepth","MOVEMENT DEPTH");c("visualSpeed","WAVE DETAIL");c("visualGlow","VISUAL GLOW BOOST");c("visualAudioGlow","AUDIO GLOW BOOST");c("visualDensity","PARTICLE DENSITY");c("visualPointSize","PARTICLE SIZE / px");c("visualScale","SPHERE / IMAGE SCALE");c("visualColor","COLOR SWAPS");c("visualPalette","COLOR PALETTE");c("visualColorSpeed","COLOR TRAVEL");c("reducedMotion","REDUCED MOTION");c("fps","ANIMATION RATE");}
        else {c("visualColor","ENABLE COLOR");c("visualColorMode","COLOR BEHAVIOR");c("visualPalette","PALETTE / FLOW ONLY");c("visualColorTransition","TRANSITION / ms");c("visualSaturation","SATURATION");c("visualBassSensitivity","BASS HIT SENSITIVITY");c("visualHitGap","MIN HIT GAP / ms");c("visualHue","START / FIXED HUE");c("visualColorSpread","HUE SPREAD / NEW MODES");c("visualColorSpeed","FLOW TRAVEL");c("reducedMotion","FREEZE MOTION + COLORS");}
        viewport.setViewPosition(0,0);resized();repaint();if(onPage)onPage(page);
    }
private:
    void changeBand(bool split){
        auto& state=processor.parameters;int count=(int)state.getRawParameterValue("bands")->load();if((split&&count>=6)||(!split&&count<=1))return;
        processor.undo.beginNewTransaction(split?"Split band":"Merge band");
        const char* suffixes[]={"drive","tone","mix","level","dynamics","feedback","feedbackHz","bass","mid","treble","presence","driveBalance","levelBalance","model","mute","solo","bypass","midSide"};
        auto copy=[&](int from,int to){for(auto suffix:suffixes){auto src="b"+juce::String(from)+"_"+suffix;auto dst="b"+juce::String(to)+"_"+suffix;processor.setParameter(dst,state.getRawParameterValue(src)->load());}};
        if(split){for(int i=count;i>selectedBand+1;--i)copy(i-1,i);copy(selectedBand,selectedBand+1);for(int i=count-1;i>selectedBand;--i)processor.setParameter("crossover"+juce::String(i),state.getRawParameterValue("crossover"+juce::String(i-1))->load());float lo=selectedBand==0?20:state.getRawParameterValue("crossover"+juce::String(selectedBand-1))->load();float hi=selectedBand==count-1?20000:state.getRawParameterValue("crossover"+juce::String(selectedBand+1))->load();processor.setParameter("crossover"+juce::String(selectedBand),std::sqrt(lo*hi));processor.setParameter("bands",(float)count+1);}
        else {int boundary=std::min(selectedBand,count-2);for(int i=selectedBand;i<count-1;++i)copy(i+1,i);for(int i=boundary;i<count-2;++i)processor.setParameter("crossover"+juce::String(i),state.getRawParameterValue("crossover"+juce::String(i+1))->load());processor.setParameter("bands",(float)count-1);}
        rebuild();
    }
    Cracked808sAudioProcessor& processor;int displayedBands=0;bool hasUpdate=false;std::array<juce::TextButton,7> tabs;juce::ComboBox selection;juce::TextButton add,remove;
    juce::Component form;juce::Viewport viewport;juce::Label stepsTitle;
    std::vector<std::unique_ptr<ParameterControl>> controls,steps;
};
}
