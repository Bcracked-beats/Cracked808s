#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace cracked {
constexpr float pi = 3.14159265358979323846f;
inline float clamp(float v,float lo,float hi){return std::clamp(v,lo,hi);}
inline float clean(float v){return std::isfinite(v)?clamp(v,-64.f,64.f):0.f;}
inline float dbGain(float d){return std::pow(10.f,d*.05f);}
inline float coefficient(float hz,float sr){return 1.f-std::exp(-2.f*pi*clamp(hz,1.f,sr*.45f)/sr);}
struct Stereo {float l=0,r=0;};
struct Lowpass {float z=0;float tick(float x,float a){z+=a*(x-z);return z;}void reset(){z=0;}};
struct DCBlock {float x=0,y=0;float tick(float v,float pole){float o=v-x+pole*y;x=v;y=o;return o;} };
struct Smooth {float v=0,a=.002f;void prepare(float sr,float milliseconds=15){a=1.f-std::exp(-1.f/(sr*milliseconds*.001f));}float tick(float target){v+=a*(target-v);return v;}void reset(float t){v=t;}};
struct AllpassDelay {
    std::vector<float> data;int pos=0,length=13;float feedback=.55f;
    float tick(float x){float d=data[(size_t)pos];float y=d-feedback*x;data[(size_t)pos]=x+feedback*y;if(++pos>=length)pos=0;return y;}
    void prepare(int n){length=std::max(n,1);data.resize((size_t)length);reset();}
    void reset(){std::fill(data.begin(),data.end(),0.f);pos=0;}
};
}
