#pragma once
#include "Common.h"
namespace cracked {
inline constexpr const char* modelNames[]={"Clean Tube","Warm Tube","Heavy Tube","Broken Tube","Clean Tape","Warm Tape","Heavy Tape","Subtle Transformer","Warm Transformer","Driven Transformer","Clean Amp","Vintage Amp","Crunch Amp","Heavy Amp","Soft Clip","Hard Clip","Foldback","Rectify","Bitcrusher","Rate Reduction","Destroy","Smudge","Breakdown"};
struct BandSettings {float drive=0,tone=0,mix=1,level=0,dynamics=0,feedback=0,feedbackHz=600,bass=0,mid=0,treble=0,presence=0,driveBalance=0,levelBalance=0;int model=0;bool bypass=false,mute=false,solo=false,midSide=false;};
struct SaturationSettings {float drive=0,tone=0,mix=1;int bands=1;bool compensation=true,preserveSub=false;std::array<float,5> crossovers{100,350,1200,4000,9000};std::array<BandSettings,6> band;};

class DistortionChannel {
public:
    void prepare(float rate){sr=rate;scale=rate/48000;loA=coefficient(120,rate);hiA=coefficient(3500,rate);dcPole=std::exp(-2*pi*8/rate);tapeA=coefficient(800,rate);heavyTapeA=coefficient(420,rate);ampA=coefficient(6500,rate);smear.prepare((int)(rate*.0017f));pitch.resize((size_t)std::ceil(2048*scale));reset();}
    void reset(){lo={};hi={};ampPost={};dc={};memory=held=phase=0;holdCounter=pitchPos=0;smear.reset();std::fill(pitch.begin(),pitch.end(),0.f);}
    float process(float input,float gain,int model){
        float x=clamp(input*gain,-32.f,32.f);
        float low=lo.tick(x,loA),high=x-hi.tick(x,hiA),v=x;
        switch(model){
        case 0:v=std::tanh(x);break;
        case 1:v=(std::tanh(x+.18f)-std::tanh(.18f))*1.04f;break;
        case 2:v=std::tanh(1.5f*x+.3f*std::tanh(x*x))*.86f;break;
        case 3:v=std::tanh(x>0?x*1.5f:x*.5f)+.11f*std::sin(x*3.f);break;
        case 4:v=std::atan(x*1.2f)*.82f;break;
        case 5:memory+=tapeA*(x-memory);v=std::tanh(x+.14f*memory)-.03f*high;break;
        case 6:memory+=heavyTapeA*(x-memory);v=std::atan(1.8f*x+.2f*memory)*.6f-.07f*high;break;
        case 7:v=x+.12f*(std::tanh(low*2)-low*2);break;
        case 8:v=std::tanh(x+.35f*low)-.16f*high;break;
        case 9:v=std::tanh(x+.65f*low+.15f*low*low)*.85f-.08f*high;break;
        case 10:v=std::tanh(x+.3f*high);break;
        case 11:v=std::tanh(std::tanh(x+.5f*high)*1.4f+.12f)-std::tanh(.12f);break;
        case 12:v=std::tanh(2.f*std::tanh(x+high))* .88f;break;
        case 13:v=std::tanh(3.f*std::tanh(1.7f*x+high));v=ampPost.tick(v,ampA);break;
        case 14:v=x/(1+std::abs(x));break;
        case 15:v=clamp(x,-.9f,.9f);break;
        case 16:v=(2/pi)*std::asin(std::sin(x*pi*.5f));break;
        case 17:v=.7f*std::abs(x)+.3f*x;break;
        case 18:v=std::round(clamp(x,-1.f,1.f)*31.f)/31.f;break;
        case 19:if(++holdCounter>=(int)std::max(2.f,sr/3200.f)){holdCounter=0;held=std::tanh(x);}v=held;break;
        case 20:if(++holdCounter>=(int)std::max(2.f,sr/2100.f)){holdCounter=0;held=std::round(clamp(x,-1.f,1.f)*7)/7;}v=clamp(held*1.7f,-.85f,.85f);break;
        case 21:v=.55f*std::tanh(x)+.45f*smear.tick(std::tanh(x));break;
        case 22:{ // Two moving read heads, crossfaded: granular downward pitch degradation.
            pitch[(size_t)pitchPos]=std::tanh(x);phase+=.00037f*48000/sr;if(phase>=1)phase-=1;
            float ph2=std::fmod(phase+.5f,1.f);auto read=[&](float ph){float at=(float)pitchPos-(48.f+ph*720.f)*scale;while(at<0)at+=(float)pitch.size();int i=(int)at;float f=at-i;return pitch[(size_t)i]*(1-f)+pitch[((size_t)i+1)%pitch.size()]*f;};
            float a=1.f-std::abs(phase*2-1),b=1.f-std::abs(ph2*2-1);v=a*read(phase)+b*read(ph2);pitchPos=(pitchPos+1)%(int)pitch.size();break;}
        default:break;
        }
        return dc.tick(v,dcPole);
    }
private:
    float sr=48000,scale=1,loA=.01f,hiA=.1f,dcPole=.999f,memory=0,held=0,phase=0,tapeA=.1f,heavyTapeA=.1f,ampA=.1f;int holdCounter=0,pitchPos=0;
    Lowpass lo,hi,ampPost;DCBlock dc;AllpassDelay smear;std::vector<float> pitch;
};

class BandProcessor {
public:
    void prepare(float rate){
        sr=rate;for(auto& c:channel)c.prepare(rate);
        attack=std::exp(-1/(rate*.004f));release=std::exp(-1/(rate*.1f));
        smoothing=1-std::exp(-1/(rate*.01f));modelStep=1/(rate*.005f);
        powerAlpha=(float)-std::expm1(-1.0/(rate*.09));gainAlpha=(float)-std::expm1(-1.0/(rate*.035));
        toneA={coefficient(120,rate),coefficient(850,rate),coefficient(4500,rate),coefficient(9000,rate)};preToneA=coefficient(180,rate);reset();
    }
    void reset(){for(auto& c:channel)c.reset();env=0;tones={};preTone={};fb={};cacheValid=false;controlCounter=0;modelGain=1;dryPower=wetPower=0;autoGain=1;powerReady=false;}
    Stereo process(Stereo x,const BandSettings& p,float globalDrive,float globalTone,bool compensate){
        if((controlCounter++&31)==0)configure(p,globalDrive,globalTone,compensate);
        for(size_t i=0;i<values.size();++i)values[i]+=smoothing*(target[i]-values[i]);
        if(currentModel!=p.model||currentMidSide!=p.midSide){modelGain=std::max(0.f,modelGain-modelStep);if(modelGain==0){currentModel=p.model;currentMidSide=p.midSide;for(auto& c:channel)c.reset();}}
        else modelGain=std::min(1.f,modelGain+modelStep);
        float processing=values[14]*modelGain*values[15];
        if(processing<1e-8f && p.bypass)return x;
        const Stereo original=x;
        float peak=std::max(std::abs(x.l),std::abs(x.r));env=peak+(env-peak)*(peak>env?attack:release);
        float dynamic=1;
        if(values[13]>.0001f)dynamic=std::pow(std::max(env,.18f)/.18f,-values[13]*.65f);
        else if(values[13]<-.0001f)dynamic=std::pow(std::min(env/.15f,1.f),-values[13]*2.f);
        if(currentMidSide)x={.5f*(x.l+x.r),.5f*(x.l-x.r)};
        const Stereo drySignal=x;Stereo wetSignal;
        float* data[]={&x.l,&x.r};
        float* wetData[]={&wetSignal.l,&wetSignal.r};
        for(size_t c=0;c<2;++c){
            const float dry=*data[c];
            const float emphasized=dry+values[16]*(dry-preTone[c].tick(dry,preToneA));
            const float feedback=values[2]==0.f?0.f:values[2]*clamp(env/.003f,0,1)*std::tanh(fb[c].z);
            float wet=channel[c].process(emphasized*dynamic+feedback,values[0]*values[7+c],currentModel);
            fb[c].tick(wet,values[3]);
            std::array<float,4> part{};for(size_t j=0;j<4;++j)part[j]=tones[c][j].tick(wet,toneA[j]);
            wet+=part[0]*values[9]+(part[1]-part[0])*values[10]+(part[2]-part[1])*values[11]+(wet-part[3])*values[12];
            *wetData[c]=wet;
        }
        // Match measured energy, not a fixed attenuation proportional to Drive.
        // Linked gain preserves the channel balance and the generated harmonics.
        dryPower+=powerAlpha*(.5*(drySignal.l*drySignal.l+drySignal.r*drySignal.r)-dryPower);
        wetPower+=powerAlpha*(.5*(wetSignal.l*wetSignal.l+wetSignal.r*wetSignal.r)-wetPower);
        if(dryPower>1e-9&&wetPower>1e-11){float targetGain=clamp((float)std::sqrt(dryPower/wetPower),.02f,3.f);if(!powerReady){autoGain=targetGain;powerReady=true;}else autoGain+=gainAlpha*(targetGain-autoGain);}
        float matchedGain=1+values[1]*(autoGain-1);
        x.l=(drySignal.l+values[4]*(wetSignal.l*matchedGain-drySignal.l))*values[5];
        x.r=(drySignal.r+values[4]*(wetSignal.r*matchedGain-drySignal.r))*values[6];
        if(currentMidSide)x={x.l+x.r,x.l-x.r};
        return {clean(original.l+processing*(x.l-original.l)),clean(original.r+processing*(x.r-original.r))};
    }
private:
    void configure(const BandSettings& p,float drive,float tone,bool compensate){
        std::array<float,18> inputs{p.drive,drive,p.tone,tone,p.mix,p.level,p.dynamics,p.feedback,p.feedbackHz,p.bass,p.mid,p.treble,p.presence,p.driveBalance,p.levelBalance,compensate?1.f:0.f,p.bypass?1.f:0.f,0};
        if(cacheValid && inputs==cached)return;
        cached=inputs;
        float d=clamp(drive+p.drive,0.f,48.f),t=clamp(tone+p.tone,-1.f,1.f);
        const float ramp=clamp(d/12.f,0,1),driveBlend=.35f+.65f*ramp*ramp*(3-2*ramp);
        target={1+1.8f*(dbGain(d)-1),compensate?1.f:0.f,clamp(p.feedback,-.92f,.92f),coefficient(p.feedbackHz,sr),
            p.mix,dbGain(clamp(p.level-p.levelBalance*9,-36.f,24.f)),dbGain(clamp(p.level+p.levelBalance*9,-36.f,24.f)),
            dbGain(-p.driveBalance*9),dbGain(p.driveBalance*9),
            dbGain(p.bass-2*t)-1,dbGain(p.mid+4*t)-1,dbGain(p.treble+6*t)-1,dbGain(p.presence+3*t)-1,p.dynamics,p.bypass?0.f:1.f,driveBlend,dbGain(6*t)-1};
        if(!cacheValid){values=target;currentModel=p.model;currentMidSide=p.midSide;cacheValid=true;}
    }
    float sr=48000,env=0,attack=.99f,release=.999f,smoothing=.002f,modelGain=1,modelStep=.001f;
    double dryPower=0,wetPower=0;float powerAlpha=.0002f,gainAlpha=.0006f,autoGain=1;bool powerReady=false;
    unsigned controlCounter=0;int currentModel=0;bool cacheValid=false,currentMidSide=false;
    float preToneA=.04f;std::array<Lowpass,2> preTone;
    std::array<float,18> cached{};std::array<float,17> target{},values{};
    std::array<DistortionChannel,2> channel;
    std::array<std::array<Lowpass,4>,2> tones{};std::array<float,4> toneA{};std::array<Lowpass,2> fb{};
};

class SaturationEngine {
public:
    void prepare(float rate){sr=rate;for(auto& b:band)b.prepare(rate);for(auto& s:audibility)s.prepare(rate,5);subA=coefficient(65,rate);mix.prepare(rate);reset();}
    void reset(){for(auto& b:band)b.reset();for(auto& s:audibility)s.reset(1);split={};sub={};last.fill(-1);mix.reset(1);}
    Stereo process(Stereo x,const SaturationSettings& p){
        int count=std::clamp(p.bands,1,6);Stereo remaining=x,result{};
        bool solo=false;for(int i=0;i<count;++i)solo|=p.band[(size_t)i].solo;
        for(int b=0;b<count;++b){Stereo v=remaining;
            if(b<count-1){float cut=clamp(p.crossovers[(size_t)b],20.f,sr*.45f);if(cut!=last[(size_t)b]){last[(size_t)b]=cut;a[(size_t)b]=coefficient(cut,sr);}v={split[(size_t)b][0].tick(remaining.l,a[(size_t)b]),split[(size_t)b][1].tick(remaining.r,a[(size_t)b])};remaining.l-=v.l;remaining.r-=v.r;}
            auto& bs=p.band[(size_t)b];Stereo processed=band[(size_t)b].process(v,bs,p.drive,p.tone,p.compensation);
            float audible=audibility[(size_t)b].tick(!bs.mute&&(!solo||bs.solo)?1.f:0.f);result.l+=processed.l*audible;result.r+=processed.r*audible;
        }
        if(p.preserveSub){result.l+=sub[0].tick(x.l-result.l,subA);result.r+=sub[1].tick(x.r-result.r,subA);}
        float blend=mix.tick(p.mix);return {x.l+blend*(result.l-x.l),x.r+blend*(result.r-x.r)};
    }
private:
    float sr=48000,subA=.01f;Smooth mix;std::array<Smooth,6> audibility;
    std::array<BandProcessor,6> band;std::array<std::array<Lowpass,2>,5> split{};
    std::array<float,5> a{},last{};std::array<Lowpass,2> sub{};
};
}
