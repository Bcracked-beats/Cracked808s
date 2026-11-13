#pragma once
#include "../DSP/AudioEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>
namespace cracked {
class ParameterReader {
public:
    explicit ParameterReader(juce::AudioProcessorValueTreeState& s){
        const char* mainIds[]={"drive","color","mix","width","input","output","monoBass","lowEnabled","monoSafe","monoCheck","monoSub","synthetic","bypass","protect","compensate","preserveSub","stereoMix","focus","balance","globalMix","routing","bands","modWave","modDest","modRate","modAmount","modPhase","modSync","modBipolar","modDivision","modAttack","modRelease","modSensitivity","quality"};
        for(size_t i=0;i<main.size();++i)main[i]=s.getRawParameterValue(mainIds[i]);
        widthCompensate=s.getRawParameterValue("widthCompensate");
        const char* bandIds[]={"drive","tone","mix","level","dynamics","feedback","feedbackHz","bass","mid","treble","presence","driveBalance","levelBalance","model","mute","solo","bypass","midSide"};
        for(int i=0;i<6;++i)for(size_t j=0;j<band[0].size();++j)band[(size_t)i][j]=s.getRawParameterValue("b"+juce::String(i)+"_"+bandIds[j]);
        for(int i=0;i<5;++i)cut[(size_t)i]=s.getRawParameterValue("crossover"+juce::String(i));
        for(int i=0;i<16;++i)steps[(size_t)i]=s.getRawParameterValue("step"+juce::String(i));
    }
    EngineSettings read() const noexcept {
        auto v=[&](size_t i){return main[i]->load(std::memory_order_relaxed);};EngineSettings s;
        s.widthCompensate=widthCompensate->load(std::memory_order_relaxed)>.5f;
        s.saturation.drive=v(0);s.saturation.tone=v(1);s.saturation.mix=v(2);s.stereo.width=v(3);s.input=v(4);s.output=v(5);s.stereo.lowBypass=v(6);s.stereo.lowEnabled=v(7)>.5f;s.stereo.monoSafe=v(8)>.5f;s.monoCheck=v(9)>.5f;s.stereo.monoSub=v(10)>.5f;s.stereo.synthetic=v(11)>.5f;s.bypass=v(12)>.5f;s.protect=v(13)>.5f;s.saturation.compensation=v(14)>.5f;s.saturation.preserveSub=v(15)>.5f;s.stereo.mix=v(16);s.stereo.focus=v(17);s.stereo.balance=v(18);s.globalMix=v(19);s.widthFirst=v(20)>.5f;s.saturation.bands=(int)v(21);
        s.modulation.wave=(int)v(22);s.modulation.destination=(int)v(23);s.modulation.rate=v(24);s.modulation.amount=v(25);s.modulation.phase=v(26);s.modulation.sync=v(27)>.5f;s.modulation.bipolar=v(28)>.5f;s.modulation.division=(int)v(29);s.modulation.attack=v(30);s.modulation.release=v(31);s.modulation.sensitivity=v(32);
        float last=20;for(size_t i=0;i<5;++i){s.saturation.crossovers[i]=std::max(last+1,cut[i]->load());last=s.saturation.crossovers[i];}
        for(size_t i=0;i<16;++i)s.modulation.steps[i]=steps[i]->load();
        for(size_t i=0;i<6;++i){auto load=[&](size_t j){return band[i][j]->load(std::memory_order_relaxed);};auto& b=s.saturation.band[i];b.drive=load(0);b.tone=load(1);b.mix=load(2);b.level=load(3);b.dynamics=load(4);b.feedback=load(5);b.feedbackHz=load(6);b.bass=load(7);b.mid=load(8);b.treble=load(9);b.presence=load(10);b.driveBalance=load(11);b.levelBalance=load(12);b.model=(int)load(13);b.mute=load(14)>.5f;b.solo=load(15)>.5f;b.bypass=load(16)>.5f;b.midSide=load(17)>.5f;}
        return s;
    }
    int quality()const{return std::clamp((int)main[33]->load(),0,3);}
private:
    std::array<std::atomic<float>*,34> main{};std::array<std::array<std::atomic<float>*,18>,6> band{};
    std::array<std::atomic<float>*,5> cut{};std::array<std::atomic<float>*,16> steps{};
    std::atomic<float>* widthCompensate=nullptr;
};
}
