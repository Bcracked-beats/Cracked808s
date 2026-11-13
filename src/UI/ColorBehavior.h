#pragma once
#include <cmath>
#include <algorithm>
#include <random>

namespace cracked::ui {
struct ColorSettings {
    int mode=0;bool enabled=false,frozen=false;
    float sensitivity=1,gapMs=240,transitionMs=110,hue=.57f;
};
class ColorBehavior {
public:
    bool advance(float dt,float bassFast,float bassSlow,float fullRms,const ColorSettings& s,float measuredHue){
        remaining=std::max(0.f,remaining-dt);
        if(bassFast<bassSlow*1.06f+.001f||bassFast<.008f/s.sensitivity)armed=true;
        const bool attack=armed&&remaining<=0&&bassFast>.015f/s.sensitivity&&bassFast>bassSlow*(1.08f+.24f/s.sensitivity)&&bassFast>fullRms*.44f;
        if(attack){armed=false;remaining=s.gapMs*.001f;}
        if(s.frozen||!s.enabled)return false;
        if(lastMode!=s.mode){lastMode=s.mode;current=target=wrap(s.mode==1||s.mode==3?s.hue:measuredHue);}
        if(s.mode==1&&attack){
            float next=(float)(random()-random.min())/(float)(random.max()-random.min());
            if(std::abs(distance(next,current))<.15f)next=wrap(current+.2f+next*.5f);
            target=next;
        }else if(s.mode==0){current=target=wrap(measuredHue);}
        else if(s.mode==2)target=wrap(measuredHue);
        else if(s.mode==3)target=wrap(s.hue);
        const float coefficient=s.transitionMs<1?1.f:1-std::exp(-dt/(s.transitionMs*.001f));
        current=wrap(current+coefficient*distance(target,current));
        return s.mode==1&&attack;
    }
    float hue()const{return current;}
private:
    static float wrap(float hue){return hue-std::floor(hue);}
    static float distance(float a,float b){float d=a-b;return d-std::floor(d+.5f);}
    std::minstd_rand random{808};float current=0,target=0,remaining=0;bool armed=true;int lastMode=-1;
};
}
