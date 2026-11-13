#include <cmath>
#include <iostream>
#include <random>
#include <chrono>
#include "DSP/AudioEngine.h"
#include "UI/MeterState.h"
using namespace cracked;
static int failures = 0;
void check(bool ok, const char* message) { if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; } }

// Catches replacing the generated complementary side with ordinary stereo-side gain.
void widening()
{
    float maxError = 0;
    for (float sr : {44100.f,48000.f,96000.f,192000.f})
    for (float f : {30.f,40.f,60.f,100.f,200.f,1000.f,5000.f})
    for (float width : {0.f,.5f,1.f,1.5f,2.f})
    {
        StereoWidener w; w.prepare(sr);
        StereoSettings p; p.width=width; p.lowBypass=180; p.monoSafe=true;
        float energy=0;
        for(int n=0;n<12000;++n) {
            float x=.4f*std::sin(6.28318530718f*f*n/sr);
            auto o=w.process({x,x},p);
            maxError=std::max(maxError,std::abs(.5f*(o.l+o.r)-x));
            if(n>5000) energy+=std::abs(o.l-o.r);
        }
        if (f>=1000 && width>0) check(energy>1.f,"mono input must create stereo side above low bypass");
    }
    check(maxError<2e-6f,"widening must preserve mono sum to float tolerance");
    std::mt19937 rng(808); std::uniform_real_distribution<float> noise(-.5f,.5f);
    StereoWidener w; w.prepare(48000); StereoSettings p; p.width=2; p.lowBypass=220;
    for(int n=0;n<24000;++n) { Stereo x{noise(rng),noise(rng)}; auto o=w.process(x,p); maxError=std::max(maxError,std::abs(.5f*(o.l+o.r-x.l-x.r))); }
    check(maxError<2e-6f,"original stereo mid must survive added width");
    auto sideEnergy=[](float f) { StereoWidener v; v.prepare(48000); StereoSettings s; s.width=2; s.lowBypass=400; s.monoSafe=true; double e=0; for(int i=0;i<24000;++i) {float x=.5f*std::sin(6.28318530718f*f*i/48000);auto y=v.process({x,x},s);if(i>12000)e+=(y.l-y.r)*(y.l-y.r);}return e;};
    check(sideEnergy(40)<sideEnergy(1000)*.001,"Mono Safe must attenuate generated sub side");
    std::cout<<"mono-null maximum error: "<<maxError<<'\n';
}

// Catches summing unmatched filters or miscounting the last residual band.
void reconstruction()
{
    std::mt19937 rng(83);std::uniform_real_distribution<float> noise(-1.f,1.f);
    for(int bands=1;bands<=6;++bands) { SaturationEngine sat;sat.prepare(48000);SaturationSettings p;p.bands=bands;p.mix=1;for(auto& b:p.band)b.bypass=true;
        double error=0;for(int n=0;n<32000;++n){Stereo x{noise(rng),noise(rng)};auto o=sat.process(x,p);error=std::max(error,(double)std::max(std::abs(x.l-o.l),std::abs(x.r-o.r)));}
        check(error<3e-6,"bypassed bands must reconstruct original waveform");
    }
}
void saturation()
{
    for(int model=0;model<23;++model){SaturationEngine s;s.prepare(48000);SaturationSettings p;p.drive=24;p.band[0].model=model;
        double delta=0;for(int n=0;n<24000;++n){float x=.3f*std::sin(6.28318530718f*80*n/48000);auto o=s.process({x,x},p);check(std::isfinite(o.l)&&std::abs(o.l)<20,"all models must remain finite and bounded");if(n>12000)delta+=std::abs(o.l-x);}
        check(delta>5,"model and drive must change audio");
    }
}
void modulation()
{
    ModulationEngine m;m.prepare(48000);ModulationSettings p;p.rate=1;p.amount=1;p.wave=0;
    float quarter=0,half=0;for(int n=0;n<24001;++n){float v=m.process(0,p,120);if(n==12000)quarter=v;if(n==24000)half=v;}
    check(quarter>.99f && std::abs(half)<.001,"LFO must keep sample-rate-correct time");
    m.prepare(48000);p.sync=true;p.division=0;float barQuarter=0;for(int n=0;n<=96000;++n)barQuarter=m.process(0,p,120);
    check(barQuarter>.99f,"4-bar sync at 120 BPM must reach quarter-cycle after two seconds");
    ModulationEngine source,target;source.prepare(48000);target.prepare(96000);p.wave=7;p.amount=1;p.attack=10;
    for(int i=0;i<100;++i)source.process(.5f,p,120);
    const float envelope=source.envelope();target.copyStateFrom(source);target.process(1,p,120);
    const float expected=1+(envelope-1)*std::exp(-1.f/(96000*.01f));
    check(std::abs(target.envelope()-expected)<1e-7f,"quality transitions must rebuild cached modulation coefficients for the destination sample rate");
}
void temporalAndSmoothing(){
    AllpassDelay ap;ap.prepare(3000);float leak=0,echo=0;for(int n=0;n<=3000;++n){float y=ap.tick(n==0?1.f:0.f);if(n>0&&n<3000)leak=std::max(leak,std::abs(y));if(n==3000)echo=y;}
    check(leak<1e-6f&&std::abs(echo)>.5f,"all-pass delay must honor long oversampled delay times");
    SaturationEngine s;s.prepare(48000);SaturationSettings p;p.drive=24;p.compensation=false;p.band[0].model=15;Stereo before;for(int n=0;n<25000;++n)before=s.process({.1f,.1f},p);p.mix=0;auto after=s.process({.1f,.1f},p);
    check(std::abs(after.l-before.l)<.002f,"primary saturation Mix must ramp rather than jump");
}
void panning(){
    for(float blend:{0.f,1.f})for(float pan:{-1.f,0.f,1.f}){
        AudioEngine engine;engine.prepare(48000);EngineSettings settings;settings.saturation.mix=0;settings.stereo.width=0;settings.globalMix=blend;settings.stereo.balance=pan;Stereo out;
        for(int n=0;n<24000;++n)out=engine.process({.25f,.25f},settings,120);
        check(std::abs(out.l-(pan>0?0.f:.25f))<1e-5f&&std::abs(out.r-(pan<0?0.f:.25f))<1e-5f,"Pan endpoints/center must work after both dry and wet global mix without a center level drop");
        settings.bypass=true;for(int n=0;n<24000;++n)out=engine.process({.25f,.125f},settings,120);
        check(std::abs(out.l-.25f)<1e-5f&&std::abs(out.r-.125f)<1e-5f,"global bypass must ignore Pan");
    }
    AudioEngine engine;engine.prepare(48000);EngineSettings settings;settings.saturation.mix=0;settings.stereo.width=0;for(int n=0;n<24000;++n)engine.process({.25f,.25f},settings,120);
    settings.stereo.balance=1;auto first=engine.process({.25f,.25f},settings,120);check(first.l>.24f&&first.l<.25f&&std::abs(first.r-.25f)<1e-5f,"Pan automation must ramp rather than instantly mute a channel");
}
void meterInvalidation(){
    using namespace cracked::ui;
    check(meterSegments(.0016f)==4&&meterSegments(.001408f)==3&&meterSegments(.001f)==0,"meter quantization must match its 45 LED drawing at the -60 dB floor");
    check(meterVisualChanged(.0016f,.001408f,0,0),"quiet meter changes must repaint even below the old linear delta threshold");
    check(meterVisualChanged(.00101f,.00099f,0,0),"the final lit meter LED must clear at silence");
    check(!meterVisualChanged(.2f,.20001f,90,89),"unchanged LEDs and a still-lit clip lamp must not redraw for an invisible countdown");
    check(meterVisualChanged(0,0,1,0),"an expired clip lamp must repaint");
}
int main(){auto start=std::chrono::steady_clock::now();widening();reconstruction();saturation();modulation();temporalAndSmoothing();panning();meterInvalidation();std::cout<<"failures: "<<failures<<"; duration "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" s\n";return failures?1:0;}
