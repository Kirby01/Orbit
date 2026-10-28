#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <limits>

static_assert(std::atomic<std::uint32_t>::is_always_lock_free,"Orbit requires lock-free 32-bit atomics");
static_assert(std::atomic<float>::is_always_lock_free,"Orbit requires lock-free float atomics");
OrbitAudioProcessor::OrbitAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                                  .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
   parameters(*this,nullptr,"OrbitState",makeParameters())
{
    gravityParam=parameters.getRawParameterValue("gravity");
    motionParam=parameters.getRawParameterValue("motion");
    setLatencySamples(0);
}
juce::AudioProcessorValueTreeState::ParameterLayout OrbitAudioProcessor::makeParameters()
{
    juce::NormalisableRange<float> driveRange{1.f,40.f,
      [](float start,float end,float n){return start*std::pow(end/start,n);},
      [](float start,float end,float value){return std::log(value/start)/std::log(end/start);}};
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gravity",1},"Gravity",driveRange,1.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"motion",1},"Motion",juce::NormalisableRange<float>(0,12,.01f),4.f));
    return layout;
}
bool OrbitAudioProcessor::isBusesLayoutSupported(const BusesLayout& b) const
{
    const auto out=b.getMainOutputChannelSet();
    return (out==juce::AudioChannelSet::mono()||out==juce::AudioChannelSet::stereo()) && b.getMainInputChannelSet()==out;
}
void OrbitAudioProcessor::updateTargets(bool immediate)
{
    auto set=[immediate](auto& s,double value){if(immediate)s.setCurrentAndTargetValue(value);else s.setTargetValue(value);};
    set(gravity,gravityParam->load()); set(motion,motionParam->load());
}
void OrbitAudioProcessor::prepareToPlay(double sr,int)
{
    rate=sr>0?sr:48000.;
    gravity.reset(rate,.02); motion.reset(rate,.02);
    meterDecay=std::exp(-1/(rate*.18));
    dsp.prepare(rate); reset();
}
void OrbitAudioProcessor::reset()
{
    dsp.reset(); meterEnvelope=0;
    arrow.store(0x80008000u,std::memory_order_relaxed);
    meter.store(0,std::memory_order_relaxed); imbalance.store(0,std::memory_order_relaxed);
    updateTargets(true);
}
void OrbitAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n=juce::jmin(getTotalNumInputChannels(),buffer.getNumChannels());
    for(int c=n;c<buffer.getNumChannels();++c)buffer.clear(c,0,buffer.getNumSamples());
    if(n<1)return;
    updateTargets(false);
    auto* l=buffer.getWritePointer(0); auto* r=n>1?buffer.getWritePointer(1):nullptr;
    for(int i=0;i<buffer.getNumSamples();++i)
    {
        const auto out=dsp.process(l[i],r?r[i]:l[i],gravity.getNextValue(),motion.getNextValue());
        l[i]=static_cast<float>(out.left); if(r)r[i]=static_cast<float>(out.right);
        meterEnvelope=std::max(dsp.getReductionDb(),meterEnvelope*meterDecay);
    }
    auto encode=[](double v){return static_cast<std::uint32_t>(std::lround((std::clamp(v,-1.,1.)+1.)*32767.5));};
    arrow.store(encode(dsp.getX())|(encode(dsp.getY())<<16),std::memory_order_relaxed);
    meter.store(static_cast<float>(meterEnvelope),std::memory_order_relaxed);
    imbalance.store(static_cast<float>(dsp.getBalance()),std::memory_order_relaxed);
}
juce::Point<float> OrbitAudioProcessor::getArrow() const noexcept
{
    const auto packed=arrow.load(std::memory_order_relaxed);
    return {static_cast<float>(packed&0xffffu)/32767.5f-1.f,static_cast<float>(packed>>16)/32767.5f-1.f};
}
void OrbitAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state=parameters.copyState(); state.setProperty("version",1,nullptr);
    if(auto xml=state.createXml())copyXmlToBinary(*xml,dest);
}
void OrbitAudioProcessor::setStateInformation(const void* data,int size)
{
    if(auto xml=getXmlFromBinary(data,size))
        if(xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* OrbitAudioProcessor::createEditor(){return new OrbitAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new OrbitAudioProcessor();}
