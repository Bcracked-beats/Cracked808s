#pragma once
#include "Common.h"
namespace cracked {
// Stereo-linked peak control after downsampling. The delay is part of the
// existing 256-sample latency budget, not an unreported extra delay.
class LookaheadLimiter {
public:
    static constexpr int lookahead=128;
    void prepare(float rate){release=(float)-std::expm1(-1./(rate*.18));enable.prepare(rate,5);reset();}
    void reset(){delay.fill({});pos=0;gain=1;reduction=1;enable.reset(0);}
    Stereo process(Stereo x,bool enabled){
        delay[(size_t)pos]=x;pos=(pos+1)%(lookahead+1);const Stereo delayed=delay[(size_t)pos];
        float peak=0;for(const auto& v:delay)peak=std::max({peak,std::abs(v.l),std::abs(v.r)});
        const float target=peak>.98f?.98f/peak:1.f;
        gain+=(target<gain?.06058694f:release)*(target-gain);
        const float delayedPeak=std::max(std::abs(delayed.l),std::abs(delayed.r));
        const float safe=delayedPeak>0?std::min(gain,.999f/delayedPeak):gain;
        float amount;if(enabled){enable.reset(1);amount=1;}else amount=enable.tick(0);
        const float applied=1+amount*(safe-1);
        reduction=enabled?applied:1.f;
        return {delayed.l*applied,delayed.r*applied};
    }
    float gainReduction()const{return reduction;}
private:
    std::array<Stereo,lookahead+1> delay{};int pos=0;
    float gain=1,release=.0001f,reduction=1;Smooth enable;
};
}
