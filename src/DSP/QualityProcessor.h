#pragma once
#include "AudioEngine.h"
#include "LookaheadLimiter.h"
#include <juce_dsp/juce_dsp.h>
namespace cracked {
// Fixed, reported latency keeps dry/bypass and quality changes aligned.
class QualityProcessor {
public:
    static constexpr int latency=256,chunkSize=256;
    void prepare(double sr){
        constexpr int powers[]={0,1,3,5};
        limiter.prepare((float)sr);
        for(size_t i=0;i<4;++i){auto& p=paths[i];p.buffer.setSize(2,chunkSize);p.engine.prepare((float)sr*(float)(1<<powers[i]));p.delay.fill({});p.pos=0;p.padding=latency-LookaheadLimiter::lookahead;
            if(i>0){p.os=std::make_unique<juce::dsp::Oversampling<float>>(2,(size_t)powers[i],juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,true);p.os->initProcessing(chunkSize);p.padding-=juce::roundToInt(p.os->getLatencyInSamples());}
        }current=1;pending=-1;fade=1;warmRemaining=0;
    }
    void process(const float* left,const float* right,float* outLeft,float* outRight,int n,const EngineSettings& s,int quality,double bpm){
        quality=std::clamp(quality,0,3);
        if(quality==current)pending=-1;
        else if(quality!=pending){pending=quality;auto& incoming=paths[(size_t)pending];incoming.engine.reset();incoming.engine.copyTransitionStateFrom(paths[(size_t)current].engine);if(incoming.os)incoming.os->reset();incoming.delay.fill({});incoming.pos=0;fade=0;warmRemaining=512;}
        auto run=[&](int index){auto& p=paths[(size_t)index];p.buffer.copyFrom(0,0,left,n);p.buffer.copyFrom(1,0,right,n);
            juce::dsp::AudioBlock<float> outputBlock(p.buffer);outputBlock=outputBlock.getSubBlock(0,(size_t)n);
            auto block=p.os?p.os->processSamplesUp(juce::dsp::AudioBlock<const float>(outputBlock)):outputBlock;
            for(size_t k=0;k<block.getNumSamples();++k){auto v=p.engine.process({block.getSample(0,(int)k),block.getSample(1,(int)k)},s,bpm);block.setSample(0,(int)k,v.l);block.setSample(1,(int)k,v.r);}
            if(p.os)p.os->processSamplesDown(outputBlock);
            for(int k=0;k<n;++k){Stereo x{p.buffer.getSample(0,k),p.buffer.getSample(1,k)};if(p.padding>0){auto delayed=p.delay[(size_t)p.pos];p.delay[(size_t)p.pos]=x;p.pos=(p.pos+1)%p.padding;x=delayed;}p.buffer.setSample(0,k,x.l);p.buffer.setSample(1,k,x.r);}
        };
        run(current);if(pending>=0)run(pending);
        for(int k=0;k<n;++k){outLeft[k]=paths[(size_t)current].buffer.getSample(0,k);outRight[k]=paths[(size_t)current].buffer.getSample(1,k);if(pending>=0){if(warmRemaining>0)--warmRemaining;else fade=std::min(1.f,fade+1.f/2048.f);outLeft[k]+=fade*(paths[(size_t)pending].buffer.getSample(0,k)-outLeft[k]);outRight[k]+=fade*(paths[(size_t)pending].buffer.getSample(1,k)-outRight[k]);}}
        for(int k=0;k<n;++k){auto v=limiter.process({outLeft[k],outRight[k]},s.protect&&!s.bypass);outLeft[k]=v.l;outRight[k]=v.r;}
        if(pending>=0&&fade>=1){current=pending;pending=-1;}
    }
    float modulation()const{return paths[(size_t)current].engine.modulationValue;}
    float gainReduction()const{return limiter.gainReduction();}
private:
    struct Path {AudioEngine engine;juce::AudioBuffer<float> buffer;std::unique_ptr<juce::dsp::Oversampling<float>> os;std::array<Stereo,latency> delay{};int pos=0,padding=latency;};
    std::array<Path,4> paths;int current=1,pending=-1,warmRemaining=0;float fade=1;
    LookaheadLimiter limiter;
};
}
