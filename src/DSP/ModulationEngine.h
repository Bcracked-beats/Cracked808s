#pragma once
#include "Common.h"
namespace cracked {
struct ModulationSettings {float rate=1,amount=0,attack=10,release=160,sensitivity=1,phase=0,glide=.1f;int wave=0,destination=0,division=2;bool sync=false,bipolar=true;std::array<float,16> steps{0,.5f,1,.5f,0,-.5f,-1,-.5f,0,.5f,1,.5f,0,-.5f,-1,-.5f};};
class ModulationEngine {
public:
    void prepare(float rate){sr=rate;phase=0;env=0;random=1;lastStep=-1;smooth=0;lastAttack=lastRelease=-1;}
    float process(float input,const ModulationSettings& p,double bpm){
        if(p.attack!=lastAttack){lastAttack=p.attack;aa=std::exp(-1.f/(sr*std::max(.0001f,p.attack*.001f)));}
        if(p.release!=lastRelease){lastRelease=p.release;ar=std::exp(-1.f/(sr*std::max(.001f,p.release*.001f)));}
        float e=std::abs(input)*p.sensitivity;env=e+(env-e)*(e>env?aa:ar);
        float ph=std::fmod(phase+p.phase+1.f,1.f),value=0;
        int step=(int)(ph*16.f);if(step!=lastStep){random=random*1664525u+1013904223u;randomValue=(float)(random>>8)*(2.f/16777215.f)-1.f;lastStep=step;}
        switch(p.wave){case 0:value=std::sin(2*pi*ph);break;case 1:value=1.f-4.f*std::abs(ph-.5f);break;case 2:value=2*ph-1;break;case 3:value=ph<.5f?1.f:-1.f;break;case 4:value=randomValue;break;case 5:smooth+=.0005f*(randomValue-smooth);value=smooth;break;case 6:value=p.steps[(size_t)step];break;default:value=clamp(env,0,1);break;}
        constexpr float beats[]={16,8,1,.5f,.25f};
        float rate=p.sync?(float)(bpm/60.)/beats[std::clamp(p.division,0,4)]:p.rate;
        phase+=rate/sr;if(phase>=1)phase-=std::floor(phase);
        if(!p.bipolar && p.wave<7)value=.5f+.5f*value;
        return clamp(value*p.amount,-1.f,1.f);
    }
    float envelope() const{return env;}
    void copyStateFrom(const ModulationEngine& other){float rate=sr;*this=other;sr=rate;lastAttack=lastRelease=-1;}
private:float sr=48000,phase=0,env=0,randomValue=0,smooth=0,lastAttack=-1,lastRelease=-1,aa=0,ar=0;int lastStep=-1;uint32_t random=1;
};
}
