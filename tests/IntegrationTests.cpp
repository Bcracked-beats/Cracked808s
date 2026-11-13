#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UI/EditorLayout.h"
#include "Updates/UpdateInfo.h"
#include "Updates/UpdateClient.h"
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <new>

thread_local bool watchAllocation=false;
thread_local size_t allocations=0;
void* operator new(std::size_t n){if(watchAllocation)++allocations;if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}

int failures=0;
void check(bool ok,const char* msg){if(!ok){++failures;std::cerr<<"FAIL: "<<msg<<'\n';}}
float get(Cracked808sAudioProcessor& p,const char* id){return p.parameters.getRawParameterValue(id)->load();}
void stateTest(){
    Cracked808sAudioProcessor p;
    p.setParameter("bands",5);p.setParameter("crossover2",2300);p.setParameter("b4_model",16);p.setParameter("b4_dynamics",.64f);p.setParameter("step9",-.66f);p.setParameter("modAmount",.34f);p.setParameter("width",1.78f);
    juce::MemoryBlock state;p.getStateInformation(state);p.loadFactoryPreset("Pure Sub");p.setStateInformation(state.getData(),(int)state.getSize());
    check(get(p,"bands")==5&&get(p,"crossover2")==2300&&get(p,"b4_model")==16&&std::abs(get(p,"b4_dynamics")-.64f)<.001&&std::abs(get(p,"step9")+.66f)<.001&&std::abs(get(p,"width")-1.78f)<.001,"state must restore bands, models, modulation and stereo");
    auto invalid=juce::ValueTree("OTHER");p.applyState(invalid);check(get(p,"bands")==5,"foreign XML must not replace valid state");
    p.setParameter("drive",7);p.copyAB();p.toggleAB();p.setParameter("drive",23);p.toggleAB();check(std::abs(get(p,"drive")-7)<.01,"A/B must restore slot A");p.toggleAB();check(std::abs(get(p,"drive")-23)<.01,"A/B must preserve slot B");
    p.getStateInformation(state);Cracked808sAudioProcessor restored;restored.setStateInformation(state.getData(),(int)state.getSize());
    check(restored.isB(),"host restore must keep the selected A/B slot");restored.toggleAB();check(std::abs(get(restored,"drive")-7)<.01,"host restore must keep the inactive A/B slot");
    juce::ValueTree legacy("CRACKED808S"),drive("PARAM");drive.setProperty("id","drive",nullptr);drive.setProperty("value",8.f,nullptr);legacy.addChild(drive,-1,nullptr);
    p.applyState(legacy);check(get(p,"bands")==1&&get(p,"modAmount")==0,"legacy presets must reset missing advanced parameters");
    p.setParameter("drive",8);p.parameters.copyState();p.undo.clearUndoHistory();p.loadFactoryPreset("Supernova");p.parameters.copyState();p.undo.undo();check(std::abs(get(p,"drive")-8)<.01,"undo must restore the audio state before factory preset load");p.undo.redo();check(std::abs(get(p,"drive")-28)<.01,"redo must reapply the factory preset audio state");
    std::cout<<"state and A/B checked\n";
}
void updatePolicy(){
    using namespace cracked::updates;
    check(newer("2.5.10","2.5.9")&&!newer("2.5.9","2.5.10")&&!newer("2.5.3","2.5.3"),"update comparison must be numeric, stable and strictly newer");
    for(const auto* v:{"2.5","2.5.4-beta","https://evil.example","02.5.4","9999999999.0.0"})check(!parseVersion(v).has_value(),"malformed or prerelease versions must not create update notices");
    check(manifestVersion(R"({"schema":1,"product":"Cracked808s","version":"2.5.3","page":"https://www.crackedvault.workers.dev/plugins/cracked808s"})")=="2.5.3","valid stable first-party metadata must parse");
    for(const auto* body:{"<html>offline</html>","{}",R"({"schema":2,"product":"Cracked808s","version":"9.0.0"})",R"({"schema":1,"product":"Other","version":"9.0.0"})",R"({"schema":1,"product":"Cracked808s","version":"9.0.0","page":"https://evil.example"})"})check(manifestVersion(body).isEmpty(),"untrusted metadata must not advertise updates");
    check(manifestVersion(juce::String::repeatedString(" ",17000)).isEmpty(),"oversized metadata must be rejected");
    const juce::int64 now=1700000000000LL;
    check(due(0,now)&&!due(now-1000,now)&&due(now-86400000,now),"automatic checks must be limited to once per day");
    const auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("Cracked808s-update-test",{},false);
    Client client(folder,folder.getChildFile("not-installed.exe"));check(!client.automatic(),"network checking must start disabled");
    check(client.setAutomatic(true),"automatic-check preference must save locally");Client restored(folder,folder.getChildFile("not-installed.exe"));check(restored.automatic(),"update preference must survive independently of presets");
    restored.start(true);check(!restored.checking()&&restored.status().containsIgnoreCase("helper"),"a missing helper must fail locally without a worker or false update");
#if JUCE_WINDOWS
    const auto fixture=juce::File::getSpecialLocation(juce::File::currentApplicationFile).getSiblingFile("Cracked808sUpdateFixture.exe");check(fixture.existsAsFile(),"update lifetime fixture must be built separately from the shipping helper");
    const auto childFolder=folder.getChildFile("child");SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};HANDLE event=CreateEventW(&security,TRUE,FALSE,nullptr);
    const auto eventValue=juce::String((juce::int64)reinterpret_cast<uintptr_t>(event));SetEnvironmentVariableW(L"CRACKED808S_TEST_EVENT",eventValue.toWideCharPointer());
    auto child=std::make_unique<Client>(childFolder,fixture);child->start(true);SetEnvironmentVariableW(L"CRACKED808S_TEST_EVENT",nullptr);
    const auto waitFile=[](const juce::File& file){const auto until=juce::Time::getMillisecondCounterHiRes()+4000;while(!file.existsAsFile()&&juce::Time::getMillisecondCounterHiRes()<until)juce::Thread::sleep(5);return file.existsAsFile();};
    check(waitFile(childFolder.getChildFile("started.txt")),"helper fixture must actually start before testing handle inheritance");
    check(WaitForSingleObject(event,0)==WAIT_TIMEOUT,"update helper must not inherit an inheritable DAW event handle");CloseHandle(event);
    const auto closing=juce::Time::getMillisecondCounterHiRes();child.reset();check(juce::Time::getMillisecondCounterHiRes()-closing<200,"destroying the client must not wait for the live helper");
    check(waitFile(childFolder.getChildFile("result.json")),"helper must finish independently after client destruction");
    const auto dailyFolder=folder.getChildFile("daily");Client daily(dailyFolder,fixture);daily.setAutomatic(true);writeResult(dailyFolder,true,"2.5.3",juce::Time::currentTimeMillis());daily.poll();check(!daily.checking(),"fresh cache must prevent an automatic request");
    writeResult(dailyFolder,true,"2.5.3",juce::Time::currentTimeMillis()-dayMs-1);juce::Thread::sleep(1050);daily.poll();check(daily.checking(),"an editor left open must check automatically when its daily cache expires");
    check(waitFile(dailyFolder.getChildFile("result.json")),"daily fixture must retain a result file");
    juce::Thread::sleep(1700);daily.poll();check(daily.available(),"a valid newer cached version must produce an update notice");
#endif
}
void mainPan(){
    Cracked808sAudioProcessor p;check(get(p,"balance")==0,"Pan must default to center for old projects");p.setParameter("balance",-.67f);juce::MemoryBlock state;p.getStateInformation(state);Cracked808sAudioProcessor restored;restored.setStateInformation(state.getData(),(int)state.getSize());check(std::abs(get(restored,"balance")+.67f)<.001f,"host state must restore Pan using the original balance ID");
    std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());juce::Slider* pan=nullptr;for(auto* parent:editor->getChildren())for(auto* child:parent->getChildren())if(child->getName()=="Pan")pan=dynamic_cast<juce::Slider*>(child);
    check(pan&&pan->isVisible()&&pan->getBounds().getWidth()>180,"Pan must be visible on the main panel without Advanced");if(pan){check(pan->getTextFromValue(0)=="C"&&pan->getTextFromValue(-1)=="L 100"&&pan->getTextFromValue(1)=="R 100","Pan must show clear left/center/right values");pan->setValue(.4,juce::sendNotificationSync);check(std::abs(get(restored,"balance")-.4f)<.001f,"main Pan must control the real automatable parameter");check(std::abs(pan->getValueFromText("L 75")+.75)<.001&&pan->getValueFromText("C")==0,"Pan text entry must accept L/R percentages and C");}
    if(pan){
        const auto layout=pan->getLookAndFeel().getSliderLayout(*pan);
        check(layout.textBoxBounds.getY()>=layout.sliderBounds.getBottom(),"Pan C/L/R readout must sit below the slider track");
        check(std::abs(layout.sliderBounds.getCentreX()-layout.textBoxBounds.getCentreX())<=1,"Pan track and readout must share the same horizontal center");
        editor->setSize(1120,840);const auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);int strayLabelPixels=0;
        for(int y=733;y<751;++y)for(int x=789;x<808;++x){const auto c=image.getPixelAt(x,y);strayLabelPixels+=std::max({c.getRed(),c.getGreen(),c.getBlue()})>80;}
        check(strayLabelPixels==0,"Pan must not paint an orphan L to the left of the track");
    }
}
void panQualitySwitch(){
    for(float direction:{-1.f,1.f}){
    Cracked808sAudioProcessor p;p.setParameter("quality",0);p.setParameter("globalMix",0);p.setParameter("width",0);p.setParameter("output",0);p.setParameter("protect",0);p.setParameter("balance",direction);p.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
    auto render=[&](){for(int j=0;j<256;++j){audio.setSample(0,j,.25f);audio.setSample(1,j,.25f);}p.processBlock(audio,midi);return audio.getMagnitude(direction>0?0:1,0,256);};for(int block=0;block<160;++block)render();
    for(int q:{1,2,3,0}){p.setParameter("quality",(float)q);float leaked=0;for(int block=0;block<40;++block)leaked=std::max(leaked,render());check(leaked<1e-5f,"changing quality must not temporarily move a hard-panned signal back toward center");}
    }
}
void layoutsAndStability(){
    for(auto io:{std::pair<int,int>{1,1},{1,2},{2,2}}){Cracked808sAudioProcessor p;auto layout=p.getBusesLayout();layout.inputBuses.getReference(0)=io.first==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();layout.outputBuses.getReference(0)=io.second==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();check(p.setBusesLayout(layout),"mono and stereo layouts must be supported");
        for(double sr:{44100.,48000.,88200.,96000.,192000.}){p.prepareToPlay(sr,32);p.setParameter("bands",3);p.setParameter("b1_feedback",.9f);
            for(int n:{32,127,256,1023,2048,4097}){juce::AudioBuffer<float> b(io.second,n);juce::MidiBuffer midi;for(int c=0;c<io.second;++c)for(int j=0;j<n;++j)b.setSample(c,j,j%307==0?16.f:.2f*std::sin((float)j*.1f));
                p.processBlock(b,midi);bool finite=true;for(int c=0;c<io.second;++c)for(int j=0;j<n;++j)finite&=std::isfinite(b.getSample(c,j))&&std::abs(b.getSample(c,j))<2;check(finite,"irregular/oversized blocks and hot input must remain bounded");}
        }
    }
    std::cout<<"mono/stereo; 5 sample rates; blocks 32..4097 checked\n";
}
void noAllocation(){
    Cracked808sAudioProcessor p;p.prepareToPlay(48000,256);p.setParameter("bands",6);juce::AudioBuffer<float> b(2,256);juce::MidiBuffer m;b.clear();p.processBlock(b,m);
    allocations=0;for(int quality:{0,1,2,3,0}){p.setParameter("quality",(float)quality);for(int n=0;n<20;++n){watchAllocation=true;p.processBlock(b,m);watchAllocation=false;}}check(allocations==0,"processBlock and quality transitions must allocate no C++ heap memory");std::cout<<"callback allocations (including quality switches): "<<allocations<<'\n';
}
double aliasRatio(int quality){
    Cracked808sAudioProcessor p;p.prepareToPlay(48000,256);p.setParameter("quality",(float)quality);p.setParameter("drive",24);p.setParameter("compensate",0);p.setParameter("protect",0);p.setParameter("output",0);p.setParameter("width",0);p.setParameter("b0_model",15);
    juce::AudioBuffer<float> b(2,256);juce::MidiBuffer m;double re=0,im=0,fr=0,fi=0;int total=0;
    for(int block=0;block<160;++block){for(int j=0;j<256;++j){int n=block*256+j;float x=.65f*std::sin(2*cracked::pi*9000.f*(float)n/48000);b.setSample(0,j,x);b.setSample(1,j,x);}p.processBlock(b,m);
        if(block>31)for(int j=0;j<256;++j){int n=block*256+j;float x=b.getSample(0,j);re+=x*std::cos(2*cracked::pi*21000.f*n/48000);im+=x*std::sin(2*cracked::pi*21000.f*n/48000);fr+=x*std::cos(2*cracked::pi*9000.f*n/48000);fi+=x*std::sin(2*cracked::pi*9000.f*n/48000);++total;}}
    return std::sqrt((re*re+im*im)/(fr*fr+fi*fi));
}
void quality(){double eco=aliasRatio(0),good=aliasRatio(2);std::cout<<"21 kHz alias/fundamental: Eco="<<eco<<" Good="<<good<<'\n';check(good<eco*.4,"8x must measurably reduce folded third harmonic");}
void cleanOutputBoost(){
    Cracked808sAudioProcessor p;p.setParameter("quality",0);p.setParameter("mix",0);p.setParameter("width",0);p.setParameter("output",6);p.prepareToPlay(48000,256);
    juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;std::array<double,10> re{},im{};float peak=0;bool linked=true;
    for(int block=0;block<375;++block){for(int j=0;j<256;++j){float x=.8f*std::sin(2*cracked::pi*60*(float)(block*256+j)/48000);b.setSample(0,j,x);b.setSample(1,j,x*.5f);}p.processBlock(b,midi);
        if(block>=187)for(int j=0;j<256;++j){float y=b.getSample(0,j);peak=std::max(peak,std::abs(y));linked&=std::abs(b.getSample(1,j)-y*.5f)<1e-5;double phase=2*3.141592653589793*60*(block*256+j-256)/48000;for(int h=1;h<=9;++h){re[(size_t)h]+=y*std::cos(phase*h);im[(size_t)h]+=y*std::sin(phase*h);}}}
    double h2=0;for(int h=2;h<=9;++h)h2+=re[(size_t)h]*re[(size_t)h]+im[(size_t)h]*im[(size_t)h];double ratio=std::sqrt(h2)/std::hypot(re[1],im[1]);
    std::cout<<"+6 dB protected clean boost: peak "<<peak<<", harmonics/fundamental "<<ratio<<'\n';check(peak<1&&ratio<.015,"Output boost with no saturation must limit smoothly, not hard-clip the bass");
    check(linked,"safety gain must link L/R rather than independently clipping channels");
}
void qualityHistory(){
    Cracked808sAudioProcessor p;p.prepareToPlay(48000,256);p.setParameter("quality",0);p.setParameter("width",0);juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;
    for(int k=0;k<32;++k){for(int n=0;n<256;++n){float x=.4f*std::sin(.03f*(float)(k*256+n));b.setSample(0,n,x);b.setSample(1,n,x);}p.processBlock(b,midi);}
    p.setParameter("quality",3);for(int k=0;k<40;++k){b.clear();p.processBlock(b,midi);}p.setParameter("quality",0);float peak=0;
    for(int k=0;k<32;++k){b.clear();p.processBlock(b,midi);peak=std::max(peak,b.getMagnitude(0,256));}
    check(peak<1e-6f,"switching back to a dormant quality after silence must not replay old audio");std::cout<<"quality switch stale peak: "<<peak<<'\n';
}
void latencyAndPerformance(){
    for(int q=0;q<4;++q){Cracked808sAudioProcessor p;p.prepareToPlay(48000,256);p.setParameter("quality",(float)q);p.setParameter("bypass",1);juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;
        for(int k=0;k<64;++k){b.clear();p.processBlock(b,midi);}float maxValue=0;int peak=0;for(int block=0;block<8;++block){b.clear();if(block==0){b.setSample(0,0,.5f);b.setSample(1,0,.5f);}p.processBlock(b,midi);for(int j=0;j<256;++j)if(std::abs(b.getSample(0,j))>maxValue){maxValue=std::abs(b.getSample(0,j));peak=block*256+j;}}
        check(std::abs(peak-p.getLatencySamples())<=2,"bypass impulse must align with reported latency in all qualities");
        p.setParameter("bypass",0);p.setParameter("bands",6);for(int k=0;k<8;++k){b.clear();p.processBlock(b,midi);}
        auto start=std::chrono::steady_clock::now();for(int k=0;k<100;++k){for(int j=0;j<256;++j){float x=.1f*std::sin(.11f*(float)(k*256+j));b.setSample(0,j,x);b.setSample(1,j,x);}p.processBlock(b,midi);}auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::cout<<"quality "<<q<<"; bypass peak "<<peak<<"; 6-band CPU/audio-time "<<elapsed/(25600./48000.)*100<<" %\n";
    }
}
void analyzer(){
    cracked::AnalysisData data;cracked::SpectrumAnalyzer fft;data.sampleRate.store(48000);for(int i=0;i<30000;++i){float x=.5f*std::sin(2*cracked::pi*1000*i/48000);data.push(x,x);}fft.update(data,.5f);fft.update(data,.5f);
    int peak=0;for(int i=1;i<cracked::SpectrumAnalyzer::bins;++i)if(fft.post[(size_t)i]>fft.post[(size_t)peak])peak=i;float hz=20*std::pow(1000.f,(float)peak/(cracked::SpectrumAnalyzer::bins-1));check(std::abs(hz-1000)<80,"FFT peak must follow actual 1 kHz tone");
    for(int i=0;i<80;++i)fft.update(data,.5f);check(*std::max_element(fft.post.begin(),fft.post.end())<-70,"FFT must settle on silence");std::cout<<"FFT detected frequency: "<<hz<<" Hz\n";
}
void factoryLevels(){
    Cracked808sAudioProcessor p;juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;float highest=0;
    for(auto name:p.getFactoryPresetNames()){p.loadFactoryPreset(name);p.prepareToPlay(48000,256);double energy=0;float peak=0;
        for(int block=0;block<80;++block){for(int j=0;j<256;++j){float time=(float)(block*256+j)/48000;float x=.35f*std::sin(2*cracked::pi*55*time)+.1f*std::sin(2*cracked::pi*220*time);b.setSample(0,j,x);b.setSample(1,j,x);}p.processBlock(b,midi);for(int c=0;c<2;++c)for(int j=0;j<256;++j){float x=b.getSample(c,j);check(std::isfinite(x),"factory preset output must be finite");peak=std::max(peak,std::abs(x));energy+=x*x;}}
        check(peak<=1.05f&&energy>1,"factory presets must produce bounded non-silent output");highest=std::max(highest,peak);
    }
    std::cout<<"factory states checked: "<<p.getFactoryPresetNames().size()<<"; highest test peak "<<highest<<'\n';
}
void knobGlow(const juce::File& folder){
    if(folder!=juce::File())folder.createDirectory();
    check(cracked::ui::brandLogo().isValid()&&cracked::ui::brandLogo().getWidth()>cracked::ui::brandLogo().getHeight(),"embedded supplied logo must load and fit its alpha bounds");
    check(cracked::ui::font(12).getTypefaceName().containsIgnoreCase("Space Grotesk"),"embedded Space Grotesk must be used, not a system fallback");
    cracked::ui::SpaceTheme theme;cracked::ui::ValueKnob knob;knob.getProperties().set("activity",0.f);knob.getProperties().set("glow",1.f);
    double brightness[3]{};juce::Image strip(juce::Image::ARGB,516,196,true);
    for(int n=0;n<3;++n){juce::Image image(juce::Image::ARGB,172,196,true);{juce::Graphics g(image);g.fillAll(juce::Colour(0xff090c0f));
        theme.drawRotarySlider(g,0,0,172,168,n*.5f,cracked::pi*1.25f,cracked::pi*2.75f,knob);}
        int count=0;for(int y=0;y<168;++y)for(int x=0;x<172;++x){float r=std::hypot(x-86.f,y-84.f);if(r>76&&r<82){brightness[n]+=image.getPixelAt(x,y).getGreen();++count;}}
        brightness[n]/=count;juce::Graphics composite(strip);composite.drawImageAt(image,n*172,0);
    }
    std::cout<<"silent knob halo mean (0/50/100%): "<<brightness[0]<<" / "<<brightness[1]<<" / "<<brightness[2]<<'\n';
    check(brightness[1]>brightness[0]+1&&brightness[2]>brightness[1]+1&&brightness[2]>brightness[0]+5,"deep knob halo must grow visibly with value even with zero audio activity");
    if(folder!=juce::File()){juce::FileOutputStream out(folder.getChildFile("Knob-glow-0-50-100.png"));out.setPosition(0);out.truncate();juce::PNGImageFormat().writeImageToStream(strip,out);}
}
void monochromeTheme(){
    Cracked808sAudioProcessor p;std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->setSize(1120,840);
    const auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);
    double background=0,tint=0;int whites=0;
    for(int y=100;y<110;++y)for(int x=20;x<30;++x){const auto c=image.getPixelAt(x,y);background+=std::max({c.getRed(),c.getGreen(),c.getBlue()});tint+=std::max({c.getRed(),c.getGreen(),c.getBlue()})-std::min({c.getRed(),c.getGreen(),c.getBlue()});}
    for(int y=30;y<61;++y)for(int x=128;x<438;++x){const auto c=image.getPixelAt(x,y);if(std::min({c.getRed(),c.getGreen(),c.getBlue()})>=245)++whites;}
    std::cout<<"UI background / tint / white title pixels: "<<background/100<<" / "<<tint/100<<" / "<<whites<<'\n';
    check(background/100<=4&&tint/100<1,"main surface must render deep neutral black rather than blue-gray");
    check(whites>=30,"brand title must render crisp near-white highlights against the black shell");
    auto* view=dynamic_cast<Cracked808sAudioProcessorEditor*>(editor.get());view->showVisualizerPopout(true);
    const auto visual=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);const auto corner=visual.getPixelAt(5,5);
    check(std::max({corner.getRed(),corner.getGreen(),corner.getBlue()})<=2,"visualizer-only mode must share the black background");
}
void visualState(){
    Cracked808sAudioProcessor p;check(p.parameters.getParameter("visualMode")!=nullptr,"Spectrum/Sphere/Image mode must be selectable");
    juce::Image image(juce::Image::ARGB,64,64,true);{juce::Graphics g(image);g.setColour(juce::Colours::white);g.fillEllipse(6,6,52,52);}
    juce::MemoryOutputStream encoded;juce::PNGImageFormat().writeImageToStream(image,encoded);const auto png=encoded.getMemoryBlock();
    check(p.setVisualPng(png),"valid custom PNG must import");const auto original=p.getVisualPng();
    juce::MemoryBlock bad("not a PNG",9);check(!p.setVisualPng(bad)&&p.getVisualPng()==original,"bad PNG must be rejected without clearing valid artwork");
    juce::MemoryBlock huge(8*1024*1024+1,true);check(!p.setVisualPng(huge),"oversized PNG import must be rejected");
    if(p.parameters.getParameter("visualMode")){p.setParameter("visualMode",2);p.setParameter("visualAmount",.83f);juce::MemoryBlock state;p.getStateInformation(state);Cracked808sAudioProcessor restored;restored.setStateInformation(state.getData(),(int)state.getSize());check(restored.getVisualPng()==original&&original.isNotEmpty(),"custom image must travel inside host state, not rely on original file");check(get(restored,"visualMode")==2&&std::abs(get(restored,"visualAmount")-.83f)<.01,"visual mode and reaction amount must restore");p.loadFactoryPreset("Warm Tube");check(get(p,"visualMode")==2&&p.getVisualPng()==original,"factory sound changes must preserve the chosen visualization");}
    {juce::MemoryBlock emptyState;p.clearVisualPng();p.getStateInformation(emptyState);p.setVisualPng(png);p.setStateInformation(emptyState.getData(),(int)emptyState.getSize());check(p.getVisualPng().isEmpty(),"restoring a state without custom art must clear the previous project's image");}
    p.setParameter("reducedMotion",1);p.setParameter("glow",.31f);p.setParameter("showPre",0);p.setParameter("smoothing",.22f);p.setParameter("fps",1);p.loadFactoryPreset("Clean Weight");
    check(get(p,"reducedMotion")==1&&std::abs(get(p,"glow")-.31f)<.001&&get(p,"showPre")==0&&std::abs(get(p,"smoothing")-.22f)<.001&&get(p,"fps")==1,"factory sound selection must preserve reduced-motion and all display preferences");
    juce::Image mark(juce::Image::ARGB,120,80,true);{juce::Graphics g(mark);cracked::ui::logo(g,{10,8,100,64});}int soft=0;for(int y=0;y<80;++y)for(int x=0;x<120;++x){auto a=mark.getPixelAt(x,y).getAlpha();if(a>8&&a<247)++soft;}
    std::cout<<"small logo fractional-alpha pixels: "<<soft<<'\n';check(soft>180,"small logo must have properly filtered edges rather than binary grain");
    cracked::ui::SpectrumDisplay display(p);display.setSize(864,296);std::array<juce::int64,3> sums{};
    for(int mode=0;mode<3;++mode){p.setParameter("visualMode",(float)mode);display.update();auto rendered=display.createComponentSnapshot(display.getLocalBounds());for(int y=0;y<296;++y)for(int x=0;x<864;++x)sums[(size_t)mode]+=rendered.getPixelAt(x,y).getAlpha();}
    check(sums[0]>0&&sums[1]>0&&sums[2]>0&&sums[0]!=sums[1]&&sums[1]!=sums[2],"all three visual modes must draw different real content");
}
void audioExamples(const juce::File& folder){
    auto audioFolder=folder.getChildFile("Audio Examples");audioFolder.createDirectory();
    const char* names[]={"01 - Dry mono.wav","02 - Warm saturation.wav","03 - Warm and wide.wav","04 - Clean boost plus 6dB.wav","05 - Broad width 200 percent.wav","06 - Center test dry.wav","07 - Center test wide 150 percent.wav"};
    for(int variant=0;variant<7;++variant){
        Cracked808sAudioProcessor p;p.setParameter("drive",18);p.setParameter("b0_model",1);p.setParameter("width",variant==2||variant==6?1.5f:variant==4?2.f:0.f);p.setParameter("mix",variant==3||variant>=5?0.f:1.f);p.setParameter("monoBass",110);p.setParameter("quality",2);p.setParameter("bypass",variant==0||variant==5?1.f:0.f);p.setParameter("output",variant==3?6.f:0.f);if(variant>=5){p.setParameter("lowEnabled",0);p.setParameter("monoSafe",0);}
        p.prepareToPlay(48000,256);juce::AudioBuffer<float> block(2,256),rendered(2,48000*6);juce::MidiBuffer midi;
        for(int n=0;n<64;++n){block.clear();p.processBlock(block,midi);}
        const int latency=p.getLatencySamples();int absolute=0;float peak=0;double side=0;
        for(int offset=0;offset<rendered.getNumSamples()+latency;offset+=256){
            for(int j=0;j<256;++j){double t=(double)(offset+j)/48000,beat=std::fmod(t,.75);constexpr float notes[]={55,82,110,146};double fundamental=variant>=5?notes[((int)(t/.75))%4]:46;double phase=2*3.141592653589793*(fundamental*beat+8*.035*(1-std::exp(-beat/.035)));
                float x=(float)(.32*(1-std::exp(-beat/.002))*std::exp(-beat*4.5)*(std::sin(phase)+.22*std::sin(phase*2)+.1*std::sin(phase*3)+.035*std::sin(phase*7)));
                block.setSample(0,j,x);block.setSample(1,j,x);
            }p.processBlock(block,midi);
            for(int j=0;j<256;++j,++absolute){int at=absolute-latency;if(at>=0&&at<rendered.getNumSamples())for(int c=0;c<2;++c){float x=block.getSample(c,j);rendered.setSample(c,at,x);peak=std::max(peak,std::abs(x));}if(at>=0)side+=std::pow(block.getSample(0,j)-block.getSample(1,j),2);}
        }
        check(peak<1,"audio examples must have unclipped headroom");check(variant!=2||side>1,"wide example must contain actual stereo side audio");
        auto stream=audioFolder.getChildFile(names[variant]).createOutputStream();stream->setPosition(0);stream->truncate();
        juce::WavAudioFormat wav;std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),48000,2,24,{},0));check(writer&&writer->writeFromAudioSampleBuffer(rendered,0,rendered.getNumSamples()),"audio example must export a real stereo WAV");
    }
}
void visualCustomization(){
    Cracked808sAudioProcessor p;const char* ids[]={"visualSensitivity","visualAttack","visualRelease","visualMotion","visualMotionDepth","visualSpeed","visualGlow","visualAudioGlow","visualDensity","visualPointSize","visualScale","visualColor","visualPalette","visualColorSpeed"};bool present=true;
    for(auto* id:ids)present&=p.parameters.getParameter(id)!=nullptr;check(present,"Advanced must expose complete reactive visual customization");if(!present)return;
    p.setParameter("visualMode",1);p.setParameter("visualColor",1);p.setParameter("visualDensity",2);p.setParameter("visualGlow",1.4f);p.setParameter("visualAttack",48);p.setParameter("visualRelease",410);
    juce::MemoryBlock state;p.getStateInformation(state);Cracked808sAudioProcessor restored;restored.setStateInformation(state.getData(),(int)state.getSize());restored.loadFactoryPreset("Warm Tube");
    check(get(restored,"visualColor")==1&&get(restored,"visualDensity")==2&&std::abs(get(restored,"visualGlow")-1.4f)<.01&&get(restored,"visualAttack")==48&&get(restored,"visualRelease")==410,"visual settings must restore and survive factory sound changes");
    cracked::ui::SpectrumDisplay display(p);display.setSize(864,296);p.setParameter("visualColor",0);p.setParameter("visualMotion",0);p.setParameter("visualAudioGlow",0);display.update();
    auto silent=display.createComponentSnapshot(display.getLocalBounds());p.setParameter("visualColor",1);display.update();auto colored=display.createComponentSnapshot(display.getLocalBounds());int colorPixels=0,grayColors=0;
    // Test visible chroma over black, not unpremultiplied rounding at tiny alpha.
    for(int y=0;y<296;++y)for(int x=0;x<864;++x){auto c=colored.getPixelAt(x,y),s=silent.getPixelAt(x,y);if((std::max({c.getRed(),c.getGreen(),c.getBlue()})-std::min({c.getRed(),c.getGreen(),c.getBlue()}))*c.getAlpha()>12*255)++colorPixels;if((std::max({s.getRed(),s.getGreen(),s.getBlue()})-std::min({s.getRed(),s.getGreen(),s.getBlue()}))*s.getAlpha()>12*255)++grayColors;}
    check(colorPixels>500&&grayColors==0,"optional color mode must visibly color particles, with a monochrome default");
    auto coverage=[&](int density){p.setParameter("visualDensity",(float)density);display.update();auto shot=display.createComponentSnapshot(display.getLocalBounds());int count=0;for(int y=40;y<260;++y)for(int x=280;x<610;++x)if(shot.getPixelAt(x,y).getAlpha()>40)++count;return count;};
    p.setParameter("visualColor",0);int sparse=coverage(0),dense=coverage(2);std::cout<<"particle coverage airy/dense: "<<sparse<<"/"<<dense<<"; colored pixels "<<colorPixels<<'\n';check(dense>sparse*1.15,"particle density must alter actual rendered particle coverage");
    display.update();auto stillA=display.createComponentSnapshot(display.getLocalBounds());display.update();auto stillB=display.createComponentSnapshot(display.getLocalBounds());bool still=true;for(int y=0;y<296;++y)for(int x=0;x<864;++x)still&=stillA.getPixelAt(x,y)==stillB.getPixelAt(x,y);check(still,"motion and audio glow disabled must produce a stable particle image");
    p.setParameter("visualMotion",1);p.setParameter("reducedMotion",1);display.update();stillA=display.createComponentSnapshot(display.getLocalBounds());display.update();stillB=display.createComponentSnapshot(display.getLocalBounds());still=true;for(int y=0;y<296;++y)for(int x=0;x<864;++x)still&=stillA.getPixelAt(x,y)==stillB.getPixelAt(x,y);check(still,"Reduced Motion must freeze particle geometry");
    p.setParameter("visualDensity",1);display.update();auto started=std::chrono::steady_clock::now();for(int frame=0;frame<8;++frame)display.createComponentSnapshot(display.getLocalBounds());std::cout<<"default particle snapshot mean: "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()/8<<" ms\n";
    cracked::ui::ParticleField field;cracked::ui::ParticleStyle style;style.density=0;style.level=1;style.audioGlow=false;style.glow=0;
    auto frameAt=[&](float measured){style.hit=measured;for(size_t i=0;i<style.profile.size();++i)style.profile[i]=measured;for(size_t i=0;i<style.waveform.size();++i)style.waveform[i]=measured*std::sin((float)i*.17f);juce::Image shot(juce::Image::ARGB,864,296,true);{juce::Graphics g(shot);field.paint(g,{26,16,812,248},style);}return shot;};
    auto first=frameAt(.7f),repeated=frameAt(.7f);double difference=0,energy=1;for(int y=0;y<296;++y)for(int x=0;x<864;++x){int a=first.getPixelAt(x,y).getAlpha(),b=repeated.getPixelAt(x,y).getAlpha();difference+=std::abs(a-b);energy+=a;}
    check(difference==0,"identical audio measurements must produce identical particles without a procedural clock");
    style.amount=style.motionDepth=1;style.scale=1.15f;style.size=1.8f;int outside=0;
    for(int step=0;step<8;++step){auto shot=frameAt((float)step/7);for(int y=0;y<296;++y)if(y<16||y>=264)for(int x=0;x<864;++x)if(shot.getPixelAt(x,y).getAlpha()>80)++outside;}
    check(outside==0,"maximum particle settings must keep the visible body inside its display field");
    p.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;int sample=0;
    auto activeFrame=[&](){for(int block=0;block<6;++block){for(int j=0;j<256;++j,++sample){float x=.65f*std::sin(2*cracked::pi*60*sample/48000);audio.setSample(0,j,x);audio.setSample(1,j,x);}p.processBlock(audio,midi);}display.update();return display.createComponentSnapshot(display.getLocalBounds());};
    auto changed=[](const juce::Image& a,const juce::Image& b){int pixels=0;for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x)pixels+=a.getPixelAt(x,y)!=b.getPixelAt(x,y);return pixels;};
    p.setParameter("visualMotion",1);p.setParameter("reducedMotion",0);p.setParameter("visualColor",0);p.setParameter("visualAudioGlow",0);for(int frame=0;frame<12;++frame)activeFrame();auto movingA=activeFrame(),movingB=activeFrame();check(changed(movingA,movingB)>100,"actual audio must move enabled particle geometry");
    p.setParameter("visualMotion",0);auto frozenA=activeFrame(),frozenB=activeFrame();check(changed(frozenA,frozenB)==0,"Motion off must stay frozen while audio continues");
    p.setParameter("visualMotion",1);p.setParameter("reducedMotion",1);frozenA=activeFrame();frozenB=activeFrame();check(changed(frozenA,frozenB)==0,"Reduced Motion must suppress geometric movement during active audio");
    p.setParameter("visualAudioGlow",1);auto glowing=activeFrame();p.setParameter("visualAudioGlow",0);auto unlit=activeFrame();check(changed(glowing,unlit)>500,"Audio Glow must affect brightness under real incoming audio");
    p.setParameter("visualDensity",2);p.setParameter("visualColor",1);p.setParameter("visualAudioGlow",1);p.setParameter("reducedMotion",0);started=std::chrono::steady_clock::now();for(int frame=0;frame<8;++frame)activeFrame();std::cout<<"dense colored active render + audio mean: "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()/8<<" ms\n";
}
void musicDrivenVisuals(const juce::File& folder){
    // Regressions for the old max-bin clamp + free-running particle clock.
    auto renderTone=[](float amplitude,float hz,int frames){
        auto owner=std::make_unique<Cracked808sAudioProcessor>();auto& p=*owner;p.setParameter("visualMode",1);p.setParameter("visualAudioGlow",0);p.setParameter("visualColor",0);
        auto viewOwner=std::make_unique<cracked::ui::SpectrumDisplay>(p);auto& display=*viewOwner;display.setSize(864,296);int sample=0;
        for(int frame=0;frame<frames;++frame){for(int j=0;j<1600;++j,++sample){const float x=amplitude*std::sin(2*cracked::pi*hz*sample/48000);p.analysis.push(x,x);}display.update();}
        return display.createComponentSnapshot(display.getLocalBounds());
    };
    auto difference=[](const juce::Image& a,const juce::Image& b){double error=0,energy=1;for(int y=20;y<260;++y)for(int x=270;x<594;++x){const int av=a.getPixelAt(x,y).getAlpha(),bv=b.getPixelAt(x,y).getAlpha();error+=std::abs(av-bv);energy+=av;}return error/energy;};
    auto spread=[](const juce::Image& image){double radius=0,energy=1;for(int y=24;y<256;++y)for(int x=290;x<574;++x){const double alpha=image.getPixelAt(x,y).getAlpha();radius+=alpha*((x-432.)*(x-432.)+(y-140.)*(y-140.));energy+=alpha;}return std::sqrt(radius/energy);};
    const auto quiet=renderTone(.45f,60,100),loud=renderTone(.85f,60,100),bright=renderTone(.85f,1500,100);
    const double quietRadius=spread(quiet),loudRadius=spread(loud),timbreDifference=difference(loud,bright);
    std::cout<<"music visuals quiet/loud spread: "<<quietRadius<<"/"<<loudRadius<<"; bass/bright difference "<<timbreDifference<<'\n';
    check(loudRadius>quietRadius*1.01,"loud and moderately loud bass must remain visually distinct, not pin the reaction at 1");
    check(timbreDifference>.12,"equal-level different frequencies must directly alter particle shape, not just animation speed");
    const auto sustainedA=renderTone(.85f,60,100),sustainedB=renderTone(.85f,60,130);
    const double drift=difference(sustainedA,sustainedB);std::cout<<"steady-tone particle drift: "<<drift<<'\n';
    check(drift<.025,"a settled sustained tone must not keep running a canned particle orbit");
    // A 192 kHz session still needs enough time history to trigger deep bass.
    {auto sourceOwner=std::make_unique<cracked::AnalysisData>();auto analyzerOwner=std::make_unique<cracked::SpectrumAnalyzer>();auto& source=*sourceOwner;auto& analyzer=*analyzerOwner;source.sampleRate.store(192000);std::array<float,96> previous{};double error=0;int comparisons=0;
        for(int f=0;f<120;++f){for(int j=0;j<3200;++j){const float x=.7f*std::sin(2*cracked::pi*37*(f*3200+j)/192000);source.push(x,x);}analyzer.update(source,.6f);
            if(f>60){for(size_t j=0;j<previous.size();++j)error+=std::abs(previous[j]-analyzer.waveform[j]);comparisons+=(int)previous.size();}previous=analyzer.waveform;}
        std::cout<<"192 kHz sustained-bass waveform drift: "<<error/comparisons<<'\n';check(error/comparisons<.002,"high sample-rate bass must retain a stable triggered waveform");}
    auto burstAtFps=[](int fps){auto processorOwner=std::make_unique<Cracked808sAudioProcessor>();auto& processor=*processorOwner;processor.setParameter("visualMode",1);auto viewOwner=std::make_unique<cracked::ui::SpectrumDisplay>(processor);auto& view=*viewOwner;view.setSize(864,296);
        for(int n=0;n<4800;){const int end=std::min(4800,n+48000/fps);for(;n<end;++n){const float x=n>=1008&&n<1392?.85f*std::sin(2*cracked::pi*1000*n/48000):0;processor.analysis.push(x,x);}view.update();}return view.createComponentSnapshot(view.getLocalBounds());};
    const double fpsDifference=difference(burstAtFps(30),burstAtFps(60));std::cout<<"same short hit, 30/60 FPS image difference: "<<fpsDifference<<'\n';
    check(fpsDifference<.025,"the same short hit must have the same reaction timing at 30 and 60 FPS");
    auto owner=std::make_unique<Cracked808sAudioProcessor>();auto& p=*owner;p.setParameter("visualMode",1);p.setParameter("visualColor",1);auto viewOwner=std::make_unique<cracked::ui::SpectrumDisplay>(p);auto& display=*viewOwner;display.setSize(864,296);
    int sample=0;auto frame=[&](float amplitude){for(int j=0;j<1600;++j,++sample){const float x=amplitude*(std::sin(2*cracked::pi*60*sample/48000)+.18f*std::sin(2*cracked::pi*240*sample/48000));p.analysis.push(x,x);}display.update();return display.createComponentSnapshot(display.getLocalBounds());};
    auto idle=frame(0);for(int i=0;i<45;++i)idle=frame(0);const double idleRadius=spread(idle);
    auto attack=frame(.65f);for(int i=0;i<2;++i)attack=frame(.65f);const double attackRadius=spread(attack);
    auto decay=frame(.18f);for(int i=0;i<8;++i)decay=frame(.18f);auto silent=frame(0);for(int i=0;i<100;++i)silent=frame(0);
    std::cout<<"hit spread idle/100ms: "<<idleRadius<<"/"<<attackRadius<<"; settled silence difference "<<difference(idle,silent)<<'\n';
    check(attackRadius>idleRadius*1.08,"a real bass hit must visibly expand particles within 100 ms");
    check(difference(attack,decay)>.15,"decaying notes must change particles rather than hold their attack pose");
    check(difference(idle,silent)<.015,"particles and optional color must return to rest after audio silence");
    auto visibleDifference=[](const juce::Image& a,const juce::Image& b){double error=0,energy=1;for(int y=20;y<260;++y)for(int x=270;x<594;++x){const auto av=a.getPixelAt(x,y),bv=b.getPixelAt(x,y);const float aa=av.getFloatAlpha(),ba=bv.getFloatAlpha();error+=std::abs(av.getRed()*aa-bv.getRed()*ba)+std::abs(av.getGreen()*aa-bv.getGreen()*ba)+std::abs(av.getBlue()*aa-bv.getBlue()*ba);energy+=(av.getRed()+av.getGreen()+av.getBlue())*aa;}return error/energy;};
    check(visibleDifference(idle,silent)<.015,"optional palette colors and glow must return to their rest colors in silence");
    for(int i=0;i<6;++i)frame(.65f);for(int i=0;i<420;++i)display.update();const auto stopped=display.createComponentSnapshot(display.getLocalBounds());
    check(visibleDifference(idle,stopped)<.015,"a host that stops sending samples must also release the visualizer to rest");
    // Custom PNG uses the same measured reaction, not a separate motion clock.
    p.setParameter("visualMode",2);for(int i=0;i<60;++i)frame(0);const auto imageIdle=frame(0);for(int i=0;i<3;++i)frame(.65f);const auto imageHit=frame(.65f);
    check(difference(imageIdle,imageHit)>.12,"Image mode must visibly respond to the actual hit");
    if(folder!=juce::File()){
        folder.createDirectory();juce::Image strip(juce::Image::RGB,864,296*4,true);{juce::Graphics g(strip);g.fillAll(juce::Colour(0xff060809));
        const juce::Image shots[]={idle,attack,decay,silent};const char* labels[]={"SILENCE","BASS HIT / 100 ms","DECAYING NOTE","RETURN TO REST"};
        for(int i=0;i<4;++i){g.drawImageAt(shots[i],0,i*296);g.setColour(cracked::ui::silver());g.setFont(cracked::ui::font(10));g.drawText(labels[i],20,i*296+15,250,20,juce::Justification::centredLeft);}}
        int visible=0;for(int y=0;y<strip.getHeight();++y)for(int x=0;x<strip.getWidth();++x)visible+=strip.getPixelAt(x,y).getGreen()>30;
        check(visible>4000,"the exported reaction sequence must contain visible rendered particles");
        juce::FileOutputStream out(folder.getChildFile("Music-reaction-sequence.png"));out.setPosition(0);out.truncate();juce::PNGImageFormat().writeImageToStream(strip,out);
    }
}
void colorBehaviors(){
    auto owner=std::make_unique<Cracked808sAudioProcessor>();auto& p=*owner;
    const char* ids[]={"visualColorMode","visualBassSensitivity","visualHitGap","visualColorTransition","visualHue","visualSaturation","visualColorSpread"};bool complete=true;for(auto* id:ids)complete&=p.parameters.getParameter(id)!=nullptr;
    check(complete,"color behavior, bass-hit and hue customization must be available");if(!complete)return;
    p.setParameter("visualColor",1);p.setParameter("visualColorMode",1);p.setParameter("visualHitGap",230);p.setParameter("visualHue",.72f);p.setParameter("visualSaturation",.8f);p.setParameter("visualColorSpread",.1f);
    juce::MemoryBlock state;p.getStateInformation(state);auto restored=std::make_unique<Cracked808sAudioProcessor>();restored->setStateInformation(state.getData(),(int)state.getSize());restored->loadFactoryPreset("Warm Tube");
    check(get(*restored,"visualColorMode")==1&&get(*restored,"visualHitGap")==230&&std::abs(get(*restored,"visualHue")-.72f)<.002f,"color behavior/customization must persist and survive factory sound changes");
    struct Result{int hits=0;float hue=0;};
    auto run=[](float hz,bool bursts,int fps,int rate,bool freeze=false){
        auto data=std::make_unique<cracked::AnalysisData>();auto analyzer=std::make_unique<cracked::SpectrumAnalyzer>();data->sampleRate.store((float)rate);cracked::ui::ColorBehavior color;cracked::ui::ColorSettings settings;settings.enabled=true;settings.mode=1;settings.frozen=freeze;Result result;
        for(int n=0;n<rate*2;){const int end=std::min(rate*2,n+rate/fps);for(;n<end;++n){const float beat=(float)(n%(rate/2))/rate,amplitude=bursts?(beat<.18f?.65f*std::exp(-beat*10):0):.65f;const float x=amplitude*std::sin(2*cracked::pi*hz*n/rate);data->push(x,x);}
            analyzer->update(*data,.6f,[&](float dt){result.hits+=color.advance(dt,analyzer->bassFast,analyzer->bassSlow,analyzer->fastRms,settings,0);});}
        result.hue=color.hue();return result;
    };
    const auto bass=run(60,true,30,48000),fast=run(60,true,60,48000),treble=run(1000,true,60,48000),steady=run(60,false,60,48000),frozen=run(60,true,60,48000,true);
    std::cout<<"random color triggers bass/treble/steady/frozen: "<<bass.hits<<'/'<<treble.hits<<'/'<<steady.hits<<'/'<<frozen.hits<<"; hue30/60 "<<bass.hue<<'/'<<fast.hue<<'\n';
    check(bass.hits==4,"separated bass hits must each select a new random full-spectrum hue");check(treble.hits==0&&steady.hits==1,"treble and sustained bass must not repeatedly change random bass-hit colors");
    check(fast.hits==bass.hits&&std::abs(fast.hue-bass.hue)<.0001f,"random bass colors must be frame-rate independent");check(frozen.hits==0&&frozen.hue==0,"Reduced Motion must freeze random hue changes");
    check(run(37,true,60,192000).hits==4&&run(60,true,30,44100).hits==4,"bass color triggers must remain reliable at low/high sample rates");
    cracked::ui::ColorBehavior fixed;cracked::ui::ColorSettings settings;settings.enabled=true;settings.mode=3;settings.hue=.82f;for(int i=0;i<200;++i)fixed.advance(.01f,.5f,.2f,.5f,settings,.1f);check(std::abs(fixed.hue()-.82f)<.001f,"Fixed Hue must ignore musical color changes");
}
void editorOwnedWindows(){
    auto* manager=juce::ModalComponentManager::getInstance();const int before=manager->getNumModalComponents();
    Cracked808sAudioProcessor processor;processor.prepareToPlay(48000,256);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    juce::Button* presetButton=nullptr;
    for(auto* parent:editor->getChildren())for(auto* child:parent->getChildren())if(auto* button=dynamic_cast<juce::Button*>(child))if(button->getButtonText()=="INIT")presetButton=button;
    check(presetButton!=nullptr,"lifecycle fixture must find the actual preset browser button");
    if(presetButton){presetButton->triggerClick();juce::MessageManager::getInstance()->runDispatchLoopUntil(20);}
    check(manager->getNumModalComponents()>before,"preset browser must actually be open before teardown");
    editor.reset();
    check(manager->getNumModalComponents()==before,"editor destruction must not leave a modal browser holding a processor reference");
    juce::MessageManager::getInstance()->runDispatchLoopUntil(250);
}
void visualizerPopout(){
    Cracked808sAudioProcessor processor;processor.prepareToPlay(48000,256);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    cracked::ui::SpectrumDisplay* original=nullptr;juce::Button* pop=nullptr;juce::Slider* reaction=nullptr;
    for(auto* parent:editor->getChildren())for(auto* child:parent->getChildren()){
        if(auto* display=dynamic_cast<cracked::ui::SpectrumDisplay*>(child))original=display;
        if(auto* button=dynamic_cast<juce::Button*>(child))if(button->getButtonText()=="POP OUT"||button->getButtonText()=="VISUAL ONLY")pop=button;
        if(auto* slider=dynamic_cast<juce::Slider*>(child))if(slider->getName()=="Visual reaction strength")reaction=slider;
    }
    check(original&&pop,"main editor must expose a visualizer-only pop-out action");if(!original||!pop)return;
    check(reaction!=nullptr,"reaction strength needs an accessible name");
    check(pop->getY()<100,"pop-out action must leave room for the visible reaction label beside its slider");
    const auto desktopBefore=juce::Desktop::getInstance().getNumComponents();const juce::Point<int> fullSize{editor->getWidth(),editor->getHeight()};
    pop->triggerClick();juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
    check(original->getTopLevelComponent()==editor.get(),"visual-only mode must use the same host editor, never a second window");
    check(juce::Desktop::getInstance().getNumComponents()==desktopBefore,"visual-only mode must not create a desktop window");
    check(editor->getHeight()<fullSize.y,"entering visual-only mode must shrink the actual plugin panel");
    check(editor->getConstrainer()->getMaximumWidth()>=3840&&editor->getConstrainer()->getFixedAspectRatio()==0,"visual-only mode must allow larger freely-shaped resizing");
    editor->setSize(2600,1400);check(original->getWidth()>=2500&&original->getHeight()>=1300,"the visualizer must expand with the host editor");
    auto* view=dynamic_cast<Cracked808sAudioProcessorEditor*>(editor.get());view->showVisualizerPopout(false);
    check(editor->getWidth()==fullSize.x&&editor->getHeight()==fullSize.y,"returning to controls must restore the original panel size");
    pop->triggerClick();juce::MessageManager::getInstance()->runDispatchLoopUntil(20);editor.reset();
    check(juce::Desktop::getInstance().getNumComponents()==desktopBefore,"editor teardown must synchronously close its owned visualizer window");
}
void advancedSidebar(){
    const auto compactSize=cracked::ui::fitFullEditorSize(1568,1093,620,.75f);
    check(compactSize.width<=1061&&compactSize.height<=560,"Advanced minimum size must fit a small high-DPI logical desktop");
    Cracked808sAudioProcessor processor;std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    auto* view=dynamic_cast<Cracked808sAudioProcessorEditor*>(editor.get());const int oldHeight=editor->getHeight();view->showAdvanced(true);
    cracked::ui::AdvancedPanel* panel=nullptr;for(auto* parent:editor->getChildren())for(auto* child:parent->getChildren())if(auto* found=dynamic_cast<cracked::ui::AdvancedPanel*>(child))panel=found;
    check(panel&&panel->getX()>=1120&&panel->getY()<80,"Advanced must occupy a right sidebar instead of the bottom");
    check(editor->getHeight()<=oldHeight,"opening Advanced must not grow the editor downward");if(!panel)return;
    juce::Viewport* viewport=nullptr;for(auto* child:panel->getChildren())if(auto* found=dynamic_cast<juce::Viewport*>(child))viewport=found;
    check(viewport!=nullptr,"sidebar controls must have a scrollable body, not clipped rows");if(!viewport)return;
    for(int page=0;page<7;++page){panel->page=page;panel->rebuild();auto* body=viewport->getViewedComponent();check(body!=nullptr,"sidebar must retain its owned control body");if(!body)continue;
        int count=0;for(auto* child:body->getChildren())if(dynamic_cast<cracked::ui::ParameterControl*>(child)){++count;check(body->getLocalBounds().contains(child->getBounds()),"every sidebar control must fit inside its scrollable content");}
        check(count>=9,"each sidebar page must preserve all its parameter controls");
    }
    view->showAdvanced(false);
}
void visualIdleScheduling(){
    Cracked808sAudioProcessor processor;cracked::ui::SpectrumDisplay display(processor);display.setSize(864,296);
    check(display.update(),"a newly opened visualizer must render its initial frame");
    check(!display.update(),"an unchanged settled visualizer must not request another repaint");
    processor.setParameter("visualGlow",2.f);check(display.update(),"visual settings must invalidate an idle frame");
    check(!display.update(),"unchanged settings must return to idle after the new frame");
    for(int n=0;n<1600;++n){float x=.6f*std::sin(2*cracked::pi*60*n/48000);processor.analysis.push(x,x);}
    check(display.update(),"real audio must wake the visualizer immediately");
    bool silentIdle=false;
    for(int frame=0;frame<300;++frame){for(int n=0;n<1600;++n)processor.analysis.push(0,0);if(!display.update())silentIdle=true;}
    check(silentIdle,"a host that keeps sending digital silence must also stop redrawing after settling");
    processor.setParameter("visualColor",1);processor.setParameter("visualColorMode",3);processor.setParameter("visualColorTransition",800);processor.setParameter("visualHue",.1f);display.update();display.update();
    processor.setParameter("visualHue",.7f);check(display.update(),"changing a stopped visualizer hue must start a new frame");
    check(display.update(),"a stopped visualizer must continue repainting its nonzero hue transition");
}
void hiddenNativeEditor(){
#if JUCE_WINDOWS
    auto parent=CreateWindowExW(0,L"STATIC",L"Disposable Cracked808s visibility fixture",WS_OVERLAPPEDWINDOW,-10000,-10000,1200,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    check(parent!=nullptr,"native visibility test needs a real host window");if(!parent)return;
    Cracked808sAudioProcessor processor;processor.prepareToPlay(48000,256);std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    ShowWindow(parent,SW_SHOWNOACTIVATE);editor->addToDesktop(juce::ComponentPeer::windowIsTemporary,parent);editor->setVisible(true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(80);check(processor.analysis.isObserved(),"a shown embedded editor must collect visual analysis");
    ShowWindow(parent,SW_HIDE);juce::MessageManager::getInstance()->runDispatchLoopUntil(80);
    check(!processor.analysis.isObserved(),"hiding the host HWND must suspend visual work even if JUCE ancestors still report visible");
    auto* view=dynamic_cast<Cracked808sAudioProcessorEditor*>(editor.get());view->showVisualizerPopout(true);juce::MessageManager::getInstance()->runDispatchLoopUntil(80);
    check(!processor.analysis.isObserved(),"visual-only mode in a hidden host must remain suspended, without a detached window");
    view->showVisualizerPopout(false);juce::MessageManager::getInstance()->runDispatchLoopUntil(80);
    check(!processor.analysis.isObserved(),"docking into a hidden host must suspend visual work again");editor.reset();DestroyWindow(parent);
#endif
}
void analysisOnlyWhenObserved(){
    Cracked808sAudioProcessor processor;processor.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
    auto process=[&]{for(int c=0;c<2;++c)for(int n=0;n<256;++n)audio.setSample(c,n,.4f);processor.processBlock(audio,midi);};
    cracked::AnalysisSample sample;process();check(!processor.analysis.pop(sample),"closed UI must not collect unused per-sample visual analysis");
    {cracked::ui::SpectrumDisplay display(processor);process();check(processor.analysis.pop(sample),"opening a visualizer must enable fresh analysis without changing audio");}
    while(processor.analysis.pop(sample)){}process();check(!processor.analysis.pop(sample),"destroying the last visualizer must stop collecting analysis again");
}
void artifacts(const juce::File& folder){
    folder.createDirectory();Cracked808sAudioProcessor p;p.prepareToPlay(48000,256);p.loadFactoryPreset("Cracked Bass");
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->setTopLeftPosition(-10000,-10000);editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);editor->setVisible(true);juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;
    for(int block=0;block<140;++block){for(int j=0;j<256;++j){float x=.5f*std::sin(2*cracked::pi*73*(float)(block*256+j)/48000)+.12f*std::sin(2*cracked::pi*237*(float)(block*256+j)/48000);b.setSample(0,j,x);b.setSample(1,j,x);}p.processBlock(b,midi);if(block%4==0)juce::MessageManager::getInstance()->runDispatchLoopUntil(2);}
    auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);juce::PNGImageFormat png;{juce::FileOutputStream out(folder.getChildFile("CRACKED808S-main.png"));out.setPosition(0);out.truncate();check(png.writeImageToStream(image,out),"main UI render must write PNG");}
    auto* view=dynamic_cast<Cracked808sAudioProcessorEditor*>(editor.get());view->showAdvanced(true);image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);{juce::FileOutputStream out(folder.getChildFile("CRACKED808S-advanced.png"));out.setPosition(0);out.truncate();png.writeImageToStream(image,out);}
    for(auto* parent:editor->getChildren())for(auto* child:parent->getChildren())if(auto* panel=dynamic_cast<cracked::ui::AdvancedPanel*>(child)){panel->page=4;panel->rebuild();}
    image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);{juce::FileOutputStream out(folder.getChildFile("CRACKED808S-update-settings.png"));out.setPosition(0);out.truncate();png.writeImageToStream(image,out);}
    view->showAdvanced(false);
    auto renderVisual=[&](int mode,const juce::String& name){p.setParameter("visualMode",(float)mode);
        for(int block=0;block<36;++block){for(int j=0;j<256;++j){float x=.4f*std::sin(2*cracked::pi*60*(float)(block*256+j)/48000)+.1f*std::sin(2*cracked::pi*240*(float)(block*256+j)/48000);b.setSample(0,j,x);b.setSample(1,j,x);}p.processBlock(b,midi);juce::MessageManager::getInstance()->runDispatchLoopUntil(3);}
        auto shot=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);juce::FileOutputStream out(folder.getChildFile(name));out.setPosition(0);out.truncate();check(png.writeImageToStream(shot,out),"visualizer screenshot must export");};
    renderVisual(1,"CRACKED808S-sphere.png");
    p.setParameter("visualColor",1);renderVisual(1,"CRACKED808S-particles-color.png");view->showVisualSettings();image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);{juce::FileOutputStream out(folder.getChildFile("CRACKED808S-visual-settings.png"));out.setPosition(0);out.truncate();png.writeImageToStream(image,out);}view->showAdvanced(false);p.setParameter("visualColor",0);
    p.setParameter("visualColor",1);p.setParameter("visualColorMode",1);renderVisual(1,"CRACKED808S-random-bass.png");
    view->showVisualSettings();for(auto* parent:editor->getChildren())for(auto* child:parent->getChildren())if(auto* panel=dynamic_cast<cracked::ui::AdvancedPanel*>(child)){panel->page=6;panel->rebuild();}
    image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1);{juce::FileOutputStream out(folder.getChildFile("CRACKED808S-color-settings.png"));out.setPosition(0);out.truncate();png.writeImageToStream(image,out);}view->showAdvanced(false);
    p.setParameter("visualColorMode",2);renderVisual(1,"CRACKED808S-full-spectrum.png");p.setParameter("visualColorMode",3);p.setParameter("visualHue",.08f);p.setParameter("visualColorSpread",0);renderVisual(1,"CRACKED808S-fixed-hue.png");
    p.setParameter("visualColorMode",0);p.setParameter("visualColor",0);
    auto importFile=folder.getChildFile("PNG import fixture.png");{juce::FileOutputStream out(importFile);out.setPosition(0);out.truncate();out.write(CrackedAssets::CrackedVaultLogo_png,CrackedAssets::CrackedVaultLogo_pngSize);}
    cracked::ui::SpectrumDisplay* display=nullptr;for(auto* parent:editor->getChildren())for(auto* child:parent->getChildren())if(auto* d=dynamic_cast<cracked::ui::SpectrumDisplay*>(child))display=d;
    check(display!=nullptr,"editor must contain actual visualizer");if(display){display->filesDropped({importFile.getFullPathName()},0,0);check(p.getVisualPng().isNotEmpty()&&get(p,"visualMode")==2,"PNG drag/drop must import and select Image mode");}
    renderVisual(2,"CRACKED808S-image.png");p.clearVisualPng();p.setParameter("visualMode",0);
    auto presets=folder.getChildFile("Factory Presets");presets.createDirectory();for(auto n:p.getFactoryPresetNames()){p.loadFactoryPreset(n);auto state=p.parameters.copyState();state.setProperty("schemaVersion",2,nullptr);state.setProperty("presetName",n,nullptr);if(auto xml=state.createXml())xml->writeTo(presets.getChildFile(n+".cracked808s"));}
    p.setParameter("visualMode",1);view->showVisualizerPopout(true);
    if(display){auto* window=display->getTopLevelComponent();juce::MessageManager::getInstance()->runDispatchLoopUntil(40);auto shot=window->createComponentSnapshot(window->getLocalBounds(),true,1);juce::FileOutputStream out(folder.getChildFile("CRACKED808S-popout.png"));out.setPosition(0);out.truncate();check(png.writeImageToStream(shot,out),"actual visualizer-only window must export a screenshot");}
    view->showVisualizerPopout(false);audioExamples(folder);
}
int main(int argc,char** argv){
    juce::ScopedJuceInitialiser_GUI gui;auto t=std::chrono::steady_clock::now();editorOwnedWindows();visualizerPopout();advancedSidebar();visualIdleScheduling();hiddenNativeEditor();analysisOnlyWhenObserved();stateTest();layoutsAndStability();noAllocation();quality();cleanOutputBoost();qualityHistory();latencyAndPerformance();analyzer();factoryLevels();knobGlow(argc>1?juce::File(juce::String::fromUTF8(argv[1])):juce::File());
    updatePolicy();mainPan();panQualitySwitch();monochromeTheme();visualState();visualCustomization();musicDrivenVisuals(argc>1?juce::File(juce::String::fromUTF8(argv[1])):juce::File());colorBehaviors();if(argc>1)artifacts(juce::File(juce::String::fromUTF8(argv[1])));
    std::cout<<"Integration failures: "<<failures<<"; elapsed "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count()<<" s\n";return failures?1:0;
}
