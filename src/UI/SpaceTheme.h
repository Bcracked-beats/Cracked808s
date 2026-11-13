#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <CrackedAssets.h>
namespace cracked::ui {
inline juce::Colour silver(){return juce::Colours::white;}
inline juce::Typeface::Ptr brandTypeface(){static auto face=juce::Typeface::createSystemTypefaceFor(CrackedAssets::SpaceGrotesk_ttf,CrackedAssets::SpaceGrotesk_ttfSize);return face;}
inline juce::Font font(float size,float spacing=.08f){juce::Font f{juce::FontOptions(brandTypeface()).withPointHeight(size)};f.setExtraKerningFactor(spacing);return f;}
inline void text(juce::Graphics& g,const juce::String& s,juce::Rectangle<float> r,float size=11,float alpha=.7f,float spacing=.18f){g.setColour(silver().withAlpha(alpha));g.setFont(font(size,spacing));g.drawText(s,r,juce::Justification::centred);}
inline juce::Image alphaFitted(juce::Image source){
        if(!source.isValid())return source;
        int left=source.getWidth(),top=source.getHeight(),right=0,bottom=0;
        juce::Image::BitmapData pixels(source,juce::Image::BitmapData::readOnly);
        for(int y=0;y<source.getHeight();++y)for(int x=0;x<source.getWidth();++x)if(pixels.getPixelColour(x,y).getAlpha()>12){left=std::min(left,x);top=std::min(top,y);right=std::max(right,x);bottom=std::max(bottom,y);}
        return right>=left&&bottom>=top?source.getClippedImage(juce::Rectangle<int>(left,top,right-left+1,bottom-top+1)):source;
}
inline juce::Image filteredImage(juce::Image source,int targetWidth){
    // Repeated half-size filtering integrates fine alpha detail instead of
    // sampling a few source pixels from a 2000px image at a 100px destination.
    while(source.isValid()&&source.getWidth()>targetWidth*2)source=source.rescaled((source.getWidth()+1)/2,(source.getHeight()+1)/2,juce::Graphics::highResamplingQuality);
    if(source.isValid()&&source.getWidth()>targetWidth)source=source.rescaled(targetWidth,std::max(1,juce::roundToInt((float)source.getHeight()*targetWidth/source.getWidth())),juce::Graphics::highResamplingQuality);
    return source;
}
struct BrandImages {
    juce::Image original=alphaFitted(juce::ImageFileFormat::loadFrom(CrackedAssets::CrackedVaultLogo_png,CrackedAssets::CrackedVaultLogo_pngSize));
    std::array<juce::Image,3> marks={filteredImage(original,128),filteredImage(original,256),filteredImage(original,512)};
};
inline juce::Image brandLogo(){juce::SharedResourcePointer<BrandImages> images;return images->original;}
inline void logo(juce::Graphics& g,juce::Rectangle<float> r){
    juce::SharedResourcePointer<BrandImages> images;
    const float physicalWidth=r.getWidth()*g.getInternalContext().getPhysicalPixelScaleFactor();
    juce::Graphics::ScopedSaveState save(g);g.setOpacity(1);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImage(images->marks[physicalWidth<=128?0:physicalWidth<=256?1:2],r,juce::RectanglePlacement::centred);
}

class SpaceTheme : public juce::LookAndFeel_V4 {
    // Release GPU-backed images with the last editor, never inside DLL teardown.
    juce::SharedResourcePointer<BrandImages> brandImages;
public:
    SpaceTheme(){setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff050505));setColour(juce::PopupMenu::textColourId,silver());setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff242424));setColour(juce::ComboBox::textColourId,silver());setColour(juce::TextButton::textColourOffId,silver());setColour(juce::TextButton::textColourOnId,juce::Colours::white);setColour(juce::Slider::textBoxTextColourId,silver());setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);setColour(juce::Slider::thumbColourId,silver());setColour(juce::Slider::trackColourId,silver().withAlpha(.6f));setColour(juce::Slider::backgroundColourId,juce::Colour(0xff1c1c1c));setColour(juce::TextEditor::textColourId,silver());setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff080808));setColour(juce::TextEditor::highlightColourId,juce::Colour(0xff383838));setColour(juce::TextEditor::focusedOutlineColourId,silver().withAlpha(.65f));}
    juce::Label* createSliderTextBox(juce::Slider&)override{auto* label=new juce::Label();label->setFont(font(10,.02f));label->setJustificationType(juce::Justification::centred);label->setColour(juce::Label::textColourId,silver().withAlpha(.8f));label->setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);label->setColour(juce::TextEditor::textColourId,silver());label->setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff080808));return label;}
    juce::Font getTextButtonFont(juce::TextButton&,int)override{return font(10,.11f);}
    juce::Font getComboBoxFont(juce::ComboBox&)override{return font(11,.04f);}
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down)override{
        auto r=b.getLocalBounds().toFloat().reduced(.5f);g.setColour(juce::Colour(over||down?0xff1b1b1b:0xff080808).withAlpha(b.isEnabled()?1.f:.4f));g.fillRoundedRectangle(r,5);
        g.setColour(silver().withAlpha(b.getToggleState()?.55f:over?.35f:.15f));g.drawRoundedRectangle(r,5,.7f);
    }
    void drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox&)override{
        g.setColour(juce::Colour(0xff050505));g.fillRoundedRectangle(.5f,.5f,(float)w-1,(float)h-1,5);g.setColour(silver().withAlpha(.19f));g.drawRoundedRectangle(.5f,.5f,(float)w-1,(float)h-1,5,.7f);juce::Path p;p.startNewSubPath((float)w-17,(float)h*.44f);p.lineTo((float)w-13,(float)h*.57f);p.lineTo((float)w-9,(float)h*.44f);g.setColour(silver().withAlpha(.65f));g.strokePath(p,juce::PathStrokeType(1));
    }
    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool over,bool)override{
        auto r=juce::Rectangle<float>(3,(float)b.getHeight()*.5f-9,34,18);bool on=b.getToggleState();g.setColour(juce::Colour(0xff040404));g.fillRoundedRectangle(r,9);g.setColour(silver().withAlpha(on?.65f:over?.45f:.3f));g.drawRoundedRectangle(r,9,.8f);
        auto dot=r.withSizeKeepingCentre(12,12).translated(on?7.f:-7.f,0);if(on){g.setColour(juce::Colours::white.withAlpha(.06f));g.fillEllipse(dot.expanded(5));}g.setColour(silver().withAlpha(on?1.f:.4f));g.fillEllipse(dot);
        if(b.getButtonText().isNotEmpty()){g.setFont(font(10,.05f));g.setColour(silver().withAlpha(.7f));g.drawText(b.getButtonText(),43,0,b.getWidth()-44,b.getHeight(),juce::Justification::centredLeft);}
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider& slider)override{
        float diameter=(float)std::min(width,height)-44;auto centre=juce::Point<float>((float)x+width*.5f,(float)y+height*.5f);auto r=juce::Rectangle<float>(diameter,diameter).withCentre(centre);float radius=diameter*.5f;
        const float activity=(float)slider.getProperties().getWithDefault("activity",0.f);
        const float preference=(float)slider.getProperties().getWithDefault("glow",.65f);
        const bool bipolar=(bool)slider.getProperties().getWithDefault("bipolar",false);
        const float amount=bipolar?std::abs(position*2-1):position;
        const float glow=juce::jlimit(0.f,1.f,preference*(.08f+.92f*std::pow(amount,.75f)+activity*.12f));
        // Broad, static, value-driven bloom remains visible when the transport is stopped.
        g.setGradientFill(juce::ColourGradient(silver().withAlpha(glow*.24f),centre.x,centre.y,juce::Colours::transparentWhite,centre.x+radius+23,centre.y,true));g.fillEllipse(r.expanded(23));
        constexpr float haloWidths[]={42,34,26,19,13,8,4};constexpr float haloAlphas[]={.012f,.018f,.026f,.04f,.07f,.12f,.24f};
        for(size_t i=0;i<7;++i){g.setColour(silver().withAlpha(glow*haloAlphas[i]));g.drawEllipse(r,haloWidths[i]);}
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff151515),r.getX(),r.getY(),juce::Colour(0xff010101),r.getRight(),r.getBottom(),false));g.fillEllipse(r.reduced(5));
        g.setColour(silver().withAlpha(.25f));g.drawEllipse(r.reduced(5),.8f);g.setColour(juce::Colours::black.withAlpha(.85f));g.drawEllipse(r.reduced(9),1);
        juce::Path track;track.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);g.setColour(silver().withAlpha(.24f));g.strokePath(track,juce::PathStrokeType(1));
        const float angle=start+position*(end-start);juce::Path arc;arc.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);
        for(int j=4;j>=1;--j){g.setColour(juce::Colours::white.withAlpha(glow*.025f*(5-j)));g.strokePath(arc,juce::PathStrokeType((float)j*3));}
        g.setColour(silver().withAlpha(.96f));g.strokePath(arc,juce::PathStrokeType(1.65f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        auto a=centre+juce::Point<float>(std::sin(angle),-std::cos(angle))*(radius-12);auto b=centre+juce::Point<float>(std::sin(angle),-std::cos(angle))*(radius-33);
        g.setColour(juce::Colours::white.withAlpha(.06f+glow*.1f));g.drawLine(a.x,a.y,b.x,b.y,7);g.setColour(juce::Colours::white.withAlpha(.95f));g.drawLine(a.x,a.y,b.x,b.y,2.4f);
        for(int i=0;i<11;++i){float theta=start+(end-start)*(float)i/10;auto p=centre+juce::Point<float>(std::sin(theta),-std::cos(theta))*(radius+9);g.setColour(silver().withAlpha(.28f));g.fillEllipse(p.x-.7f,p.y-.7f,1.4f,1.4f);}
    }
};

class ValueKnob : public juce::Slider {
public:
    ValueKnob(){setSliderStyle(RotaryVerticalDrag);setTextBoxStyle(TextBoxBelow,false,94,20);setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);setMouseDragSensitivity(240);}
    void mouseDoubleClick(const juce::MouseEvent&)override{showTextBox();}
    void mouseDown(const juce::MouseEvent& e)override{if(e.mods.isPopupMenu()){juce::PopupMenu m;m.addItem(1,"Reset to default");m.addItem(2,"Enter value");m.showMenuAsync({},[safe=juce::Component::SafePointer<ValueKnob>(this)](int result){if(!safe)return;if(result==1)safe->setValue(safe->defaultValue,juce::sendNotificationSync);if(result==2)safe->showTextBox();});return;}juce::Slider::mouseDown(e);}
    juce::String getTextFromValue(double v)override{if(format==0||format>=4)return juce::String(v,1)+" dB";if(format==1)return v<-.01?"Dark "+juce::String((int)(-v*100)):v>.01?"Bright "+juce::String((int)(v*100)):"Neutral";return juce::String((int)std::round(v*100))+" %";}
    double getValueFromText(const juce::String& t)override{double v=t.retainCharacters("0123456789.-").getDoubleValue();if(format==1){v/=100;if(t.containsIgnoreCase("dark"))v=-std::abs(v);}if(format==2||format==3)v/=100;return v;}
    double defaultValue=0;int format=0;
};
}
