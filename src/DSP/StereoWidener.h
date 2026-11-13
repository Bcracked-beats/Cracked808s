#pragma once
#include "Common.h"
namespace cracked {
struct StereoSettings {
    float width=0,lowBypass=150,focus=.5f,mix=1,balance=0;
    bool lowEnabled=true,monoSafe=true,monoSub=false,synthetic=true;
};
// Topology-preserving state-variable sections retain meaningful integrator states
// during cutoff changes. Double precision also supports low cutoffs at 32x.
struct SideHighpass {
    double g=0,damping=1,h=1,s1=0,s2=0;
    void configure(float hz,float rate,double q){
        g=std::tan(3.141592653589793*clamp(hz,20.f,rate*.4f)/rate);damping=1/q;h=1/(1+damping*g+g*g);
    }
    float tick(float x){const double high=h*(x-(g+damping)*s1-s2),band=high*g+s1;s1=high*g+band;const double low=band*g+s2;s2=band*g+low;return (float)high;}
    void reset(){s1=s2=0;}
};
class StereoWidener {
public:
    void prepare(float rate){
        sr=rate;
        constexpr float delaysA[]={.000083f,.000293f,.000731f},delaysB[]={.000137f,.000487f,.001117f};
        for(size_t i=0;i<3;++i){left[i].prepare((int)(rate*delaysA[i]));right[i].prepare((int)(rate*delaysB[i]));}
        powerAlpha=(float)-std::expm1(-1./(rate*.09));gainAlpha=(float)-std::expm1(-1./(rate*.06));
        cutoffAlpha=-std::expm1(-32./(rate*.025));
        fastGainAlpha=(float)-std::expm1(-1./(rate*.005));
        centerAlpha=(float)-std::expm1(-1./(rate*.008));
        covarianceAlpha=-std::expm1(-1./(rate*.02));
        smooth.prepare(rate);reset();
    }
    void reset(){
        for(auto& a:left)a.reset();for(auto& a:right)a.reset();
        for(auto* filters:{&generatedHP,&referenceHP,&existingHP})for(auto& f:*filters)f.reset();
        subHP.reset();subGenerated.reset();subReference.reset();focusLow={};referenceLow={};smooth.reset(0);lastCut=lastFocus=-1;lastFourth=false;
        generatedPower=referencePower=midPower=covGenerated=covReference=0;sideGain=1;weightGenerated=1;weightReference=0;
        filterCounter=0;filterReady=false;retuneRemaining=0;lastProtection=-1;centerReady=false;
    }
    Stereo process(Stereo x,const StereoSettings& p){
        const float m=.5f*(x.l+x.r);float s=.5f*(x.l-x.r);
        const bool fourth=p.lowEnabled&&p.monoSafe;
        if((filterCounter++&31)==0){
            const double target=std::log(clamp(p.lowBypass,20.f,1000.f));
            const int protection=(p.lowEnabled?1:0)|(p.monoSafe?2:0)|(p.monoSub?4:0);
            if(filterReady&&((protection!=0&&std::abs(target-cutoffLog)>.025)||(lastProtection!=protection)))retuneRemaining=(int)(sr*.08f);
            lastProtection=protection;
            if(!filterReady){cutoffLog=target;filterReady=true;}else cutoffLog+=cutoffAlpha*(target-cutoffLog);
            const float cut=(float)std::exp(cutoffLog);
            if(cut!=lastCut||fourth!=lastFourth){
                lastCut=cut;lastFourth=fourth;
                for(auto* filters:{&generatedHP,&referenceHP,&existingHP}){
                    (*filters)[0].configure(lastCut,sr,fourth?.5411961001:.7071067812);
                    (*filters)[1].configure(lastCut,sr,1.306562965);
                }
                subHP.configure(lastCut,sr,.7071067812);
                subGenerated.configure(lastCut,sr,.7071067812);subReference.configure(lastCut,sr,.7071067812);
            }
        }
        if(p.focus!=lastFocus){lastFocus=p.focus;focusA=coefficient(1800.f+16000.f*p.focus*p.focus,sr);}
        float a=m,b=m;for(size_t i=0;i<3;++i){a=left[i].tick(a);b=right[i].tick(b);}
        // Matched direct terms cancel; short all-pass paths avoid a delayed
        // mono subtraction that panned attacks and smeared the 808 decay.
        float generated=.9f*(a-b),reference=m,eligible=s;
        for(size_t i=0;i<2;++i){
            const float g=generatedHP[i].tick(generated),r=referenceHP[i].tick(reference),e=existingHP[i].tick(eligible);
            if((p.lowEnabled||p.monoSafe)&&(i==0||fourth)){generated=g;reference=r;eligible=e;}
        }
        generated=focusLow.tick(generated,focusA);reference=referenceLow.tick(reference,focusA);
        const float w=smooth.tick(p.width)*p.mix;
        float existing=s+w*.5f*eligible;
        const float subExisting=subHP.tick(existing),subG=subGenerated.tick(generated),subR=subReference.tick(reference);
        if(p.monoSub){existing=subExisting;generated=subG;reference=subR;}
        // A complementary side can still pan if it contains an in-phase copy of M.
        // Rotate two already-protected candidates into the null of their mid
        // covariance. This corrects only synthetic side, never the source pan.
        midPower+=covarianceAlpha*((double)m*m-midPower);
        const float detectionGain=std::max(.1f,w*sideGain);
        covGenerated+=covarianceAlpha*((double)m*generated*detectionGain-covGenerated);
        covReference+=covarianceAlpha*((double)m*reference*detectionGain-covReference);
        const double norm=std::hypot(covGenerated,covReference);
        if(norm>std::max(1e-12,midPower*1e-5)){
            const float wg=(float)(covReference/norm),wr=(float)(-covGenerated/norm);
            if(!centerReady&&midPower>1e-9){weightGenerated=wg;weightReference=wr;centerReady=true;}
            else {weightGenerated+=centerAlpha*(wg-weightGenerated);weightReference+=centerAlpha*(wr-weightReference);}
        }
        const float balanced=weightGenerated*generated+weightReference*reference;
        // Match only eligible upper-band energy: never boost the protected sub to fake width.
        generatedPower+=powerAlpha*((double)balanced*balanced-generatedPower);
        referencePower+=powerAlpha*((double)reference*reference-referencePower);
        // A changed filter invalidates the previously learned gain/phase relation.
        // Briefly restrict makeup while those statistics settle; don't amplify
        // the disappearing candidate during a large cutoff or guard transition.
        const float cap=retuneRemaining>0?1.6f:2.8f;if(retuneRemaining>0)--retuneRemaining;
        const float target=generatedPower>1e-12?clamp((float)(1.25*std::sqrt(referencePower/generatedPower)),.4f,cap):1.f;
        sideGain+=(target<sideGain?fastGainAlpha:gainAlpha)*(target-sideGain);
        s=existing+(p.synthetic?w*sideGain*balanced:0.f);
        return {m+s,m-s}; // Added side cancels exactly in (L+R)/2.
    }
private:
    float sr=48000,lastCut=-1,lastFocus=-1,focusA=.3f,powerAlpha=.001f,gainAlpha=.001f,sideGain=1;
    bool lastFourth=false;
    bool filterReady=false,centerReady=false;unsigned filterCounter=0;int retuneRemaining=0,lastProtection=-1;float fastGainAlpha=.004f,centerAlpha=.002f;
    double cutoffLog=0,cutoffAlpha=.01,covarianceAlpha=.0007;
    double generatedPower=0,referencePower=0,midPower=0,covGenerated=0,covReference=0;
    float weightGenerated=1,weightReference=0;
    Smooth smooth;
    std::array<AllpassDelay,3> left,right;
    std::array<SideHighpass,2> generatedHP,referenceHP,existingHP;
    SideHighpass subHP,subGenerated,subReference;
    Lowpass focusLow,referenceLow;
};
}
