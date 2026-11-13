#pragma once
#include <algorithm>
#include <cmath>

namespace cracked::ui {
inline int meterSegments(float amplitude){
    const float db=amplitude>0?20.f*std::log10(amplitude):-72.f;
    return (int)std::ceil(std::clamp((db+60.f)/60.f,0.f,1.f)*45.f);
}
inline bool meterVisualChanged(float previous,float current,int previousClip,int currentClip){
    return meterSegments(previous)!=meterSegments(current)||(previousClip>0)!=(currentClip>0);
}
}
