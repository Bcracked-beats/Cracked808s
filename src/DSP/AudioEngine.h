#pragma once
#include "SaturationEngine.h"
#include "StereoWidener.h"
#include "ModulationEngine.h"
namespace cracked {
struct EngineSettings {SaturationSettings saturation;StereoSettings stereo;ModulationSettings modulation;float input=0,output=0,globalMix=1;bool bypass=false,monoCheck=false,protect=true,widthFirst=false,widthCompensate=true;};
class AudioEngine {
public:
    void prepare(float rate){sat.prepare(rate);widener.prepare(rate);mod.prepare(rate);input.prepare(rate);output.prepare(rate);mix.prepare(rate);bypass.prepare(rate,8);powerAlpha=-std::expm1(-1./(rate*.08));gainAlpha=-std::expm1(-1./(rate*.03));panAlpha=-std::expm1(-1./(rate*.02));reset();}
    void reset(){sat.reset();widener.reset();input.reset(1);output.reset(1);mix.reset(1);bypass.reset(0);widthInput=widthOutput=pan=0;widthGain=1;gainsReady=false;}
    void copyTransitionStateFrom(const AudioEngine& other){mod.copyStateFrom(other.mod);pan=other.pan;}
    Stereo process(Stereo x,const EngineSettings& p,double bpm){
        x={clean(x.l),clean(x.r)};Stereo original=x;
        // These targets change at control rate. Preserve sample-wise smoothing,
        // but do not calculate two identical powers at every oversampled sample.
        if(!gainsReady||p.input!=inputDb){inputDb=p.input;inputTarget=dbGain(p.input);}
        if(!gainsReady||p.output!=outputDb){outputDb=p.output;outputTarget=dbGain(p.output);}gainsReady=true;
        float ig=input.tick(inputTarget),og=output.tick(outputTarget),blend=mix.tick(p.globalMix),bp=bypass.tick(p.bypass?1.f:0.f);
        x.l*=ig;x.r*=ig;
        auto sp=p.saturation;auto wp=p.stereo;
        modulationValue=mod.process(.5f*(x.l+x.r),p.modulation,bpm);
        switch(p.modulation.destination){case 0:sp.drive=clamp(sp.drive+modulationValue*18,0,48);break;case 1:wp.width=clamp(wp.width+modulationValue,0,2);break;case 2:sp.tone=clamp(sp.tone+modulationValue,-1,1);break;case 3:sp.mix=clamp(sp.mix+modulationValue,0,1);break;default:break;}
        Stereo dry=x;
        if(p.widthFirst)x=sat.process(widen(x,wp,p.widthCompensate),sp);else x=widen(sat.process(x,sp),wp,p.widthCompensate);
        x={(dry.l+blend*(x.l-dry.l))*og,(dry.r+blend*(x.r-dry.r))*og};
        const double target=clamp(p.stereo.balance,-1.f,1.f);pan+=panAlpha*(target-pan);if(std::abs(target-pan)<1e-8)pan=target;
        x.l*=(float)std::min(1.,1.-pan);x.r*=(float)std::min(1.,1.+pan);
        // Output safety is stereo-linked and lookahead-based after downsampling.
        x={x.l+bp*(original.l-x.l),x.r+bp*(original.r-x.r)};
        if(p.monoCheck)x={.5f*(x.l+x.r),.5f*(x.l+x.r)};
        return {clean(x.l),clean(x.r)};
    }
    float modulationValue=0;
private:
    Stereo widen(Stereo x,const StereoSettings& p,bool compensate){
        auto y=widener.process(x,p);
        widthInput+=powerAlpha*(.5*((double)x.l*x.l+(double)x.r*x.r)-widthInput);
        widthOutput+=powerAlpha*(.5*((double)y.l*y.l+(double)y.r*y.r)-widthOutput);
        const double target=compensate&&widthOutput>1e-12?std::clamp(std::sqrt(widthInput/widthOutput),.2,1.):1.;
        widthGain+=gainAlpha*(target-widthGain);return {y.l*(float)widthGain,y.r*(float)widthGain};
    }
    double widthInput=0,widthOutput=0,widthGain=1,powerAlpha=.0002,gainAlpha=.0005,pan=0,panAlpha=.001;
    SaturationEngine sat;StereoWidener widener;ModulationEngine mod;Smooth input,output,mix,bypass;
    float inputDb=0,outputDb=0,inputTarget=1,outputTarget=1;bool gainsReady=false;
};
}
