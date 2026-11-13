#pragma once
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>

namespace cracked {
struct AnalysisSample {float pre=0,post=0;};
class AnalysisData {
public:
    void addConsumer() noexcept {if(consumers.fetch_add(1,std::memory_order_acq_rel)==0){tail.store(head.load(std::memory_order_acquire),std::memory_order_release);for(auto& peak:peaks)peak.store(0,std::memory_order_relaxed);}}
    void removeConsumer() noexcept {consumers.fetch_sub(1,std::memory_order_acq_rel);}
    bool isObserved()const noexcept{return consumers.load(std::memory_order_acquire)>0;}
    void push(float pre,float post) noexcept {
        const auto h=head.load(std::memory_order_relaxed),next=(h+1)&mask;
        if(next==tail.load(std::memory_order_acquire))return;
        data[h]={pre,post};head.store(next,std::memory_order_release);
    }
    bool pop(AnalysisSample& sample) noexcept {
        const auto t=tail.load(std::memory_order_relaxed);
        if(t==head.load(std::memory_order_acquire))return false;
        sample=data[t];tail.store((t+1)&mask,std::memory_order_release);return true;
    }
    void meter(float il,float ir,float ol,float oright) noexcept {
        const float v[]={std::abs(il),std::abs(ir),std::abs(ol),std::abs(oright)};
        for(size_t i=0;i<4;++i){auto old=peaks[i].load(std::memory_order_relaxed);while(v[i]>old&&!peaks[i].compare_exchange_weak(old,v[i],std::memory_order_relaxed)){} }
        ll=.9995f*ll+.0005f*ol*ol;rr=.9995f*rr+.0005f*oright*oright;lr=.9995f*lr+.0005f*ol*oright;
        correlation.store(lr/std::sqrt(std::max(1.e-12f,ll*rr)),std::memory_order_relaxed);
        side.store(std::abs(ol-oright)*.5f,std::memory_order_relaxed);
    }
    std::array<std::atomic<float>,4> peaks{};
    std::atomic<float> correlation{1},side{0},modulation{0},sampleRate{48000},limiterGain{1};
private:
    static constexpr unsigned mask=32767;
    std::array<AnalysisSample,mask+1> data{};
    std::atomic<unsigned> head{0},tail{0};
    std::atomic<unsigned> consumers{0};
    float ll=0,rr=0,lr=0;
};

class SpectrumAnalyzer {
public:
    static constexpr int order=12,size=1<<order,bins=180;
    SpectrumAnalyzer(){pre.fill(-90);post.fill(-90);peak.fill(-90);}
    void update(AnalysisData& source,float smoothing){update(source,smoothing,[](float){});}
    template<typename Reaction>void update(AnalysisData& source,float smoothing,Reaction&& react){
        AnalysisSample s;bool frame=false;samplesRead=0;
        const float sr=std::max(8000.f,source.sampleRate.load());
        const float fast=1-std::exp(-1/(sr*.008f)),slow=1-std::exp(-1/(sr*.12f)),meter=1-std::exp(-1/(sr*.025f));
        const float bassFilter=1-std::exp(-2*juce::MathConstants<float>::pi*180/sr);
        while(samplesRead<16384&&source.pop(s)){
            ++samplesRead;const float power=s.post*s.post;
            fastPower+=fast*(power-fastPower);slowPower+=slow*(power-slowPower);meterPower+=meter*(power-meterPower);
            low1+=bassFilter*(s.post-low1);low2+=bassFilter*(low1-low2);const float bassPower=low2*low2;
            bassFastPower+=fast*(bassPower-bassFastPower);bassSlowPower+=slow*(bassPower-bassSlowPower);
            const float baseline=std::sqrt(std::max(0.f,slowPower));
            onset=std::max(onset,std::max(0.f,std::sqrt(std::max(0.f,fastPower))-baseline*1.12f-.005f));
            ring[0][(size_t)write]=s.pre;ring[1][(size_t)write]=s.post;write=(write+1)%size;
            waveRing[(size_t)waveWrite]=s.post;waveWrite=(waveWrite+1)%waveSize;
            if(++hop>=1024){hop=0;transform(sr,smoothing);frame=true;}
            if(++reactionHop>=128){reactionHop=0;rms=std::sqrt(std::max(0.f,meterPower));bassFast=std::sqrt(std::max(0.f,bassFastPower));bassSlow=std::sqrt(std::max(0.f,bassSlowPower));fastRms=std::sqrt(std::max(0.f,fastPower));captureWaveform(sr);react(128.f/sr);onset=0;}
        }
        rms=std::sqrt(std::max(0.f,meterPower));
        if(!frame){for(auto& v:post)v=std::max(-90.f,v-1.8f);for(auto& v:pre)v=std::max(-90.f,v-1.8f);}
        for(size_t i=0;i<peak.size();++i)peak[i]=std::max(post[i],peak[i]-.35f);
    }
    std::array<float,bins> pre{},post{},peak{};
    // Measured on the UI's existing FIFO consumer, never on the audio callback.
    std::array<float,bins> magnitude{};
    std::array<float,96> waveform{};
    float rms=0,onset=0,centroid=0,bassFast=0,bassSlow=0,fastRms=0;int samplesRead=0;
private:
    void captureWaveform(float sr){
        const int span=std::min(waveSize/4,(int)(sr*.016f));float start=(float)(waveSize-span-1);
        auto sample=[&](int i){return waveRing[(size_t)((waveWrite+i)%waveSize)];};
        // Trigger on the latest rising zero crossing that leaves a full window.
        // Interpolation keeps a sustained tone stable instead of aliasing at UI FPS.
        for(int i=waveSize-span-1;i>=std::max(1,waveSize-span-(int)(sr*.06f));--i){const float a=sample(i-1),b=sample(i);if(a<=0&&b>0){start=(float)(i-1)-a/(b-a);break;}}
        for(size_t i=0;i<waveform.size();++i){const float at=start+(float)i*(span-1)/(waveform.size()-1);const int k=(int)at;waveform[i]=juce::jmap(at-k,sample(k),sample(k+1));}
    }
    void transform(float sr,float smoothing){
        for(int c=0;c<2;++c){work.fill(0);for(int i=0;i<size;++i)work[(size_t)i]=ring[(size_t)c][(size_t)((write+i)%size)];
            window.multiplyWithWindowingTable(work.data(),size);fft.performFrequencyOnlyForwardTransform(work.data());
            auto& out=c==0?pre:post;
            for(int i=0;i<bins;++i){float f=20.f*std::pow(1000.f,(float)i/(bins-1));float pos=f*(float)size/sr;
                pos=juce::jlimit(0.f,(float)(size/2-2),pos);int k=(int)pos;float fraction=pos-(float)k;
                float amplitude=work[(size_t)k]*(1-fraction)+work[(size_t)k+1]*fraction;
                if(c==1)magnitude[(size_t)i]=amplitude*(4.f/size);
                float value=juce::Decibels::gainToDecibels(amplitude*(4.f/size),-90.f);
                float a=value>out[(size_t)i]?.65f:juce::jmap(smoothing,.35f,.08f);
                out[(size_t)i]+=a*(juce::jlimit(-90.f,6.f,value)-out[(size_t)i]);
            }
        }
        double total=0,weighted=0;for(int i=0;i<bins;++i){const double power=magnitude[(size_t)i]*magnitude[(size_t)i];total+=power;weighted+=power*i/(bins-1);}
        centroid=total>1.e-10?(float)(weighted/total):0;
    }
    juce::dsp::FFT fft{order};juce::dsp::WindowingFunction<float> window{size,juce::dsp::WindowingFunction<float>::hann,false};
    static constexpr int waveSize=16384; // >= 76 ms of waveform/trigger history at 192 kHz.
    std::array<float,waveSize> waveRing{};int waveWrite=0,reactionHop=0;
    std::array<std::array<float,size>,2> ring{};std::array<float,size*2> work{};int write=0,hop=0;float fastPower=0,slowPower=0,meterPower=0;
    float low1=0,low2=0,bassFastPower=0,bassSlowPower=0;
};
}
