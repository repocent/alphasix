#pragma once

#include "Common.h"

namespace aj
{
namespace id
{
inline constexpr const char* lfoRate   = "lfoRate";
inline constexpr const char* lfoDelay  = "lfoDelay";
inline constexpr const char* lfoWave   = "lfoWave";

inline constexpr const char* dcoLfo    = "dcoLfo";
inline constexpr const char* dcoRange  = "dcoRange";
inline constexpr const char* dcoPulse  = "dcoPulse";
inline constexpr const char* dcoSaw    = "dcoSaw";
inline constexpr const char* dcoPwm    = "dcoPwm";
inline constexpr const char* pwmMode   = "pwmMode";
inline constexpr const char* dcoSub    = "dcoSub";
inline constexpr const char* dcoNoise  = "dcoNoise";
inline constexpr const char* detune    = "detune";
inline constexpr const char* bend      = "bend";
inline constexpr const char* porta     = "porta";
inline constexpr const char* voiceMode = "voiceMode";

inline constexpr const char* hpf       = "hpf";

inline constexpr const char* cutoff    = "cutoff";
inline constexpr const char* res       = "res";
inline constexpr const char* vcfEnv    = "vcfEnv";
inline constexpr const char* vcfLfo    = "vcfLfo";
inline constexpr const char* vcfKey    = "vcfKey";

inline constexpr const char* fA = "fA";
inline constexpr const char* fD = "fD";
inline constexpr const char* fS = "fS";
inline constexpr const char* fR = "fR";

inline constexpr const char* aA = "aA";
inline constexpr const char* aD = "aD";
inline constexpr const char* aS = "aS";
inline constexpr const char* aR = "aR";
inline constexpr const char* vcaMode = "vcaMode";
inline constexpr const char* level   = "level";

inline constexpr const char* chorus   = "chorus";
inline constexpr const char* velocity = "velocity";
inline constexpr const char* master   = "master";
} // namespace id

/** Plain-value snapshot of every parameter, read once per audio block. */
struct SynthParams
{
    float lfoRate = 4.f, lfoDelay = 0.f;
    int   lfoWave = 0;

    float dcoLfo = 0.f;
    int   range = 1;            // 0 = 16', 1 = 8', 2 = 4'
    bool  pulse = true, saw = true;
    float pwm = 0.f;
    int   pwmMode = 0;          // 0 manual, 1 LFO, 2 env
    float sub = 0.f, noise = 0.f;
    float detuneCents = 0.f;
    float bendRange = 2.f;
    float porta = 0.f;
    int   voiceMode = 0;        // 0 poly, 1 mono, 2 legato

    int   hpf = 0;              // 0..3

    float cutoff = 0.6f, res = 0.f, vcfEnv = 0.f, vcfLfo = 0.f, vcfKey = 0.5f;
    float fA = 0.01f, fD = 0.3f, fS = 0.5f, fR = 0.3f;
    float aA = 0.01f, aD = 0.3f, aS = 0.8f, aR = 0.3f;
    int   vcaMode = 0;          // 0 env, 1 gate
    float level = 0.8f;

    int   chorus = 1;           // 0 off, 1 I, 2 II, 3 I+II
    float velocity = 0.3f;
    float master = 0.7f;
};

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

} // namespace aj
