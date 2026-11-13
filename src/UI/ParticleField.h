#pragma once
#include "SpaceTheme.h"

namespace cracked::ui {
inline juce::Colour reactiveColour(float position,int palette){
    position-=std::floor(position);const float at=position*4;const int i=(int)at;const float mix=at-i;
    constexpr juce::uint32 colors[4][4]={
        {0xfff07bce,0xffe8e975,0xff65f4a0,0xff45d6c8},
        {0xffba8cff,0xff69b8ff,0xff6cf3b2,0xffffa774},
        {0xffb6d8ff,0xff62b8fa,0xff55eadc,0xffd3f5ff},
        {0xffff9270,0xffffcf80,0xfff86c7f,0xffbc7be8}};
    palette=juce::jlimit(0,3,palette);return juce::Colour(colors[palette][i%4]).interpolatedWith(juce::Colour(colors[palette][(i+1)%4]),mix);
}
inline juce::Colour behaviorColour(float position,int palette,bool fullSpectrum,float saturation){
    position-=std::floor(position);if(fullSpectrum)return juce::Colour::fromHSV(position,saturation,.96f,1.f);
    const auto color=reactiveColour(position,palette);return color.withSaturation(juce::jlimit(0.f,1.f,color.getSaturation()*saturation/.75f));
}
struct ParticleStyle {
    float level=0,hit=0,detail=.35f,amount=.65f,motionDepth=.55f,colorPhase=0,glow=1.15f,size=.95f,scale=1;
    std::array<float,48> profile{};std::array<float,96> waveform{};
    float colorSpread=1,saturation=.75f;int density=1,palette=0;bool motion=true,audioGlow=true,colors=false,fullColor=false;
};

class ParticleField {
public:
    void paint(juce::Graphics& g,juce::Rectangle<float> area,const ParticleStyle& style){
        prepare(style.density);for(auto& p:dots)p.clear();for(auto& p:halos)p.clear();
        const auto centre=area.getCentre();const float react=style.level*style.amount;
        const float motion=style.motion?style.amount*style.motionDepth:0;
        const float radius=std::min(area.getHeight()*.30f,74*style.scale);
        const float glow=style.glow*(style.audioGlow?.35f+react*1.6f+style.hit*style.amount*1.8f:.8f);
        auto colour=[&](float position){return behaviorColour(position,style.palette,style.fullColor,style.saturation);};
        const auto haloColor=style.colors?colour(style.colorPhase):silver();
        g.setGradientFill(juce::ColourGradient(haloColor.withAlpha(juce::jlimit(0.f,.25f,glow*.065f)),centre.x,centre.y,juce::Colours::transparentWhite,centre.x+radius+34,centre.y,true));g.fillEllipse(juce::Rectangle<float>((radius+34)*2,(radius+34)*2).withCentre(centre));
        auto lookup=[](const auto& values,float position){const float at=juce::jlimit(0.f,1.f,position)*(values.size()-1);const auto k=(size_t)at;return juce::jmap(at-(float)k,values[k],values[std::min(k+1,values.size()-1)]);};
        size_t index=0;
        for(const auto& p:particles){
            // Latitude maps bass -> treble; longitude maps a zero-crossing-triggered
            // waveform. Every displacement is a current audio measurement, not elapsed time.
            const float band=lookup(style.profile,p.band),wave=lookup(style.waveform,p.wave);
            const float detail=juce::jlimit(0.f,1.f,style.detail/1.5f);
            const float warp=juce::jlimit(.65f,1.42f,1+motion*(style.level*.48f+style.hit*.55f+band*.6f+(wave*p.ripple)*(.16f+detail*.2f)));
            const float x=p.x*warp,z=p.z*warp,y=p.y*warp;
            const float yy=y*.984f-z*.178f,zz=z*.984f+y*.178f,perspective=3.6f/(3.6f+zz),depth=juce::jlimit(0.f,1.f,.5f-zz*.5f);
            const float px=centre.x+x*radius*perspective,py=centre.y+yy*radius*perspective;
            const float diameter=style.size*(.65f+depth*.65f)*perspective;
            float hue=(.28f+p.x*.34f+p.y*.12f)*style.colorSpread+style.colorPhase;hue-=std::floor(hue);const int color=style.colors?std::min(15,(int)(hue*16)):0;
            const int bucket=std::min(3,(int)(depth*4));dots[(size_t)(color*4+bucket)].addEllipse(px-diameter*.5f,py-diameter*.5f,diameter,diameter);
            if((index++%23)==0&&depth>.4f&&glow>.01f)halos[(size_t)color].addEllipse(px-diameter*1.8f,py-diameter*1.8f,diameter*3.6f,diameter*3.6f);
        }
        for(int color=0;color<(style.colors?16:1);++color){const auto tint=style.colors?colour(((float)color+.5f)/16):silver();
            g.setColour(tint.withAlpha(juce::jlimit(0.f,.16f,glow*.055f)));g.fillPath(halos[(size_t)color]);
            for(int depth=0;depth<4;++depth){constexpr float opacity[]={.1f,.22f,.43f,.72f};g.setColour(tint.withAlpha(juce::jlimit(0.f,.96f,opacity[depth]*(.85f+glow*.22f))));g.fillPath(dots[(size_t)(color*4+depth)]);}
        }
    }
private:
    struct Particle{float x,y,z,band,wave,ripple;};
    void prepare(int density){
        density=juce::jlimit(0,2,density);if(density==lastDensity)return;lastDensity=density;particles.clear();
        constexpr int rows[]={40,56,72},columns[]={104,160,192};
        for(int row=0;row<rows[density];++row){const float latitude=juce::MathConstants<float>::pi*((float)(row+.5f)/rows[density]-.5f),cl=std::cos(latitude),sl=std::sin(latitude);const int count=std::max(8,juce::roundToInt(columns[density]*cl));
            for(int col=0;col<count;++col){const float hue=((float)col+(row%2)*.5f)/count,longitude=juce::MathConstants<float>::twoPi*hue;
                particles.push_back({cl*std::cos(longitude),sl,cl*std::sin(longitude),(sl+1)*.5f,hue,std::sin(3*longitude+2*latitude)});}
        }
    }
    int lastDensity=-1;std::vector<Particle> particles;
    std::array<juce::Path,64> dots;std::array<juce::Path,16> halos;
};
}
