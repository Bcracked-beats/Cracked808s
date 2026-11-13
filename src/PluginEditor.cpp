#include "PluginEditor.h"
#include "UI/PresetBrowser.h"
#include "UI/MeterState.h"
#include "UI/EditorLayout.h"
using namespace cracked::ui;
Cracked808sAudioProcessorEditor::Cracked808sAudioProcessorEditor(Cracked808sAudioProcessor& p):AudioProcessorEditor(&p),processor(p),spectrum(p),advanced(p){
    setLookAndFeel(&theme);addAndMakeVisible(content);
    auto add=[&](juce::Component& c){content.addAndMakeVisible(c);};
    add(spectrum);add(advanced);advanced.setVisible(false);
    const char* ids[]={"drive","color","mix","width","input","output"};const char* names[]={"Drive","Tone","Saturation mix","Stereo width","Input","Output"};
    for(size_t i=0;i<6;++i){add(knobs[i]);auto& k=knobs[i];k.setBufferedToImage(true);k.format=(int)i;k.getProperties().set("bipolar",i==1||i>=4);attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,ids[i],k);auto* param=p.parameters.getParameter(ids[i]);k.defaultValue=param->convertFrom0to1(param->getDefaultValue());k.setTooltip(juce::String(names[i])+" | Shift: fine | Double-click: value | Right-click: reset");
        k.textFromValueFunction=[i](double v){if(i==0||i>=4)return juce::String(v,1)+" dB";if(i==1)return v<-.01?"Dark "+juce::String((int)(-v*100)):v>.01?"Bright "+juce::String((int)(v*100)):"Neutral";return juce::String((int)std::round(v*100))+" %";};
        k.valueFromTextFunction=[i](const juce::String& t){double v=t.retainCharacters("0123456789.-").getDoubleValue();if(i==2||i==3)v/=100.;if(i==1){v/=100.;if(t.containsIgnoreCase("dark"))v=-std::abs(v);}return v;};
    }
    for(auto* b:{&preset,&previous,&next,&save,&ab,&copy,&undo,&redo,&settings,&power})add(*b);
    preset.onClick=[this]{auto browser=std::make_unique<cracked::ui::PresetBrowser>(processor);browser->setLookAndFeel(&theme);juce::CallOutBox::launchAsynchronously(std::move(browser),preset.getBounds(),&content);};
    previous.onClick=[this]{presetStep(-1);};next.onClick=[this]{presetStep(1);};save.onClick=[this]{menu();};
    ab.onClick=[this]{processor.toggleAB();};copy.onClick=[this]{processor.copyAB();};undo.onClick=[this]{processor.undo.undo();};redo.onClick=[this]{processor.undo.redo();};
    settings.onClick=[this]{showAdvanced(!expanded);};
    power.setClickingTogglesState(true);powerAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"bypass",power);power.setTooltip("Global bypass | host compensated 256 samples");
    add(low);add(mono);lowAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"lowEnabled",low);monoAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"monoSafe",mono);
    low.setTooltip("Exclude low frequencies from added stereo widening");mono.setTooltip("Extra high-pass protection on generated side energy");
    add(lowCut);lowCut.setSliderStyle(juce::Slider::LinearHorizontal);lowCut.setTextBoxStyle(juce::Slider::TextBoxRight,false,58,18);lowCut.setTextValueSuffix(" Hz");cutAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"monoBass",lowCut);
    add(pan);pan.setName("Pan");pan.setTitle("Pan");pan.setSliderStyle(juce::Slider::LinearHorizontal);pan.setTextBoxStyle(juce::Slider::TextBoxBelow,false,64,20);pan.setDoubleClickReturnValue(true,0);
    panAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"balance",pan);
    pan.textFromValueFunction=[](double value){return std::abs(value)<.0005?juce::String("C"):juce::String(value<0?"L ":"R ")+juce::String((int)std::round(std::abs(value)*100));};
    pan.valueFromTextFunction=[](const juce::String& value){if(value.trim().equalsIgnoreCase("C")||value.containsIgnoreCase("center"))return 0.;double amount=value.retainCharacters("0123456789.-").getDoubleValue()/100.;return value.containsIgnoreCase("L")?-std::abs(amount):value.containsIgnoreCase("R")?std::abs(amount):amount;};
    pan.updateText();pan.setTooltip("Pan the complete output left/right. Center keeps the original level. Double-click: center. Requires a stereo output; Mono Check folds the result to mono.");
    add(visualMode);visualMode.addItemList({"SPECTRUM","PARTICLES","IMAGE"},1);visualModeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"visualMode",visualMode);visualMode.setTooltip("Choose an audio visualizer. Bands editing temporarily shows the spectrum.");
    add(importImage);add(clearImage);importImage.onClick=[this]{spectrum.choosePng();};clearImage.onClick=[this]{processor.clearVisualPng();spectrum.update();};
    importImage.setTooltip("Import a PNG or drop one onto the visualizer. It is embedded in your saved state.");clearImage.setTooltip("Restore your original brand mark for Image mode");
    add(visualSettings);visualSettings.onClick=[this]{showVisualSettings();};visualSettings.setTooltip("Open Advanced > Visuals for movement, glow, particles and optional colors");
    add(popOut);popOut.setName("Visualizer only");popOut.setTooltip("Shrink this plugin panel to its visualizer. No separate window. Resize freely, then use Full UI to return.");
    popOut.onClick=[this]{showVisualizerPopout(true);};
    addChildComponent(fullUI);fullUI.setName("Return to full controls");fullUI.setAlwaysOnTop(true);fullUI.onClick=[this]{showVisualizerPopout(false);};
    add(visualAmount);visualAmount.setName("Visual reaction strength");visualAmount.setTitle("Visual reaction strength");visualAmount.setSliderStyle(juce::Slider::LinearHorizontal);visualAmount.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);visualAmount.setTooltip("Visual reaction strength; does not change audio");visualAmountAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"visualAmount",visualAmount);
    advanced.onPage=[this](int page){spectrum.editBands=!visualOnly&&expanded&&page==0;};
    content.addChildComponent(updateNotice);updateNotice.setTooltip("A newer public release is available. Open the official download page; close your DAW before installing.");
    const auto openUpdate=[] {juce::URL(cracked::updates::pageUrl).launchInDefaultBrowser();};
    updateNotice.onClick=openUpdate;advanced.updatePage.onClick=openUpdate;
    advanced.autoUpdate.setToggleState(updateClient.automatic(),juce::dontSendNotification);
    advanced.autoUpdate.onClick=[this]{if(!updateClient.setAutomatic(advanced.autoUpdate.getToggleState()))advanced.autoUpdate.setToggleState(updateClient.automatic(),juce::dontSendNotification);else if(updateClient.automatic())updateClient.start(false);advanced.setUpdateStatus(updateClient.status(),updateClient.available(),updateClient.checking());};
    advanced.updateCheck.onClick=[this]{updateClient.start(true);advanced.setUpdateStatus(updateClient.status(),updateClient.available(),updateClient.checking());};
    advanced.setUpdateStatus(updateClient.status(),updateClient.available(),updateClient.checking());
    content.setLookAndFeel(&theme);setResizable(true,true);configureFullSize({1120,840});startTimerHz(30);
}
Cracked808sAudioProcessorEditor::~Cracked808sAudioProcessorEditor(){stopTimer();content.setLookAndFeel(nullptr);setLookAndFeel(nullptr);}
void Cracked808sAudioProcessorEditor::configureFullSize(juce::Point<int> wanted){
    const int baseWidth=expanded?1568:1120;
    int workWidth=baseWidth*2+32,workHeight=1740;
    if(auto* display=juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())){workWidth=display->userArea.getWidth();workHeight=display->userArea.getHeight();}
    const auto minimum=fitFullEditorSize(baseWidth,workWidth,workHeight,.75f);
    const auto fitted=fitFullEditorSize(baseWidth,workWidth,workHeight,std::min(wanted.x/(float)baseWidth,wanted.y/840.f));
    setResizeLimits(minimum.width,minimum.height,baseWidth*2,1680);getConstrainer()->setFixedAspectRatio(baseWidth/840.);setSize(fitted.width,fitted.height);
}
void Cracked808sAudioProcessorEditor::showAdvanced(bool show){
    if(visualOnly)showVisualizerPopout(false);
    const float previousWidth=expanded?1568.f:1120.f,oldScale=getWidth()/previousWidth;
    expanded=show;advanced.setVisible(show);spectrum.editBands=show&&advanced.page==0;settings.setButtonText(show?"COLLAPSE":"ADVANCED");
    configureFullSize({juce::roundToInt((show?1568.f:1120.f)*oldScale),juce::roundToInt(840*oldScale)});resized();repaint();
}
void Cracked808sAudioProcessorEditor::showVisualSettings(){advanced.page=5;advanced.rebuild();showAdvanced(true);}
void Cracked808sAudioProcessorEditor::showVisualizerPopout(bool show){
    if(show==visualOnly)return;visualOnly=show;
    if(show){fullSize={getWidth(),getHeight()};spectrum.editBands=false;content.setVisible(false);addAndMakeVisible(spectrum);fullUI.setVisible(true);setResizeLimits(320,200,3840,2160);getConstrainer()->setFixedAspectRatio(0);setSize(720,420);}
    else{content.addAndMakeVisible(spectrum);content.setVisible(true);fullUI.setVisible(false);spectrum.editBands=expanded&&advanced.page==0;configureFullSize(fullSize);}
    resized();repaint();
}
void Cracked808sAudioProcessorEditor::resized(){
    if(visualOnly){spectrum.setBounds(getLocalBounds());fullUI.setBounds(getWidth()-94,8,84,23);return;}
    const float baseWidth=expanded?1568.f:1120.f;scale=std::min((float)getWidth()/baseWidth,(float)getHeight()/840.f);content.setBounds(0,0,(int)baseWidth,840);content.setTransform(juce::AffineTransform::scale(scale));
    spectrum.setBounds(128,136,864,296);visualMode.setBounds(166,108,131,23);importImage.setBounds(307,108,102,23);clearImage.setBounds(415,108,59,23);visualAmount.setBounds(668,109,133,22);
    visualSettings.setBounds(486,108,88,23);
    popOut.setBounds(943,76,80,20);
    preset.setBounds(486,35,224,34);previous.setBounds(711,35,29,34);next.setBounds(741,35,29,34);save.setBounds(790,35,43,34);ab.setBounds(843,35,45,34);copy.setBounds(895,35,42,34);settings.setBounds(943,35,80,34);power.setBounds(1040,35,45,34);
    undo.setBounds(790,76,43,20);redo.setBounds(843,76,45,20);
    constexpr int centres[]={112,278,444,655,857,1014};for(size_t i=0;i<6;++i)knobs[i].setBounds(centres[i]-82,484,164,190);
    low.setBounds(607,699,39,24);mono.setBounds(693,699,39,24);lowCut.setBounds(587,751,151,22);
    pan.setBounds(810,730,263,45);
    advanced.setBounds(1124,24,420,792);
    updateNotice.setBounds(810,797,263,22);
}
void Cracked808sAudioProcessorEditor::timerCallback(){
    const auto showing=[](juce::Component& component){auto* peer=component.getPeer();return peer&&component.isShowing()&&peer->isShowing();};
    const bool mainVisible=showing(*this);spectrum.setAnalysisEnabled(mainVisible);if(!mainVisible)return;
    spectrum.update();bool metersChanged=false;
    for(size_t i=0;i<4;++i){const auto oldMeter=meters[i];const auto oldClip=clip[i];float p=processor.analysis.peaks[i].exchange(0);meters[i]=std::max(p,meters[i]*.88f);if(p>=.998f)clip[i]=90;else clip[i]=std::max(0,clip[i]-1);metersChanged|=meterVisualChanged(oldMeter,meters[i],oldClip,clip[i]);}
    int target=processor.parameters.getRawParameterValue("fps")->load()>.5f?60:30;if(fps!=target){fps=target;startTimerHz(fps);}
    const auto updateNow=juce::Time::getMillisecondCounterHiRes();
    if(updateNow-lastUpdateUi>=1000){lastUpdateUi=updateNow;updateClient.poll();advanced.autoUpdate.setToggleState(updateClient.automatic(),juce::dontSendNotification);advanced.setUpdateStatus(updateClient.status(),updateClient.available(),updateClient.checking());updateNotice.setVisible(!visualOnly&&updateClient.available());}
    if(visualOnly)return;
    preset.setButtonText(processor.getPresetName().toUpperCase());ab.setButtonText(processor.isB()?"B / A":"A / B");undo.setEnabled(processor.undo.canUndo());redo.setEnabled(processor.undo.canRedo());power.setButtonText(power.getToggleState()?"OFF":"ON");
    const bool imageMode=processor.parameters.getRawParameterValue("visualMode")->load()>1.5f&&!spectrum.editBands;importImage.setVisible(imageMode);clearImage.setVisible(imageMode);visualMode.setEnabled(!spectrum.editBands);
    visualAmount.setVisible(processor.parameters.getRawParameterValue("visualMode")->load()>.5f&&!spectrum.editBands);
    float intensity=processor.parameters.getRawParameterValue("glow")->load();bool reduced=processor.parameters.getRawParameterValue("reducedMotion")->load()>.5f;
    float activity[]={std::max(0.f,meters[2]-meters[0])+.2f*meters[2],meters[2]*.15f,0,processor.analysis.side.load(),meters[0],meters[2]};
    for(size_t i=0;i<6;++i){const float target=reduced?0.f:juce::jlimit(0.f,1.f,activity[i]);auto& knobProperties=knobs[i].getProperties();if(std::abs((float)knobProperties.getWithDefault("activity",-1.f)-target)>.005f||std::abs((float)knobProperties.getWithDefault("glow",-1.f)-intensity)>.001f){knobProperties.set("activity",target);knobProperties.set("glow",intensity);knobs[i].repaint();}}
    if(expanded){advanced.refresh();if(advanced.page==3)advanced.repaint(16,advanced.getHeight()-34,advanced.getWidth()-32,22);}
    auto dirty=[&](juce::Rectangle<float> area){repaint(area.transformedBy(juce::AffineTransform::scale(scale)).getSmallestIntegerContainer());};
    if(metersChanged){dirty({20,108,110,328});dirty({1016,108,90,328});}
    const auto limiter=processor.analysis.limiterGain.load();const int safety=(int)processor.parameters.getRawParameterValue("protect")->load();if(std::abs(limiter-displayedLimiter)>.001f||safety!=displayedSafety){displayedLimiter=limiter;displayedSafety=safety;dirty({818,108,171,23});}
}
void Cracked808sAudioProcessorEditor::drawMeter(juce::Graphics& g,float x,bool output){
    const int first=output?2:0;constexpr float top=140,bottom=405,height=bottom-top;
    text(g,output?"OUT":"IN",{x-10,109,64,20},11,.8f,.2f);
    for(int c=0;c<2;++c){const int segments=meterSegments(meters[(size_t)(first+c)]);float mx=x+(float)c*17;
        g.setColour(silver().withAlpha(.15f));g.drawRoundedRectangle(mx,top,11,height,4,.7f);
        for(int j=0;j<45;++j){float yy=bottom-4.f-(float)j*(height-8)/45;bool on=j<segments;g.setColour(silver().withAlpha(on?.2f+.65f*(float)j/45:.025f));g.fillRect(mx+2,yy,7.f,4.f);}
        g.setColour(silver().withAlpha(clip[(size_t)(first+c)]>0?.95f:.08f));g.fillRoundedRectangle(mx,132,11,3,1);
    }
    for(int db=0;db>=-60;db-=12){float y=bottom+(float)db/60*height;text(g,juce::String(-60-db),{x+32,y-8,31,16},9,.6f,0);}text(g,"dB",{x-2,bottom+11,35,15},9,.6f,.05f);
}
void Cracked808sAudioProcessorEditor::paint(juce::Graphics& g){
    g.fillAll(juce::Colours::black);if(visualOnly)return;g.addTransform(juce::AffineTransform::scale(scale));
    auto shell=juce::Rectangle<float>(10,10,expanded?1548.f:1100.f,820);g.setGradientFill(juce::ColourGradient(juce::Colour(0xff030303),0,0,juce::Colours::black,1120,840,false));g.fillRoundedRectangle(shell,14);
    juce::Random stars(808);for(int i=0;i<165;++i){float x=20+stars.nextFloat()*1080,y=100+stars.nextFloat()*700;float a=.025f+stars.nextFloat()*.07f;g.setColour(silver().withAlpha(a));g.fillEllipse(x,y,.5f+stars.nextFloat(),.5f+stars.nextFloat());}
    // Leave the analyzer uncluttered; the supplied brand mark carries the space theme.
    g.setGradientFill(juce::ColourGradient(silver().withAlpha(.035f),560,220,juce::Colours::transparentWhite,560,440,true));g.fillRect(155,111,850,300);
    for(int layer=0;layer<4;++layer){juce::Path horizon;horizon.startNewSubPath(11,707+(float)layer*7);for(int x=11;x<1110;x+=8){float y=770.f+std::sin((float)x*.0034f)*33.f+std::sin((float)x*.017f+layer)*1.5f-(float)layer*2.f;horizon.lineTo((float)x,y);}g.setColour(silver().withAlpha(.024f-(float)layer*.004f));g.strokePath(horizon,juce::PathStrokeType(layer==0?1.f:.5f));}
    g.setColour(silver().withAlpha(.18f));g.drawRoundedRectangle(shell,14,.7f);
    logo(g,{35,30,72,48});g.setColour(silver().withAlpha(.97f));g.setFont(font(22,.16f));g.drawText("CRACKED808S",juce::Rectangle<float>(128,30,310,31),juce::Justification::centredLeft);
    g.setColour(silver().withAlpha(.66f));g.setFont(font(9,.28f));g.drawText("808 PROCESSOR",juce::Rectangle<float>(128,66,310,17),juce::Justification::centredLeft);
    drawMeter(g,35,false);drawMeter(g,1030,true);
    const float gain=processor.analysis.limiterGain.load();
    const auto safety=processor.parameters.getRawParameterValue("protect")->load()>.5f?"LIMIT  "+juce::String(juce::Decibels::gainToDecibels(std::max(gain,.00001f)),1)+" dB":"SAFETY OFF";
    text(g,safety,{818,108,171,23},9,gain<.98f?.95f:.5f,.05f);
    if(visualAmount.isVisible())text(g,"REACTION",{587,108,75,23},8,.5f,.06f);
    const char* groups[]={"SATURATION","WIDENING","I / O"};const float xs[]={30,570,786},ws[]={500,177,302};for(int i=0;i<3;++i){float centre=xs[i]+ws[i]*.5f;text(g,groups[i],{centre-68,446,136,20},11,.8f,.21f);g.setColour(silver().withAlpha(.14f));g.drawHorizontalLine(456,xs[i],centre-75);g.drawHorizontalLine(456,centre+75,xs[i]+ws[i]);}
    g.setColour(silver().withAlpha(.13f));g.drawVerticalLine(548,459,746);g.drawVerticalLine(770,459,746);
    constexpr int centres[]={112,278,444,655,857,1014};const char* names[]={"DRIVE","TONE","MIX","WIDTH","INPUT","OUTPUT"};for(int i=0;i<6;++i)text(g,names[i],{(float)centres[i]-80,674,160,26},11,.92f,.18f);
    text(g,"0                       36",{49,647,126,18},8,.38f,0);text(g,"DARK                 BRIGHT",{213,647,130,18},8,.38f,0);text(g,"0                      100",{381,647,126,18},8,.38f,0);text(g,"0                      200",{592,647,126,18},8,.38f,0);
    text(g,"LOW BYPASS",{575,726,99,18},9,.64f,.08f);text(g,"MONO SAFE",{662,726,99,18},9,.64f,.08f);
    text(g,"PAN",{810,704,263,20},10,.8f,.18f);
    g.setColour(silver().withAlpha(.13f));g.drawHorizontalLine(790,45,520);g.drawHorizontalLine(790,600,1075);logo(g,{538,775,44,29});text(g,"CRACKED VAULT",{393,810,334,15},8,.48f,.2f);
}
void Cracked808sAudioProcessorEditor::presetStep(int direction){auto names=processor.getFactoryPresetNames();names.addArray(processor.getUserPresetNames());int i=names.indexOf(processor.getPresetName());i=(i+direction+names.size())%names.size();if(processor.getFactoryPresetNames().contains(names[i]))processor.loadFactoryPreset(names[i]);else processor.loadUserPreset(names[i]);}
void Cracked808sAudioProcessorEditor::savePreset(bool saveAs){
    if(!saveAs&&processor.getUserPresetNames().contains(processor.getPresetName())){processor.saveUserPreset(processor.getPresetName());return;}
    auto* dialog=new juce::AlertWindow("Save your vibe","Name this complete CRACKED808S state.",juce::MessageBoxIconType::NoIcon);dialog->addTextEditor("name",processor.getPresetName()=="Init"?"My 808":processor.getPresetName());dialog->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey));dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<Cracked808sAudioProcessorEditor>(this),dialog](int result){if(safe&&result==1){auto name=dialog->getTextEditorContents("name");if(!safe->processor.saveUserPreset(name))juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Could not save","Check the preset name and folder permissions.");}}),true);
}
void Cracked808sAudioProcessorEditor::menu(){juce::PopupMenu m;m.addItem(1,"Save");m.addItem(2,"Save As...");m.addItem(3,"Delete user preset",processor.getUserPresetNames().contains(processor.getPresetName()));m.showMenuAsync({},[safe=juce::Component::SafePointer<Cracked808sAudioProcessorEditor>(this)](int r){if(!safe)return;if(r==1)safe->savePreset(false);if(r==2)safe->savePreset(true);if(r==3)safe->processor.deleteUserPreset(safe->processor.getPresetName());});}
