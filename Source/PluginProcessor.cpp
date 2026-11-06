#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace aj;

namespace
{
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

// Choice indices: range 0=16' 1=8' 2=4' | pwmMode 0=Manual 1=LFO 2=Env | lfoWave 0=Tri 1=Sq 2=Saw 3=S&H
//                 voiceMode 0=Poly 1=Mono 2=Legato | vcaMode 0=Env 1=Gate | chorus 0=Off 1=I 2=II 3=I+II
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list = {
        { "Init", {} },

        { "Brass", {
            { id::dcoPulse, 0 }, { id::dcoSaw, 1 }, { id::dcoSub, 0.2f },
            { id::cutoff, 0.45f }, { id::res, 0.15f }, { id::vcfEnv, 0.55f }, { id::vcfKey, 0.4f },
            { id::fA, 0.12f }, { id::fD, 0.5f }, { id::fS, 0.6f }, { id::fR, 0.25f },
            { id::aA, 0.04f }, { id::aD, 0.3f }, { id::aS, 0.9f }, { id::aR, 0.2f },
            { id::chorus, 2 } } },

        { "Warm Strings", {
            { id::dcoPulse, 1 }, { id::dcoSaw, 1 }, { id::pwmMode, 1 }, { id::dcoPwm, 0.45f },
            { id::lfoRate, 0.6f }, { id::cutoff, 0.62f }, { id::res, 0.05f }, { id::vcfEnv, 0.1f },
            { id::fA, 0.6f }, { id::fS, 0.8f },
            { id::aA, 0.55f }, { id::aD, 0.5f }, { id::aS, 1.f }, { id::aR, 0.9f },
            { id::hpf, 1 }, { id::chorus, 2 } } },

        { "Fat Bass", {
            { id::dcoRange, 0 }, { id::dcoPulse, 1 }, { id::dcoSaw, 1 }, { id::dcoSub, 0.5f },
            { id::cutoff, 0.3f }, { id::res, 0.35f }, { id::vcfEnv, 0.45f }, { id::vcfKey, 0.3f },
            { id::fA, 0.002f }, { id::fD, 0.25f }, { id::fS, 0.1f }, { id::fR, 0.2f },
            { id::aA, 0.002f }, { id::aD, 0.3f }, { id::aS, 0.9f }, { id::aR, 0.12f },
            { id::voiceMode, 1 }, { id::chorus, 0 } } },

        { "Square Lead", {
            { id::dcoPulse, 1 }, { id::dcoSaw, 0 }, { id::dcoPwm, 0.3f }, { id::dcoSub, 0.15f },
            { id::dcoLfo, 0.12f }, { id::lfoRate, 5.5f }, { id::lfoDelay, 0.5f },
            { id::cutoff, 0.72f }, { id::res, 0.25f }, { id::vcfEnv, 0.2f },
            { id::porta, 0.12f }, { id::voiceMode, 2 },
            { id::aA, 0.005f }, { id::aS, 0.9f }, { id::aR, 0.25f }, { id::chorus, 1 } } },

        { "Pluck", {
            { id::dcoPulse, 1 }, { id::dcoSaw, 0 }, { id::pwmMode, 2 }, { id::dcoPwm, 0.5f },
            { id::cutoff, 0.35f }, { id::res, 0.3f }, { id::vcfEnv, 0.6f },
            { id::fA, 0.001f }, { id::fD, 0.18f }, { id::fS, 0.f }, { id::fR, 0.2f },
            { id::aA, 0.001f }, { id::aD, 0.35f }, { id::aS, 0.f }, { id::aR, 0.25f },
            { id::chorus, 2 } } },

        { "Sweep Pad", {
            { id::dcoPulse, 0 }, { id::dcoSaw, 1 }, { id::dcoSub, 0.35f },
            { id::cutoff, 0.25f }, { id::res, 0.45f }, { id::vcfEnv, 0.7f }, { id::vcfLfo, 0.2f },
            { id::lfoRate, 0.35f },
            { id::fA, 2.f }, { id::fD, 2.5f }, { id::fS, 0.5f }, { id::fR, 2.5f },
            { id::aA, 1.2f }, { id::aD, 1.f }, { id::aS, 0.9f }, { id::aR, 2.f },
            { id::chorus, 1 } } },

        { "Wind", {
            { id::dcoPulse, 0 }, { id::dcoSaw, 0 }, { id::dcoNoise, 0.9f },
            { id::cutoff, 0.5f }, { id::res, 0.6f }, { id::vcfEnv, 0.f }, { id::vcfLfo, 0.45f },
            { id::lfoRate, 0.2f }, { id::lfoWave, 3 },
            { id::aA, 1.5f }, { id::aS, 1.f }, { id::aR, 2.f }, { id::chorus, 2 } } },
    };
    return list;
}
} // namespace

//==============================================================================
AlphaSixProcessor::AlphaSixProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
}

void AlphaSixProcessor::prepareToPlay(double sampleRate, int)
{
    engine.setParams(readParams());
    engine.prepare(sampleRate);
    engine.setParams(readParams());
}

bool AlphaSixProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

SynthParams AlphaSixProcessor::readParams() const
{
    SynthParams p;
    p.lfoRate = raw(id::lfoRate);
    p.lfoDelay = raw(id::lfoDelay);
    p.lfoWave = (int) raw(id::lfoWave);

    p.dcoLfo = raw(id::dcoLfo);
    p.range = (int) raw(id::dcoRange);
    p.pulse = raw(id::dcoPulse) > 0.5f;
    p.saw = raw(id::dcoSaw) > 0.5f;
    p.pwm = raw(id::dcoPwm);
    p.pwmMode = (int) raw(id::pwmMode);
    p.sub = raw(id::dcoSub);
    p.noise = raw(id::dcoNoise);
    p.detuneCents = raw(id::detune);
    p.bendRange = raw(id::bend);
    p.porta = raw(id::porta);
    p.voiceMode = (int) raw(id::voiceMode);

    p.hpf = (int) raw(id::hpf);

    p.cutoff = raw(id::cutoff);
    p.res = raw(id::res);
    p.vcfEnv = raw(id::vcfEnv);
    p.vcfLfo = raw(id::vcfLfo);
    p.vcfKey = raw(id::vcfKey);

    p.fA = raw(id::fA); p.fD = raw(id::fD); p.fS = raw(id::fS); p.fR = raw(id::fR);
    p.aA = raw(id::aA); p.aD = raw(id::aD); p.aS = raw(id::aS); p.aR = raw(id::aR);
    p.vcaMode = (int) raw(id::vcaMode);
    p.level = raw(id::level);

    p.chorus = (int) raw(id::chorus);
    p.velocity = raw(id::velocity);
    p.master = raw(id::master);
    return p;
}

void AlphaSixProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2)
        return;

    engine.setParams(readParams());

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit(pos, numSamples, meta.samplePosition);
        if (at > pos)
        {
            engine.render(left + pos, right + pos, at - pos);
            pos = at;
        }
        engine.handleMidi(meta.getMessage());
    }
    if (pos < numSamples)
        engine.render(left + pos, right + pos, numSamples - pos);

    midi.clear();
}

juce::AudioProcessorEditor* AlphaSixProcessor::createEditor()
{
    return new AlphaSixEditor(*this);
}

//==============================================================================
int AlphaSixProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String AlphaSixProcessor::getProgramName(int index)
{
    if (index >= 0 && index < getNumPrograms())
        return presets()[(size_t) index].name;
    return {};
}

void AlphaSixProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    currentProgram = index;

    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm))
            r->setValueNotifyingHost(r->getDefaultValue());

    for (auto& kv : presets()[(size_t) index].values)
        if (auto* r = apvts.getParameter(kv.first))
            r->setValueNotifyingHost(r->convertTo0to1(kv.second));
}

void AlphaSixProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void AlphaSixProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            currentProgram = (int) tree.getProperty("program", 0);
            apvts.replaceState(tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AlphaSixProcessor();
}
