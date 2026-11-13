#pragma once
#include <algorithm>
#include <cmath>
namespace cracked::ui {
struct EditorSize { int width,height; };
inline EditorSize fitFullEditorSize(int baseWidth,int workWidth,int workHeight,float desiredScale){
    const float scale=std::max(0.f,std::min({desiredScale,std::max(1,workWidth-32)/(float)baseWidth,std::max(1,workHeight-60)/840.f}));
    return {std::max(1,(int)std::lround(baseWidth*scale)),std::max(1,(int)std::lround(840*scale))};
}
}
