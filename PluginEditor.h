#pragma once
#include "PluginProcessor.h"
#include <array>

class OrbitLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrbitLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class OrbitAudioProcessorEditor : public juce::AudioProcessorEditor,private juce::Timer
{
public:
    explicit OrbitAudioProcessorEditor(OrbitAudioProcessor&);
    ~OrbitAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    OrbitAudioProcessor& processor;
    OrbitLookAndFeel look;
    std::array<juce::Slider,2> knobs;
    using Attachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<Attachment>,2> attachments;
    juce::TooltipWindow tooltip;
    juce::Point<float> arrowPoint;
    std::array<juce::Point<float>,8> trail{};
    float reduction=0,imbalance=0;
    double lastTime=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitAudioProcessorEditor)
};
