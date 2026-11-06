#include "Params.h"

namespace aj
{
juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using F  = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&](const char* pid, const char* name, float lo, float hi, float def, float skew = 1.f)
    {
        layout.add(std::make_unique<F>(juce::ParameterID{ pid, 1 }, name, NR(lo, hi, 0.f, skew), def));
    };
    auto addC = [&](const char* pid, const char* name, juce::StringArray choices, int def)
    {
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ pid, 1 }, name, choices, def));
    };
    auto addB = [&](const char* pid, const char* name, bool def)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ pid, 1 }, name, def));
    };
    auto addI = [&](const char* pid, const char* name, int lo, int hi, int def)
    {
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ pid, 1 }, name, lo, hi, def));
    };

    // LFO
    addF(id::lfoRate,  "LFO Rate",  0.05f, 30.f, 4.f, 0.35f);
    addF(id::lfoDelay, "LFO Delay", 0.f,   4.f,  0.f, 0.5f);
    addC(id::lfoWave,  "LFO Wave",  { "Tri", "Square", "Saw", "S&H" }, 0);

    // DCO
    addF(id::dcoLfo,   "DCO LFO",   0.f, 1.f, 0.f);
    addC(id::dcoRange, "Range",     { "16'", "8'", "4'" }, 1);
    addB(id::dcoPulse, "Pulse",     true);
    addB(id::dcoSaw,   "Saw",       true);
    addF(id::dcoPwm,   "PWM",       0.f, 1.f, 0.f);
    addC(id::pwmMode,  "PWM Mode",  { "Manual", "LFO", "Env" }, 0);
    addF(id::dcoSub,   "Sub",       0.f, 1.f, 0.f);
    addF(id::dcoNoise, "Noise",     0.f, 1.f, 0.f);
    addF(id::detune,   "Tune",      -50.f, 50.f, 0.f);
    addI(id::bend,     "Bend Range", 0, 12, 2);
    addF(id::porta,    "Portamento", 0.f, 3.f, 0.f, 0.4f);
    addC(id::voiceMode, "Voice Mode", { "Poly", "Mono", "Legato" }, 0);

    // HPF
    addI(id::hpf, "HPF", 0, 3, 0);

    // VCF
    addF(id::cutoff, "Cutoff",    0.f, 1.f, 0.65f);
    addF(id::res,    "Resonance", 0.f, 1.f, 0.1f);
    addF(id::vcfEnv, "VCF Env",  -1.f, 1.f, 0.35f);
    addF(id::vcfLfo, "VCF LFO",   0.f, 1.f, 0.f);
    addF(id::vcfKey, "Key Follow", 0.f, 1.f, 0.5f);

    addF(id::fA, "VCF Attack",  0.001f, 5.f,  0.005f, 0.3f);
    addF(id::fD, "VCF Decay",   0.002f, 10.f, 0.4f,   0.3f);
    addF(id::fS, "VCF Sustain", 0.f,    1.f,  0.4f);
    addF(id::fR, "VCF Release", 0.002f, 10.f, 0.4f,   0.3f);

    // VCA
    addF(id::aA, "VCA Attack",  0.001f, 5.f,  0.005f, 0.3f);
    addF(id::aD, "VCA Decay",   0.002f, 10.f, 0.4f,   0.3f);
    addF(id::aS, "VCA Sustain", 0.f,    1.f,  0.8f);
    addF(id::aR, "VCA Release", 0.002f, 10.f, 0.3f,   0.3f);
    addC(id::vcaMode, "VCA Mode", { "Env", "Gate" }, 0);
    addF(id::level, "Level", 0.f, 1.f, 0.8f);

    // Global
    addC(id::chorus, "Chorus", { "Off", "I", "II", "I+II" }, 1);
    addF(id::velocity, "Velocity", 0.f, 1.f, 0.3f);
    addF(id::master, "Volume", 0.f, 1.f, 0.7f);

    return layout;
}
} // namespace aj
