#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/SaturationEngine.h"

namespace cracked {
inline juce::AudioProcessorValueTreeState::ParameterLayout makeLayout(){
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto f=[&](juce::String id,juce::String name,float lo,float hi,float def,float step=.001f,float skew=1.f){layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,step,skew),def));};
    auto b=[&](juce::String id,juce::String name,bool def){layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},name,def));};
    auto choice=[&](juce::String id,juce::String name,juce::StringArray names,int def=0){layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,names,def));};
    f("drive","Drive",0,36,9,.1f);f("color","Tone",-1,1,0,.01f);f("mix","Saturation Mix",0,1,1,.01f);
    f("width","Width",0,2,.5f,.01f);f("input","Input",-36,36,0,.1f);f("output","Output",-36,36,-1,.1f);
    f("monoBass","Low Bypass Frequency",20,1000,110,1,.4f);f("glow","Visual Intensity",0,1,.8f,.01f);
    b("lowEnabled","Low Bypass",true);b("monoSafe","Mono Safe",true);b("monoCheck","Mono Check",false);b("monoSub","Mono Sub",false);b("synthetic","Synthetic Side",true);
    b("bypass","Bypass",false);b("protect","Output Protection",true);b("compensate","Auto Gain",true);b("preserveSub","Preserve Sub",false);
    f("stereoMix","Widening Mix",0,1,1);f("focus","Stereo Focus",0,1,.5f);f("balance","Stereo Balance",-1,1,0);f("globalMix","Plugin Mix",0,1,1);
    b("widthCompensate","Width Level Match",true);
    choice("routing","Routing",{"Saturation > Width","Width > Saturation"});choice("quality","Quality",{"Eco 1x","Normal 2x","Good 8x","Superb 32x"},1);
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"bands",1},"Band Count",1,6,1));
    constexpr float cutoffs[]={100,350,1200,4000,9000};for(int i=0;i<5;++i)f("crossover"+juce::String(i),"Crossover "+juce::String(i+1),20,20000,cutoffs[i],1,.3f);
    juce::StringArray models;for(auto* n:modelNames)models.add(n);
    for(int i=0;i<6;++i){auto prefix="b"+juce::String(i)+"_";auto name="Band "+juce::String(i+1)+" ";
        choice(prefix+"model",name+"Model",models);f(prefix+"drive",name+"Drive",-24,36,0,.1f);f(prefix+"tone",name+"Tone",-1,1,0);f(prefix+"mix",name+"Mix",0,1,1);f(prefix+"level",name+"Level",-36,24,0,.1f);
        f(prefix+"dynamics",name+"Dynamics",-1,1,0);f(prefix+"feedback",name+"Feedback",0,.9f,0);f(prefix+"feedbackHz",name+"Feedback Frequency",30,12000,600,1,.35f);
        for(auto t:{"bass","mid","treble","presence"})f(prefix+t,name+juce::String(t),-12,12,0,.1f);
        f(prefix+"driveBalance",name+"Drive Balance",-1,1,0);f(prefix+"levelBalance",name+"Level Balance",-1,1,0);
        b(prefix+"mute",name+"Mute",false);b(prefix+"solo",name+"Solo",false);b(prefix+"bypass",name+"Bypass",false);b(prefix+"midSide",name+"Mid Side",false);
    }
    choice("modWave","Modulation Source",{"Sine LFO","Triangle LFO","Saw LFO","Square LFO","Random","Smooth Random","16 Step","Envelope Follower"});
    choice("modDest","Modulation Destination",{"Drive","Width","Tone","Saturation Mix"});f("modRate","LFO Rate",.02f,20,1,.01f,.4f);f("modAmount","Modulation Amount",-1,1,0);f("modPhase","LFO Phase",0,1,0);
    b("modSync","Tempo Sync",false);b("modBipolar","Bipolar",true);choice("modDivision","Sync Division",{"4 bars","2 bars","1 beat","1/8 note","1/16 note"},2);
    f("modAttack","Follower Attack",.1f,200,10,.1f,.4f);f("modRelease","Follower Release",5,2000,160,1,.4f);f("modSensitivity","Follower Sensitivity",.1f,10,1,.01f,.4f);
    for(int i=0;i<16;++i)f("step"+juce::String(i),"Step "+juce::String(i+1),-1,1,i%4==2?1.f:0.f,.01f);
    b("reducedMotion","Reduced Motion",false);b("showPre","Pre Spectrum",true);f("smoothing","Spectrum Smoothing",0,1,.6f);choice("fps","Animation Rate",{"30 FPS","60 FPS"});
    choice("visualMode","Visualizer",{"Spectrum","Particle Sphere","Image"});f("visualAmount","Visual Reaction",0,1,.65f,.01f);
    f("visualSensitivity","Visual Sensitivity",.25f,4,1.5f,.01f,.6f);f("visualAttack","Visual Attack",5,200,28,1,.5f);f("visualRelease","Visual Release",40,1500,260,1,.5f);
    b("visualMotion","Visual Motion",true);f("visualMotionDepth","Visual Motion Depth",0,1,.55f,.01f);f("visualSpeed","Visual Flow Speed",.05f,1.5f,.35f,.01f);
    f("visualGlow","Visual Glow Boost",0,2,1.15f,.01f);b("visualAudioGlow","Audio Glow Boost",true);
    choice("visualDensity","Particle Density",{"Airy","Full","Dense"},1);f("visualPointSize","Particle Size",.4f,1.8f,.95f,.05f);f("visualScale","Visual Scale",.6f,1.15f,1,.01f);
    b("visualColor","Color Swaps",false);choice("visualPalette","Visual Palette",{"Aurora","Prism","Ice","Ember"});f("visualColorSpeed","Color Flow Speed",0,2,.3f,.01f);
    choice("visualColorMode","Color Behavior",{"Palette Flow","Random Bass Hits","Full Spectrum","Fixed Hue"});
    f("visualBassSensitivity","Bass Hit Sensitivity",.25f,4,1,.01f);f("visualHitGap","Minimum Hit Gap",120,1200,240,10);
    f("visualColorTransition","Color Transition",0,600,110,5);f("visualHue","Fixed Hue",0,1,.57f,.001f);
    f("visualSaturation","Color Saturation",0,1,.75f,.01f);f("visualColorSpread","Hue Spread",0,1,.3f,.01f);
    return layout;
}
}
