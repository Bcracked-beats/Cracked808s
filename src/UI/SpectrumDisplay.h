#pragma once
#include "SpaceTheme.h"
#include "ParticleField.h"
#include "ColorBehavior.h"
#include "../Analysis/AnalysisData.h"
#include "../PluginProcessor.h"
namespace cracked::ui {
class SpectrumDisplay : public juce::Component,public juce::FileDragAndDropTarget {
public:
    explicit SpectrumDisplay(Cracked808sAudioProcessor& p):processor(p){processor.analysis.addConsumer();}
    ~SpectrumDisplay()override{chooser.reset();if(observed)processor.analysis.removeConsumer();}
    void setAnalysisEnabled(bool enable){if(enable==observed)return;observed=enable;if(enable)processor.analysis.addConsumer();else processor.analysis.removeConsumer();}
    bool update(){
        const double now=juce::Time::getMillisecondCounterHiRes()*.001;
        const float idleDt=(float)juce::jlimit(1./120.,.1,now-lastTime);lastTime=now;
        const float sensitivity=setting("visualSensitivity"),attack=.001f*setting("visualAttack"),release=.001f*setting("visualRelease");
        ColorSettings colors;colors.enabled=setting("visualColor")>.5f;colors.frozen=reduced();colors.mode=(int)setting("visualColorMode");colors.sensitivity=setting("visualBassSensitivity");colors.gapMs=setting("visualHitGap");colors.transitionMs=setting("visualColorTransition");colors.hue=setting("visualHue");
        auto advance=[&](float dt,bool audio){
        const float coefficients[]={1-std::exp(-dt/attack),1-std::exp(-dt/release),1-std::exp(-dt/(release*.45f)),1-std::exp(-dt/(release*.7f)),1-std::exp(-dt/(release*.5f))};
        auto mapped=[&](float amplitude){return audio&&amplitude>.0001f?1-std::exp(-amplitude*2.4f*sensitivity):0.f;};
        auto follow=[&](float& value,float target,int releaseKind=0){value+=coefficients[target>value?0:1+releaseKind]*(target-value);if(std::abs(value)<.0001f)value=0;};
        follow(level,mapped(analyzer.rms));follow(hit,mapped(analyzer.onset*2),1);
        for(size_t i=0;i<profile.size();++i){const int bin=(int)(i*(SpectrumAnalyzer::bins-1)/(profile.size()-1));follow(profile[i],mapped(analyzer.magnitude[(size_t)bin]),2);}
        for(size_t i=0;i<waveform.size();++i){const float x=audio?analyzer.waveform[i]*sensitivity:0;follow(waveform[i],x/(.5f+std::abs(x)),3);}
        follow(centroid,audio&&analyzer.rms>.0001f?analyzer.centroid:0);
        const float measured=setting("visualColorSpeed")*(centroid*1.4f+hit*.35f+level*.12f);
        colorBehavior.advance(dt,audio?analyzer.bassFast:0,audio?analyzer.bassSlow:0,audio?analyzer.fastRms:0,colors,colors.mode==2?measured*2.5f:measured);
        };
        // Fixed source-time hops preserve short-hit timing at both UI frame rates.
        analyzer.update(processor.analysis,setting("smoothing"),[&](float dt){advance(dt,true);});
        if(analyzer.samplesRead==0)advance(idleDt,false);
        colorPhase=colorBehavior.hue();
        bool imageChanged=false;
        if(setting("visualMode")>1.5f){auto encoded=processor.getVisualPng();if(encoded!=imageState||!image.isValid()){
            imageChanged=true;
            imageState=encoded;juce::MemoryBlock bytes;juce::Image source;
            if(encoded.isNotEmpty()&&bytes.fromBase64Encoding(encoded))source=juce::ImageFileFormat::loadFrom(bytes.getData(),bytes.getSize());
            image=filteredImage(alphaFitted(source.isValid()?source:brandLogo()),768);
            glowImage=juce::Image(juce::Image::ARGB,image.getWidth()+72,image.getHeight()+72,true);
            {juce::Graphics shadow(glowImage);shadow.addTransform(juce::AffineTransform::translation(36,36));juce::DropShadow(silver().withAlpha(.6f),24,{}).drawForImage(shadow,image);}
        }}
        constexpr const char* ids[]={"visualMode","visualAmount","visualSensitivity","visualAttack","visualRelease","visualColor","visualColorMode","visualBassSensitivity","visualHitGap","visualColorTransition","visualHue","visualColorSpeed","smoothing","showPre","visualScale","visualMotionDepth","reducedMotion","visualMotion","visualGlow","visualAudioGlow","visualPalette","visualSaturation","visualColorSpread","visualSpeed","visualPointSize","visualDensity"};
        std::array<float,26> settings{};for(size_t i=0;i<settings.size();++i)settings[i]=setting(ids[i]);
        const auto zero=[](float value){return value==0;};
        const bool settled=!editBands&&level==0&&hit==0&&centroid==0&&std::all_of(profile.begin(),profile.end(),zero)&&std::all_of(waveform.begin(),waveform.end(),zero)&&*std::max_element(analyzer.post.begin(),analyzer.post.end())<=-89.9f&&*std::max_element(analyzer.pre.begin(),analyzer.pre.end())<=-89.9f;
        const float hueDelta=colorPhase-renderedHue;
        const bool hueChanged=colors.enabled&&std::abs(hueDelta-std::floor(hueDelta+.5f))>.0001f;
        const bool changed=!settled||!settledLastFrame||settings!=lastSettings||imageChanged||hueChanged;settledLastFrame=settled;lastSettings=settings;
        if(changed){renderedHue=colorPhase;repaint();}return changed;
    }
    void choosePng(){
        chooser=std::make_unique<juce::FileChooser>("Choose a reactive PNG",juce::File{},"*.png");
        chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe=juce::Component::SafePointer<SpectrumDisplay>(this)](const juce::FileChooser& c){if(safe&&c.getResult().existsAsFile())safe->importPng(c.getResult());});
    }
    bool isInterestedInFileDrag(const juce::StringArray& files)override{return files.size()==1&&files[0].endsWithIgnoreCase(".png");}
    void filesDropped(const juce::StringArray& files,int,int)override{if(files.size()==1)importPng(juce::File(files[0]));}
    void importPng(const juce::File& file){
        juce::MemoryBlock bytes;
        if(file.getSize()>8*1024*1024||!file.loadFileAsData(bytes)||!processor.setVisualPng(bytes)){
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"PNG not imported","Use a valid PNG up to 8 MB, 4096 pixels per side, and 16 million pixels. Your previous image is unchanged.",{},this);return;
        }
        processor.setParameter("visualMode",2);update();
    }
    void paint(juce::Graphics& g)override{
        const int mode=editBands?0:(int)processor.parameters.getRawParameterValue("visualMode")->load();
        if(mode==1){paintSphere(g);return;}if(mode==2){paintImage(g);return;}
        auto r=plot();const float bloom=visualGlow();const auto tint=visualTint(.15f);
        const float hz[]={20,50,100,200,500,1000,2000,5000,10000,20000};const char* labels[]={"20","50","100","200","500","1k","2k","5k","10k","20k"};
        for(int i=0;i<10;++i){float x=r.getX()+std::log(hz[i]/20)/std::log(1000.f)*r.getWidth();g.setColour(silver().withAlpha(.045f));g.drawVerticalLine((int)x,r.getY(),r.getBottom());text(g,labels[i],{x-19,r.getBottom()+10,38,16},9,.65f,.03f);}
        for(int db=0;db>=-72;db-=12){float y=yFor((float)db);g.setColour(silver().withAlpha(.06f));g.drawHorizontalLine((int)y,r.getX(),r.getRight());text(g,juce::String(db),{0,y-8,30,16},9,.6f,0);}
        text(g,"dB",{0,0,30,18},9,.5f,0);
        juce::Path post,pre,fill;juce::Point<float> previousPost,previousPre;
        for(int i=0;i<SpectrumAnalyzer::bins;++i){float x=r.getX()+r.getWidth()*(float)i/(SpectrumAnalyzer::bins-1);float y=yFor(analyzer.post[(size_t)i]),yp=yFor(analyzer.pre[(size_t)i]);
            juce::Point<float> currentPost{x,y},currentPre{x,yp};
            if(i==0){post.startNewSubPath(currentPost);pre.startNewSubPath(currentPre);}else{post.quadraticTo(previousPost,(previousPost+currentPost)*.5f);pre.quadraticTo(previousPre,(previousPre+currentPre)*.5f);}
            previousPost=currentPost;previousPre=currentPre;
        }
        post.lineTo(previousPost);pre.lineTo(previousPre);fill=post;fill.lineTo(r.getRight(),r.getBottom());fill.lineTo(r.getX(),r.getBottom());fill.closeSubPath();g.setGradientFill(juce::ColourGradient(tint.withAlpha(juce::jlimit(0.f,.3f,.14f*bloom)),0,r.getY(),juce::Colours::transparentWhite,0,r.getBottom(),false));g.fillPath(fill);
        if(processor.parameters.getRawParameterValue("showPre")->load()>.5f){g.setColour(silver().withAlpha(.18f));g.strokePath(pre,juce::PathStrokeType(.8f));}
        for(int j=5;j>=1;--j){g.setColour(tint.withAlpha(juce::jlimit(0.f,.15f,.018f*bloom)));g.strokePath(post,juce::PathStrokeType((float)j*2));}g.setGradientFill(juce::ColourGradient(visualTint(0).withAlpha(.95f),r.getX(),0,visualTint(.55f).withAlpha(.95f),r.getRight(),0,false));g.strokePath(post,juce::PathStrokeType(1.3f));
        if(editBands){int count=(int)processor.parameters.getRawParameterValue("bands")->load();for(int i=0;i<count-1;++i){float f=processor.parameters.getRawParameterValue("crossover"+juce::String(i))->load();float x=r.getX()+std::log(f/20)/std::log(1000.f)*r.getWidth();g.setColour(silver().withAlpha(i==drag?.8f:.25f));g.drawVerticalLine((int)x,r.getY(),r.getBottom());g.fillEllipse(x-3,r.getY()+3,6,6);text(g,juce::String((int)f)+" Hz",{x-35,r.getY()+12,70,18},9,.65f,0);}}
        g.setColour(silver().withAlpha(.18f));g.drawHorizontalLine((int)r.getBottom(),r.getX(),r.getRight());
    }
    void mouseDown(const juce::MouseEvent& e)override{if(!editBands)return;int n=(int)processor.parameters.getRawParameterValue("bands")->load();float best=12;drag=-1;for(int i=0;i<n-1;++i){float f=processor.parameters.getRawParameterValue("crossover"+juce::String(i))->load();float x=plot().getX()+std::log(f/20)/std::log(1000.f)*plot().getWidth();if(std::abs(e.position.x-x)<best){best=std::abs(e.position.x-x);drag=i;}}if(drag>=0){processor.undo.beginNewTransaction("Move crossover");processor.parameters.getParameter("crossover"+juce::String(drag))->beginChangeGesture();}}
    void mouseDrag(const juce::MouseEvent& e)override{if(drag<0)return;int n=(int)processor.parameters.getRawParameterValue("bands")->load();float hz=20*std::pow(1000.f,(e.position.x-plot().getX())/plot().getWidth());float lo=drag==0?20:processor.parameters.getRawParameterValue("crossover"+juce::String(drag-1))->load()+5;float hi=drag==n-2?20000:processor.parameters.getRawParameterValue("crossover"+juce::String(drag+1))->load()-5;processor.setParameter("crossover"+juce::String(drag),juce::jlimit(lo,std::max(lo,hi),hz),false);}
    void mouseUp(const juce::MouseEvent&)override{if(drag>=0)processor.parameters.getParameter("crossover"+juce::String(drag))->endChangeGesture();drag=-1;}
    bool editBands=false;
private:
    float setting(const char* id)const{return processor.parameters.getRawParameterValue(id)->load();}
    float amount()const{return processor.parameters.getRawParameterValue("visualAmount")->load();}
    bool reduced()const{return processor.parameters.getRawParameterValue("reducedMotion")->load()>.5f;}
    bool moving()const{return !reduced()&&setting("visualMotion")>.5f;}
    float visualGlow()const{return setting("visualGlow")*(setting("visualAudioGlow")>.5f?.35f+amount()*(level*1.6f+hit*1.8f):.8f);}
    juce::Colour visualTint(float position)const{const int mode=(int)setting("visualColorMode");const float spread=mode==0?1.f:setting("visualColorSpread");return setting("visualColor")>.5f?behaviorColour(position*spread+colorPhase,(int)setting("visualPalette"),mode!=0,setting("visualSaturation")):silver();}
    juce::Rectangle<float> field()const{return getLocalBounds().toFloat().reduced(26,16).withTrimmedBottom(16);}
    void paintSphere(juce::Graphics& g){
        const auto r=field();ParticleStyle style;style.level=level;style.hit=hit;style.profile=profile;style.waveform=waveform;style.detail=setting("visualSpeed");style.amount=amount();style.motionDepth=setting("visualMotionDepth");style.colorPhase=colorPhase;style.glow=setting("visualGlow");style.size=setting("visualPointSize");style.scale=setting("visualScale")*std::max(1.f,getHeight()/296.f);style.density=(int)setting("visualDensity");style.palette=(int)setting("visualPalette");style.motion=moving();style.audioGlow=setting("visualAudioGlow")>.5f;style.colors=setting("visualColor")>.5f;
        const int colorMode=(int)setting("visualColorMode");style.fullColor=colorMode!=0;style.colorSpread=colorMode==0?1.f:setting("visualColorSpread");style.saturation=setting("visualSaturation");
        particles.paint(g,r,style);text(g,"PARTICLE FIELD",{r.getCentreX()-100,r.getBottom()+5,200,18},9,.55f,.18f);
    }
    void paintImage(juce::Graphics& g){
        const auto r=field();const float react=amount()*(level*.65f+hit*.8f),scale=juce::jlimit(.5f,1.12f,setting("visualScale")*(moving()?.84f+react*.38f*setting("visualMotionDepth"):1.f));
        auto bounds=r.reduced(r.getWidth()*.22f,16).withSizeKeepingCentre(r.getWidth()*.56f*scale,(r.getHeight()-32)*scale);
        const auto& source=image.isValid()?image:brandLogo();g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        if(glowImage.isValid()){const auto fitted=juce::RectanglePlacement(juce::RectanglePlacement::centred).appliedTo(source.getBounds().toFloat(),bounds);const float margin=36*fitted.getWidth()/source.getWidth();g.setOpacity(juce::jlimit(0.f,1.f,visualGlow()*.65f));g.drawImage(glowImage,fitted.expanded(margin));}
        g.setOpacity(1);if(setting("visualColor")>.5f){g.setColour(visualTint(.2f));g.drawImage(source,bounds,juce::RectanglePlacement::centred,true);}else g.drawImage(source,bounds,juce::RectanglePlacement::centred);
        text(g,imageState.isEmpty()?"YOUR MARK / AUDIO REACTIVE":"CUSTOM PNG / AUDIO REACTIVE",{r.getCentreX()-200,r.getBottom()+5,400,18},9,.5f,.16f);
    }
    juce::Rectangle<float> plot()const{return {38,22,(float)getWidth()-48,(float)getHeight()-58};}
    float yFor(float db)const{auto r=plot();return r.getBottom()-juce::jlimit(0.f,1.f,(db+78)/84)*r.getHeight();}
    Cracked808sAudioProcessor& processor;SpectrumAnalyzer analyzer;ParticleField particles;int drag=-1;float level=0,hit=0,centroid=0,colorPhase=0;double lastTime=0;
    std::array<float,48> profile{};std::array<float,96> waveform{};
    ColorBehavior colorBehavior;
    juce::Image image,glowImage;juce::String imageState;std::unique_ptr<juce::FileChooser> chooser;
    bool observed=true,settledLastFrame=false;float renderedHue=0;std::array<float,26> lastSettings{};
};
}
