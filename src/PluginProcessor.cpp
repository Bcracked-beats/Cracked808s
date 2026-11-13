#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/ParameterLayout.h"

namespace {
bool validVisualPng(const juce::MemoryBlock& png){
    if(png.getSize()<24||png.getSize()>8*1024*1024)return false;
    const auto* bytes=static_cast<const unsigned char*>(png.getData());constexpr unsigned char signature[]={137,80,78,71,13,10,26,10};
    if(std::memcmp(bytes,signature,8)!=0)return false;
    const auto w=juce::ByteOrder::bigEndianInt(bytes+16),h=juce::ByteOrder::bigEndianInt(bytes+20);
    if(w==0||h==0||w>4096||h>4096||(uint64_t)w*h>16*1024*1024)return false;
    juce::MemoryInputStream stream(png,false);return juce::PNGImageFormat().decodeImage(stream).isValid();
}
bool displayParameter(const juce::String& id){return id.startsWith("visual")||id=="reducedMotion"||id=="glow"||id=="showPre"||id=="smoothing"||id=="fps";}
}

Cracked808sAudioProcessor::Cracked808sAudioProcessor()
 :AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,&undo,"CRACKED808S",createParameterLayout()),reader(parameters),engine(std::make_unique<cracked::QualityProcessor>()){}
juce::AudioProcessorValueTreeState::ParameterLayout Cracked808sAudioProcessor::createParameterLayout(){return cracked::makeLayout();}
void Cracked808sAudioProcessor::prepareToPlay(double sr,int){engine->prepare(sr);analysis.sampleRate.store((float)sr);setLatencySamples(cracked::QualityProcessor::latency);}
bool Cracked808sAudioProcessor::isBusesLayoutSupported(const BusesLayout& l)const{
    auto in=l.getMainInputChannelSet(),out=l.getMainOutputChannelSet();
    return (in==juce::AudioChannelSet::mono()||in==juce::AudioChannelSet::stereo())&&(out==juce::AudioChannelSet::stereo()||(in==juce::AudioChannelSet::mono()&&out==juce::AudioChannelSet::mono()));
}
void Cracked808sAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){process(b,false);m.clear();}
void Cracked808sAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){process(b,true);m.clear();}
void Cracked808sAudioProcessor::process(juce::AudioBuffer<float>& buffer,bool bypass){
    juce::ScopedNoDenormals guard;auto s=reader.read();s.bypass=s.bypass||bypass;
    double bpm=120;if(auto* play=getPlayHead())if(auto pos=play->getPosition())if(auto tempo=pos->getBpm())bpm=*tempo;
    const auto inputs=getTotalNumInputChannels(),outputs=getTotalNumOutputChannels();
    const bool collectAnalysis=analysis.isObserved();
    std::array<float,256> il{},ir{},ol{},oright{};
    for(int start=0;start<buffer.getNumSamples();start+=256){int n=std::min(256,buffer.getNumSamples()-start);
        for(int k=0;k<n;++k){il[(size_t)k]=cracked::clean(buffer.getSample(0,start+k));ir[(size_t)k]=inputs>1?cracked::clean(buffer.getSample(1,start+k)):il[(size_t)k];}
        engine->process(il.data(),ir.data(),ol.data(),oright.data(),n,s,reader.quality(),bpm);
        for(int k=0;k<n;++k){float l=ol[(size_t)k],r=oright[(size_t)k];if(outputs==1)l=.5f*(l+r);buffer.setSample(0,start+k,l);if(outputs>1)buffer.setSample(1,start+k,r);
            if(collectAnalysis){analysis.push(.5f*(il[(size_t)k]+ir[(size_t)k]),.5f*(l+r));analysis.meter(il[(size_t)k],ir[(size_t)k],l,r);}}
    }
    analysis.modulation.store(engine->modulation(),std::memory_order_relaxed);
    analysis.limiterGain.store(engine->gainReduction(),std::memory_order_relaxed);
}
juce::AudioProcessorEditor* Cracked808sAudioProcessor::createEditor(){return new Cracked808sAudioProcessorEditor(*this);}
void Cracked808sAudioProcessor::setParameter(const juce::String& id,float v,bool gesture){if(auto* p=parameters.getParameter(id)){if(gesture)p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));if(gesture)p->endChangeGesture();}}
juce::ValueTree Cracked808sAudioProcessor::snapshot(){auto state=parameters.copyState();state.setProperty("schemaVersion",2,nullptr);state.setProperty("presetName",presetName,nullptr);return state;}
void Cracked808sAudioProcessor::getStateInformation(juce::MemoryBlock& m){
    const juce::ScopedLock lock(metadataLock);auto state=snapshot();juce::ValueTree compare("COMPARISON"),a("A"),b("B");
    compare.setProperty("activeB",slotB,nullptr);
    a.addChild((!slotB||!slotAState.isValid()?state:slotAState).createCopy(),-1,nullptr);
    b.addChild((slotB||!slotBState.isValid()?state:slotBState).createCopy(),-1,nullptr);
    compare.addChild(a,-1,nullptr);compare.addChild(b,-1,nullptr);state.addChild(compare,-1,nullptr);
    if(auto x=state.createXml())copyXmlToBinary(*x,m);
}
void Cracked808sAudioProcessor::applyState(const juce::ValueTree& state){
    const juce::ScopedLock lock(metadataLock);
    if(!state.isValid()||!state.hasType("CRACKED808S"))return;
    auto base=parameters.copyState();
    for(auto child:base){auto id=child.getProperty("id").toString();if(auto* p=parameters.getParameter(id))child.setProperty("value",p->convertFrom0to1(p->getDefaultValue()),nullptr);}
    for(auto child:state){auto id=child.getProperty("id").toString();if(auto* p=parameters.getParameter(id)){float v=(float)child.getProperty("value");if(std::isfinite(v)){auto entry=base.getChildWithProperty("id",id);if(entry.isValid())entry.setProperty("value",p->getNormalisableRange().snapToLegalValue(v),nullptr);}}}
    if(state.hasProperty("visualPng")){auto text=state.getProperty("visualPng").toString();juce::MemoryBlock png;if(text.isEmpty())base.removeProperty("visualPng",nullptr);else if(text.length()<=12*1024*1024&&png.fromBase64Encoding(text)&&validVisualPng(png))base.setProperty("visualPng",text,nullptr);}else base.removeProperty("visualPng",nullptr);
    parameters.replaceState(base);presetName=state.getProperty("presetName","Custom").toString();
}
void Cracked808sAudioProcessor::setStateInformation(const void* d,int n){if(auto x=getXmlFromBinary(d,n)){
    const juce::ScopedLock lock(metadataLock);auto state=juce::ValueTree::fromXml(*x);if(!state.hasType("CRACKED808S"))return;applyState(state);
    auto compare=state.getChildWithName("COMPARISON");slotB=(bool)compare.getProperty("activeB",false);
    slotAState=compare.getChildWithName("A").getChild(0).createCopy();slotBState=compare.getChildWithName("B").getChild(0).createCopy();
}}
void Cracked808sAudioProcessor::toggleAB(){const juce::ScopedLock lock(metadataLock);auto current=snapshot();if(slotB){slotBState=current;if(slotAState.isValid())applyState(slotAState);}else{slotAState=current;if(slotBState.isValid())applyState(slotBState);else slotBState=current.createCopy();}slotB=!slotB;}
void Cracked808sAudioProcessor::copyAB(){const juce::ScopedLock lock(metadataLock);if(slotB)slotAState=snapshot();else slotBState=snapshot();}

namespace {
struct Preset {const char* name;const char* category;float drive,tone,width,cut,mix,out;int model,bands;bool sub;};
constexpr Preset factory[]={
 {"Pure Sub","Clean 808s",0,-.15f,0,180,.25f,0,0,1,true},{"Clean Weight","Clean 808s",9,.12f,.45f,110,.85f,-1,1,1,true},{"Low End Polish","Clean 808s",6,.15f,.3f,110,.8f,0,4,1,true},{"Sub Presence","Clean 808s",13,.3f,.55f,110,.8f,-1,1,2,true},
 {"Warm Tube","Warm Saturation",12,-.15f,.15f,160,.8f,-2,1,1,false},{"Analog Weight","Warm Saturation",15,-.3f,.2f,180,.75f,-2,8,2,true},{"Soft Tape","Warm Saturation",10,-.2f,.25f,130,.8f,-1,5,1,false},{"Heavy Warmth","Warm Saturation",20,-.25f,.1f,220,.8f,-3,6,2,true},
 {"Cracked Bass","Aggressive 808s",26,.4f,.5f,180,.9f,-3,12,3,true},{"Distorted 808","Aggressive 808s",24,.2f,.2f,180,1,-3,2,2,true},{"Hard Knock","Aggressive 808s",18,.1f,.15f,220,.85f,-3,15,2,true},{"Speaker Destroyer","Aggressive 808s",28,.35f,.3f,240,.75f,-5,20,3,true},
 {"Sub Safe Width","Stereo Enhancement",12,.2f,1.25f,120,.85f,-2,1,2,true},{"Spacious Harmonics","Stereo Enhancement",16,.45f,1.5f,140,.9f,-3,5,3,true},{"Wide Growl","Stereo Enhancement",21,.3f,1.7f,130,.9f,-4,9,3,true},{"Airy Bass","Stereo Enhancement",13,.6f,1.4f,160,.85f,-2,10,2,true},
 {"Cosmic Drive","Creative",20,.3f,1.3f,180,.75f,-3,16,3,true},{"Dark Matter","Creative",18,-.5f,.75f,160,.7f,-3,21,2,true},{"Black Hole","Creative",23,-.25f,1.1f,230,.7f,-4,22,3,true},{"Supernova","Creative",28,.7f,1.8f,300,.7f,-5,18,3,true}};
}
juce::StringArray Cracked808sAudioProcessor::getFactoryPresetNames()const{juce::StringArray a;a.add("Init");for(auto& p:factory)a.add(p.name);return a;}
juce::String Cracked808sAudioProcessor::factoryCategory(int i)const{return i==0?"Start":factory[juce::jlimit(0,19,i-1)].category;}
void Cracked808sAudioProcessor::loadFactoryPreset(const juce::String& name){
    const juce::ScopedLock lock(metadataLock);if(!getFactoryPresetNames().contains(name))return;
    undo.beginNewTransaction("Load preset");
    for(auto* param:getParameters())if(auto* p=dynamic_cast<juce::RangedAudioParameter*>(param))if(!displayParameter(p->getParameterID()))p->setValueNotifyingHost(p->getDefaultValue());
    for(auto& p:factory)if(name==p.name){setParameter("drive",p.drive);setParameter("color",p.tone);setParameter("width",p.width);setParameter("monoBass",p.cut);setParameter("mix",p.mix);setParameter("output",p.out);setParameter("bands",(float)p.bands);setParameter("preserveSub",p.sub?1.f:0.f);for(int i=0;i<6;++i)setParameter("b"+juce::String(i)+"_model",(float)p.model);}
    presetName=name;
}
juce::File Cracked808sAudioProcessor::presetFolder()const{return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Cracked808s Presets");}
juce::StringArray Cracked808sAudioProcessor::getUserPresetNames()const{juce::StringArray a;for(auto f:presetFolder().findChildFiles(juce::File::findFiles,false,"*.cracked808s"))a.add(f.getFileNameWithoutExtension());a.sortNatural();return a;}
bool Cracked808sAudioProcessor::saveUserPreset(juce::String name){const juce::ScopedLock lock(metadataLock);name=juce::File::createLegalFileName(name.trim()).substring(0,80);if(name.isEmpty())return false;auto folder=presetFolder();if(folder.createDirectory().failed())return false;auto state=snapshot();state.setProperty("presetName",name,nullptr);if(auto xml=state.createXml())if(xml->writeTo(folder.getChildFile(name+".cracked808s"))){presetName=name;return true;}return false;}
bool Cracked808sAudioProcessor::loadUserPreset(const juce::String& name){const juce::ScopedLock lock(metadataLock);if(juce::File::createLegalFileName(name)!=name)return false;if(auto xml=juce::XmlDocument::parse(presetFolder().getChildFile(name+".cracked808s"))){auto state=juce::ValueTree::fromXml(*xml);if(!state.hasType("CRACKED808S"))return false;applyState(state);presetName=name;return true;}return false;}
bool Cracked808sAudioProcessor::deleteUserPreset(const juce::String& name){if(name.isEmpty()||juce::File::createLegalFileName(name)!=name)return false;return presetFolder().getChildFile(name+".cracked808s").moveToTrash();}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Cracked808sAudioProcessor();}
bool Cracked808sAudioProcessor::setVisualPng(const juce::MemoryBlock& png){if(!validVisualPng(png))return false;const juce::ScopedLock lock(metadataLock);undo.beginNewTransaction("Import visual PNG");parameters.state.setProperty("visualPng",png.toBase64Encoding(),&undo);return true;}
juce::String Cracked808sAudioProcessor::getVisualPng()const{const juce::ScopedLock lock(metadataLock);return parameters.state.getProperty("visualPng").toString();}
void Cracked808sAudioProcessor::clearVisualPng(){const juce::ScopedLock lock(metadataLock);undo.beginNewTransaction("Reset visual image");parameters.state.removeProperty("visualPng",&undo);}
