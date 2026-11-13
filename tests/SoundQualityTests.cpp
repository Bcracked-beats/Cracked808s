#include "DSP/AudioEngine.h"
#include "DSP/LookaheadLimiter.h"
#include <iostream>
#include <array>
#include <cmath>
using namespace cracked;
int failures=0;
void check(bool ok,const char* why){if(!ok){++failures;std::cerr<<"FAIL: "<<why<<'\n';}}
struct Measurement {double rms=0,harmonics=0,fundamental=0;};
Measurement saturationMeasure(float amplitude,float drive,float tone=0){
    SaturationEngine engine;engine.prepare(48000);SaturationSettings p;p.drive=drive;p.tone=tone;p.band[0].model=1;
    double sum=0;std::array<double,10> re{},im{};int count=0;
    for(int i=0;i<96000;++i){float phase=2*pi*60*(float)i/48000;float x=amplitude*std::sin(phase);float y=engine.process({x,x},p).l;
        if(i>=48000){sum+=y*y;++count;for(int h=1;h<=9;++h){re[(size_t)h]+=y*std::cos((double)phase*h);im[(size_t)h]+=y*std::sin((double)phase*h);}}}
    Measurement m;m.rms=std::sqrt(sum/count);m.fundamental=std::hypot(re[1],im[1]);double harmonics=0;for(int h=2;h<=9;++h)harmonics+=re[(size_t)h]*re[(size_t)h]+im[(size_t)h]*im[(size_t)h];m.harmonics=std::sqrt(harmonics)/m.fundamental;return m;
}
void driveTest(){
    for(float amp:{.08f,.2f,.5f}){auto m=saturationMeasure(amp,24);double change=20*std::log10(m.rms/(amp/std::sqrt(2.)));std::cout<<"Drive 24 dB, input peak "<<amp<<": RMS change "<<change<<" dB, THD "<<m.harmonics<<'\n';check(std::abs(change)<1.5,"auto gain must preserve RMS across quiet and loud 808s");}
    auto low=saturationMeasure(.12f,0),warm=saturationMeasure(.12f,12),hot=saturationMeasure(.12f,24);
    std::cout<<"harmonic/fundamental: drive0="<<low.harmonics<<" drive12="<<warm.harmonics<<" drive24="<<hot.harmonics<<'\n';
    check(warm.harmonics>.06 && warm.harmonics>low.harmonics*3,"mid Drive must add audible harmonics to a quiet bass");check(hot.harmonics>.18,"high Drive must create strong harmonic weight");
    auto gentle=saturationMeasure(.5f,5),dark=saturationMeasure(.2f,12,-1),bright=saturationMeasure(.2f,12,1);
    std::cout<<"Drive5 harmonic ratio on hotter bass="<<gentle.harmonics<<"; Tone dark/bright="<<dark.harmonics<<"/"<<bright.harmonics<<'\n';
    check(gentle.harmonics<.075,"first few dB of Drive must remain gradual rather than jump to heavy distortion");
    check(bright.harmonics>dark.harmonics*2,"Tone must meaningfully change harmonic color, not mostly level");
}
double widthMeasure(float width,float* monoError=nullptr){
    StereoWidener w;w.prepare(48000);StereoSettings p;p.width=width;p.lowBypass=120;p.monoSafe=true;
    double midEnergy=0,sideEnergy=0;float error=0;
    for(int i=0;i<96000;++i){double t=(double)i/48000;float x=(float)(.22*std::sin(2*pi*60*t)+.12*std::sin(2*pi*180*t)+.08*std::sin(2*pi*300*t)+.05*std::sin(2*pi*420*t)+.04*std::sin(2*pi*900*t)+.03*std::sin(2*pi*2100*t));auto y=w.process({x,x},p);float m=.5f*(y.l+y.r),s=.5f*(y.l-y.r);error=std::max(error,std::abs(m-x));if(i>48000){midEnergy+=m*m;sideEnergy+=s*s;}}
    if(monoError)*monoError=error;return std::sqrt(sideEnergy/midEnergy);
}
void widthTest(){float error=0;double zero=widthMeasure(0),full=widthMeasure(1,&error),max=widthMeasure(2);std::cout<<"side/mid RMS: width0="<<zero<<" width100="<<full<<" width200="<<max<<" mono error="<<error<<'\n';check(zero<1e-8,"zero Width must produce no synthetic side");check(full>.68,"100% Width must make a broader upper-band stereo field");check(max>1.3&&max>full*1.6,"200% Width must be clearly stronger than 100%");check(error<2e-6,"stronger widening must still preserve the mono sum");}
void monoSubTest(){
    auto side=[](float hz){StereoWidener w;w.prepare(48000);StereoSettings p;p.width=2;p.monoSub=true;p.lowEnabled=p.monoSafe=false;p.lowBypass=400;double energy=0;
        for(int n=0;n<48000;++n){float x=.2f*std::sin(2*pi*hz*n/48000);auto y=w.process({x,-x},p);if(n>24000)energy+=(y.l-y.r)*(y.l-y.r);}return energy;};
    check(side(40)<side(1000)*.001,"Mono Sub must also remove sub from newly amplified existing side when Low Bypass is off");
}
void widthHeadroom(){
    AudioEngine engine;engine.prepare(48000);EngineSettings p;p.saturation.mix=0;p.stereo.width=2;p.stereo.lowBypass=120;p.protect=false;double dry=0,wet=0;
    for(int n=0;n<96000;++n){float t=(float)n/48000;float x=.2f*std::sin(2*pi*60*t)+.12f*std::sin(2*pi*180*t)+.08f*std::sin(2*pi*420*t);auto y=engine.process({x,x},p,120);if(n>48000){dry+=x*x;wet+=.5*(y.l*y.l+y.r*y.r);}}
    double db=10*std::log10(wet/dry);std::cout<<"full-engine width200 RMS lift: "<<db<<" dB\n";check(std::abs(db)<.7,"widening must not consume several dB of output headroom just to sound wider");
}
void cutoffAutomation(){
    for(float rate:{48000.f,192000.f,6144000.f}){StereoWidener w;w.prepare(rate);StereoSettings p;p.width=2;p.lowBypass=1000;float peak=0;const int step=(int)(rate*.2f);
        for(int n=0;n<step*2;++n){if(n==step+64)p.lowBypass=20;float x=.5f*std::sin(2*pi*1000*n/rate);auto y=w.process({x,x},p);if(n>step)peak=std::max({peak,std::abs(y.l),std::abs(y.r)});}
        std::cout<<"cutoff 1000->20 Hz, internal rate "<<rate<<", peak "<<peak<<'\n';check(peak<2,"low-cut automation must not inject a large resonant transient");
    }
}
void safetyTests(){
    for(float rate:{44100.f,48000.f,192000.f}){LookaheadLimiter limiter;limiter.prepare(rate);float peak=0,error=0;bool linked=true;
        for(int n=0;n<24000;++n){float x=n>6000&&n<12000?4.f:.1f;auto y=limiter.process({x,x*.25f},true);peak=std::max(peak,std::abs(y.l));linked&=std::abs(y.r-y.l*.25f)<1e-6;if(n>18000)error=std::max(error,std::abs(y.l-.1f));}
        check(peak<1&&linked,"hot transients must stay bounded with linked stereo gain");
        limiter.reset();float transparency=0;for(int n=0;n<3000;++n){float x=.1f*std::sin((float)n*.03f);auto y=limiter.process({x,x},false);if(n>=128)transparency=std::max(transparency,std::abs(y.l-.1f*std::sin((float)(n-128)*.03f)));}check(transparency<1e-6,"disabled safety must only delay, not color audio");
        for(int n=0;n<2000;++n)limiter.process({8,4},false);auto on=limiter.process({8,4},true);check(std::abs(on.l)<1,"enabling protection on a hot stream must not leak an over-ceiling frame");
    }
}
void centeredWidth(){
    double worst=0;for(float rate:{44100.f,48000.f,96000.f})for(float hz:{60.f,110.f,180.f,300.f,700.f,1500.f})for(bool low:{false,true}){
        StereoWidener w;w.prepare(rate);StereoSettings p;p.width=1.5f;p.lowBypass=120;p.lowEnabled=p.monoSafe=low;double left=0,right=0,cross=0,mid=0,side=0;
        for(int n=0;n<(int)(rate*1.5f);++n){float x=.2f*std::sin(2*pi*hz*n/rate);auto y=w.process({x,x},p);if(n>(int)(rate*.7f)){left+=y.l*y.l;right+=y.r*y.r;float s=.5f*(y.l-y.r);cross+=x*s;mid+=x*x;side+=s*s;}}
        const double balance=10*std::log10((left+1e-15)/(right+1e-15));worst=std::max(worst,std::abs(balance));
        if(std::abs(balance)>.35)std::cout<<"width pan: "<<hz<<"Hz @"<<rate<<" low="<<low<<" L/R="<<balance<<"dB, mid/side correlation="<<cross/std::sqrt(std::max(1e-20,mid*side))<<'\n';
        check(std::abs(balance)<.35,"synthetic width must keep a centered mono note centered");
    }std::cout<<"worst mono-width L/R imbalance: "<<worst<<" dB\n";
    StereoWidener w;w.prepare(48000);StereoSettings p;p.width=2;p.synthetic=false;double l=0,r=0;
    for(int n=0;n<48000;++n){float x=.2f*std::sin(2*pi*60*n/48000);auto y=w.process({x,x*.4f},p);if(n>24000){l+=y.l*y.l;r+=y.r*y.r;}}
    check(l>r*2,"centering the synthetic width must not flip an intentionally left-weighted source");
    w.reset();p.synthetic=true;l=r=0;for(int n=0;n<48000;++n){float x=.2f*std::sin(2*pi*60*n/48000);auto y=w.process({x,x*.4f},p);if(n>24000){l+=y.l*y.l;r+=y.r*y.r;}}check(l>r*1.05,"synthetic centering must preserve intentional source pan rather than force the whole signal to center");
    for(bool low:{false,true}){StereoWidener pulse;pulse.prepare(48000);StereoSettings settings;settings.width=1.5f;settings.lowBypass=120;settings.lowEnabled=settings.monoSafe=low;double worstHit=0;
        for(float hz:{55.f,82.f,110.f,146.f}){double le=0,re=0;for(int n=0;n<24000;++n){float t=(float)n/48000,envelope=(1-std::exp(-t/.002f))*std::exp(-t*12);float phase=2*pi*hz*t;float x=envelope*(.3f*std::sin(phase)+.12f*std::sin(phase*3)+.06f*std::sin(phase*5));auto y=pulse.process({x,x},settings);if(n<12000){le+=y.l*y.l;re+=y.r*y.r;}}
            double db=10*std::log10((le+1e-15)/(re+1e-15));worstHit=std::max(worstHit,std::abs(db));}
        std::cout<<"decaying 808 worst-hit L/R imbalance, low="<<low<<": "<<worstHit<<" dB\n";check(worstHit<1,"short decaying 808s must not acquire an obvious stereo lean");
    }
}
int main(){driveTest();widthTest();monoSubTest();widthHeadroom();cutoffAutomation();safetyTests();centeredWidth();std::cout<<"Sound-quality failures: "<<failures<<'\n';return failures?1:0;}
