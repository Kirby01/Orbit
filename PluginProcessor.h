#pragma once
#include <JuceHeader.h>
#include "OrbitDSP.h"
#include <atomic>
#include <cstdint>

class OrbitAudioProcessor : public juce::AudioProcessor
{
public:
    OrbitAudioProcessor();
    ~OrbitAudioProcessor() override = default;
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    const juce::String getName() const override { return "Orbit"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState parameters;
    juce::Point<float> getArrow() const noexcept;
    float getReduction() const noexcept { return meter.load(std::memory_order_relaxed); }
    float getImbalance() const noexcept { return imbalance.load(std::memory_order_relaxed); }
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    void updateTargets(bool immediate);
    OrbitDSP dsp;
    std::atomic<float>* gravityParam=nullptr;
    std::atomic<float>* motionParam=nullptr;
    juce::SmoothedValue<double> gravity, motion;
    // Pack x/y into a single lock-free 32-bit snapshot, avoiding torn coordinates.
    std::atomic<std::uint32_t> arrow{0x80008000u};
    std::atomic<float> meter{0}, imbalance{0};
    double rate=48000, meterDecay=0, meterEnvelope=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitAudioProcessor)
};
