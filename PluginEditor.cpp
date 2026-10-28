#include "PluginEditor.h"

namespace
{
    const juce::Colour mint(0xffa8edbb), jade(0xff5fbf8d), ivory(0xffe1f2e3);
    const juce::Colour ink(0xff0b241c), muted(0xff92b3a0), line(0xff315843);
    juce::Font font(float size,bool bold=false)
    {
       #if JUCE_MAJOR_VERSION >= 8
        return juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
       #else
        return juce::Font(size, bold ? juce::Font::bold : juce::Font::plain);
       #endif
    }
}
OrbitLookAndFeel::OrbitLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId,ivory);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId,jade.withAlpha(.3f));
    setColour(juce::TooltipWindow::backgroundColourId,ink);
    setColour(juce::TooltipWindow::textColourId,ivory);
    setColour(juce::TooltipWindow::outlineColourId,line);
}
void OrbitLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
                                      float value,float start,float end,juce::Slider&)
{
    const float radius=juce::jmin(static_cast<float>(w),static_cast<float>(h))*.5f-10.f;
    const float cx=x+w*.5f,cy=y+h*.5f,angle=start+value*(end-start);
    juce::Path arc; arc.addCentredArc(cx,cy,radius+4,radius+4,0,start,end,true);
    g.setColour(ink);g.strokePath(arc,juce::PathStrokeType(3));
    juce::Path active;active.addCentredArc(cx,cy,radius+4,radius+4,0,start,angle,true);
    g.setColour(jade);g.strokePath(active,juce::PathStrokeType(3));
    g.setColour(juce::Colours::black.withAlpha(.22f));
    g.fillEllipse(cx-radius,cy-radius+3,radius*2,radius*2);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff35654d),cx,cy-radius,
                                         juce::Colour(0xff163c2d),cx,cy+radius,false));
    g.fillEllipse(cx-radius,cy-radius,radius*2,radius*2);
    g.setColour(juce::Colour(0xff6b9d7e).withAlpha(.65f));
    g.drawEllipse(cx-radius+.5f,cy-radius+.5f,radius*2-1,radius*2-1,1);
    g.setColour(mint);
    g.drawLine(cx+std::sin(angle)*radius*.5f,cy-std::cos(angle)*radius*.5f,
               cx+std::sin(angle)*radius*.83f,cy-std::cos(angle)*radius*.83f,2.5f);
}
OrbitAudioProcessorEditor::OrbitAudioProcessorEditor(OrbitAudioProcessor& p)
 : AudioProcessorEditor(&p),processor(p),tooltip(this,650)
{
    setLookAndFeel(&look);
    const std::array<const char*,2> ids{"gravity","motion"};
    const std::array<const char*,2> names{"Gravity","Motion"};
    const std::array<const char*,2> tips{
      "Reflex base drive. 1 = no added detector drive. Double-click: 1.50.",
      "Extra detector drive from wheel imbalance, in dB. 0 = plain Reflex. Double-click: 4 dB."};
    for(std::size_t i=0;i<knobs.size();++i)
    {
        auto& k=knobs[i];
        k.setLookAndFeel(&look);
        k.setName(names[i]);k.setTooltip(tips[i]);
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,78,22);
        k.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
        k.setScrollWheelEnabled(false);
        addAndMakeVisible(k);
        attachments[i]=std::make_unique<Attachment>(processor.parameters,ids[i],k);
        k.textFromValueFunction=[i](double v){return juce::String(v,i==0?2:1)+(i==1?" dB":"");};
        k.valueFromTextFunction=[](const juce::String& t){return t.getDoubleValue();};
        k.setDoubleClickReturnValue(true,i==0?1.5:4.);
        k.updateText();
    }
    arrowPoint=processor.getArrow();trail.fill(arrowPoint);
    setResizable(false,false);setSize(440,280);
    lastTime=juce::Time::getMillisecondCounterHiRes();startTimerHz(60);
}
OrbitAudioProcessorEditor::~OrbitAudioProcessorEditor()
{
    stopTimer();
    for(auto& knob:knobs)knob.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}
void OrbitAudioProcessorEditor::resized()
{
    knobs[0].setBounds(238,91,86,94);
    knobs[1].setBounds(336,91,86,94);
}
void OrbitAudioProcessorEditor::timerCallback()
{
    const double now=juce::Time::getMillisecondCounterHiRes();
    const float mix=static_cast<float>(1-std::exp(-std::clamp(now-lastTime,0.,100.)/25.));lastTime=now;
    const auto next=processor.getArrow();
    arrowPoint=arrowPoint+(next-arrowPoint)*mix; // Display smoothing only.
    for(std::size_t i=trail.size()-1;i>0;--i)
        trail[i]=trail[i-1];
    trail[0]=arrowPoint;
    reduction=processor.getReduction();imbalance=processor.getImbalance();
    repaint();
}
void OrbitAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff214b37),0,0,juce::Colour(0xff102d22),440,280,false));
    g.fillAll();
    g.setColour(juce::Colour(0xff5d8667).withAlpha(.28f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),8,1);
    g.setColour(ivory);g.setFont(font(29,true));g.drawText("ORBIT",23,14,150,37,juce::Justification::centredLeft);
    g.setColour(muted);g.setFont(font(9.5f,true));
    g.drawText("HARMONIOUS RECORDS",233,21,184,17,juce::Justification::centredRight);
    g.setFont(font(10));g.drawText("EVENT WHEEL COMPRESSOR",232,38,185,16,juce::Justification::centredRight);
    g.setColour(line);g.drawHorizontalLine(63,23,417);

    constexpr float cx=121,cy=156,r=70;
    g.setColour(ink);g.fillEllipse(cx-r-7,cy-r-7,2*r+14,2*r+14);
    g.setColour(line);g.drawEllipse(cx-r,cy-r,2*r,2*r,1);
    g.setColour(line.withAlpha(.55f));g.drawEllipse(cx-r*.5f,cy-r*.5f,r,r,.75f);
    g.drawLine(cx-r+5,cy,cx+r-5,cy,.6f);g.drawLine(cx,cy-r+5,cx,cy+r-5,.6f);
    for(int i=0;i<24;++i)
    {
        const float a=juce::MathConstants<float>::twoPi*static_cast<float>(i)/24;
        const float x=cx+(r+4)*std::cos(a),y=cy-(r+4)*std::sin(a);
        g.setColour(muted.withAlpha(i%6==0?.65f:.25f));g.fillEllipse(x-1,y-1,2,2);
    }
    for(std::size_t i=trail.size()-1;i>0;--i)
    {
        const auto a=trail[i],b=trail[i-1];
        g.setColour(jade.withAlpha(.25f*(1-static_cast<float>(i)/trail.size())));
        g.drawLine(cx+r*a.x,cy-r*a.y,cx+r*b.x,cy-r*b.y,2);
    }
    const juce::Point<float> centre(cx,cy),tip(cx+r*arrowPoint.x,cy-r*arrowPoint.y);
    g.setColour(mint.withAlpha(.09f));g.fillEllipse(tip.x-8,tip.y-8,16,16);
    if(centre.getDistanceFrom(tip)>3)
    {
        g.setColour(mint);g.drawArrow(juce::Line<float>(centre,tip),1.7f,6,7);
    }
    g.setColour(ivory);g.fillEllipse(tip.x-2.3f,tip.y-2.3f,4.6f,4.6f);
    g.setColour(muted);g.fillEllipse(cx-1.5f,cy-1.5f,3,3);
    g.setFont(font(9.5f));g.drawText("BALANCE",65,238,112,16,juce::Justification::centred);

    g.setFont(font(11,true));g.setColour(ivory);
    g.drawText("GRAVITY",238,75,86,18,juce::Justification::centred);
    g.drawText("MOTION",336,75,86,18,juce::Justification::centred);
    g.setColour(muted);g.setFont(font(9.5f));
    g.drawText("BASE DRIVE",238,185,86,16,juce::Justification::centred);
    g.drawText("WHEEL DEPTH",336,185,86,16,juce::Justification::centred);
    g.setColour(line);g.drawHorizontalLine(211,240,417);
    g.setColour(muted);g.setFont(font(10));g.drawText("REDUCTION",240,218,100,17,juce::Justification::centredLeft);
    g.setColour(mint);g.setFont(font(13,true));
    g.drawText(juce::String(reduction,1)+" dB",334,216,83,21,juce::Justification::centredRight);
    g.setColour(ink);g.fillRoundedRectangle(240,242,177,4,2);
    g.setColour(jade);g.fillRoundedRectangle(240,242,177*juce::jlimit(0.f,1.f,reduction/24),4,2);
    g.setColour(muted.withAlpha(.7f));g.setFont(font(9));
    g.drawText("ZERO LATENCY  /  STEREO LINKED",23,260,240,12,juce::Justification::centredLeft);
    g.drawText("IMBALANCE "+juce::String(juce::roundToInt(imbalance*100))+"%",300,260,117,12,juce::Justification::centredRight);
}
