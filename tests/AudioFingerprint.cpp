#include "DSP/AudioEngine.h"
#include <fstream>
#include <iostream>
#include <chrono>

// A matched-build comparison tool, not a compiler-specific golden unit test.
// Render before/after a performance refactor and compare the binary float files.
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"usage: AudioFingerprint output.f32\n";return 2;}
    std::ofstream file(argv[1],std::ios::binary);if(!file)return 3;
    const auto start=std::chrono::steady_clock::now();
    for(float rate:{48000.f,96000.f,384000.f,1536000.f}){
        cracked::AudioEngine engine;engine.prepare(rate);cracked::EngineSettings p;
        p.saturation.bands=6;p.saturation.drive=12;p.stereo.width=1.1f;
        for(int sample=0;sample<32000;++sample){
            p.input=sample<8000?-6.f:sample<16000?6.f:0.f;p.output=sample<12000?-3.f:0.f;
            p.modulation.amount=sample<10000?0.f:.4f;p.modulation.attack=sample<20000?10.f:35.f;p.modulation.release=sample<24000?160.f:80.f;
            p.modulation.wave=sample<16000?0:7;
            const float x=.3f*std::sin(2*cracked::pi*73*sample/rate)+.05f*std::sin(2*cracked::pi*1207*sample/rate);
            const auto y=engine.process({x,x*.93f},p,120);file.write(reinterpret_cast<const char*>(&y),sizeof y);
        }
    }
    std::cout<<"fingerprint render: "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<" ms\n";return file?0:4;
}
