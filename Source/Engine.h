#pragma once

#include "Common.h"
#include "Params.h"

// DSP core of AlphaSix: a 6-voice DCO synth in the spirit of the Roland Alpha Juno.
//   DCO (pulse + saw + sub + noise, PWM)  ->  HPF (4 steps)  ->  24 dB/oct resonant LPF  ->  VCA
//   global LFO, two analog-style ADSR envelopes, stereo BBD-style chorus.

namespace aj
{
constexpr int kNumVoices = 6;
constexpr int kChunk = 16; // control-rate block size (samples)

inline float polyBlep(float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

inline float wrap01(float x)
{
    x -= std::floor(x);
    return x;
}

//==============================================================================
struct Envelope
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float value = 0.f;

    void gateOn() { stage = Attack; }
    void gateOff() { if (stage != Idle) stage = Release; }
    void kill() { stage = Idle; value = 0.f; }

    // Coefficients are one-pole "per-sample" rates. Attack aims past 1.0 (RC-charge style).
    float process(float S, float aC, float dC, float rC)
    {
        switch (stage)
        {
            case Idle: break;
            case Attack:
                value += (1.3f - value) * aC;
                if (value >= 1.f) { value = 1.f; stage = Decay; }
                break;
            case Decay:
                value += (S - value) * dC;
                if (value <= S + 0.0005f) stage = Sustain;
                break;
            case Sustain:
                value += (S - value) * 0.01f;
                if (S < 1.0e-4f && value < 1.0e-4f) { value = 0.f; stage = Idle; }
                break;
            case Release:
                value += (0.f - value) * rC;
                if (value < 1.0e-4f) { value = 0.f; stage = Idle; }
                break;
        }
        return value;
    }
};

//==============================================================================
/** Everything a voice needs for one control-rate chunk, already resolved. */
struct Chunk
{
    int   n = kChunk;
    float sr = 44100.f;
    float lfo = 0.f, lfoDelay = 0.f;
    float vibSemis = 0.f, bendSemis = 0.f, tuneSemis = 0.f, rangeSemis = 0.f;
    float portaTau = 0.f;
    bool  pulse = true, saw = true;
    float sub = 0.f, noise = 0.f;
    int   pwmMode = 0;
    float pwm = 0.f;
    int   hpf = 0;
    float cutoff = 0.5f, res = 0.f, vcfEnv = 0.f, vcfLfo = 0.f, vcfKey = 0.f;
    float fS = 0.5f, aS = 1.f;
    float fAc = 0.f, fDc = 0.f, fRc = 0.f, aAc = 0.f, aDc = 0.f, aRc = 0.f;
    float level = 1.f;
};

//==============================================================================
struct Voice
{
    Envelope fEnv, aEnv;
    bool gate = false, sustained = false;
    int  note = -1;
    float velGain = 1.f;
    float phase = 0.f, subPhase = 0.f;
    float curNote = 60.f, targetNote = 60.f;
    float lfoTimer = 0.f;
    uint64_t age = 0;

    float z1 = 0, z2 = 0, z3 = 0, z4 = 0, hz = 0;

    bool active() const { return aEnv.stage != Envelope::Idle; }

    void resetFilter() { z1 = z2 = z3 = z4 = hz = 0.f; }

    void start(int n, float vel, float velSens, float glideFrom, bool retrigger, uint64_t newAge)
    {
        const bool wasActive = active();
        note = n;
        targetNote = (float) n;
        curNote = glideFrom;
        gate = true;
        sustained = false;
        age = newAge;
        velGain = 1.f - velSens * (1.f - vel);
        lfoTimer = 0.f;
        if (!wasActive)
        {
            phase = 0.f;
            subPhase = 0.f;
            resetFilter();
            fEnv.value = 0.f;
            aEnv.value = 0.f;
        }
        if (retrigger || !wasActive)
        {
            fEnv.gateOn();
            aEnv.gateOn();
        }
    }

    void release()
    {
        gate = false;
        sustained = false;
        fEnv.gateOff();
        aEnv.gateOff();
    }

    void render(float* out, const Chunk& c, juce::Random& rng)
    {
        const float sr = c.sr;
        const int n = c.n;

        if (c.portaTau > 0.0005f)
            curNote += (targetNote - curNote) * (1.f - std::exp(-(float) n / (c.portaTau * sr)));
        else
            curNote = targetNote;

        lfoTimer += (float) n / sr;
        const float dg = c.lfoDelay <= 0.001f ? 1.f : juce::jlimit(0.f, 1.f, lfoTimer / c.lfoDelay);
        const float lfo = c.lfo * dg;

        const float semis = curNote + c.rangeSemis + c.tuneSemis + c.bendSemis + lfo * c.vibSemis;
        const float freq = 440.f * std::pow(2.f, (semis - 69.f) / 12.f);
        const float dt = juce::jmin(freq / sr, 0.45f);
        const float sdt = dt * 0.5f;

        float pw = 0.5f;
        switch (c.pwmMode)
        {
            case 0:  pw = 0.5f - 0.45f * c.pwm; break;
            case 1:  pw = 0.5f + 0.45f * c.pwm * lfo; break;
            default: pw = 0.5f - 0.45f * c.pwm * fEnv.value; break;
        }
        pw = juce::jlimit(0.05f, 0.95f, pw);

        // Filter coefficients (zero-delay-feedback 4-pole ladder)
        const float oct = 10.f * c.cutoff + c.vcfEnv * 8.f * fEnv.value + c.vcfLfo * 3.f * lfo
                        + c.vcfKey * (curNote - 60.f) / 12.f;
        const float fc = juce::jlimit(20.f, juce::jmin(0.45f * sr, 20000.f), 20.f * std::pow(2.f, oct));
        const float g = std::tan(juce::MathConstants<float>::pi * fc / sr);
        const float G = g / (1.f + g);
        const float G2 = G * G, G3 = G2 * G, G4 = G2 * G2;
        const float omG = 1.f - G;
        const float k = 4.f * c.res;
        const float denom = 1.f + k * G4;

        // HPF (one-pole)
        static constexpr float hpfHz[4] = { 0.f, 230.f, 520.f, 1500.f };
        const bool useHpf = c.hpf > 0;
        float hG = 0.f;
        if (useHpf)
        {
            const float hg = std::tan(juce::MathConstants<float>::pi * hpfHz[c.hpf] / sr);
            hG = hg / (1.f + hg);
        }

        for (int i = 0; i < n; ++i)
        {
            const float fe = fEnv.process(c.fS, c.fAc, c.fDc, c.fRc);
            (void) fe;
            const float ae = aEnv.process(c.aS, c.aAc, c.aDc, c.aRc);

            phase += dt;
            if (phase >= 1.f) phase -= 1.f;
            subPhase += sdt;
            if (subPhase >= 1.f) subPhase -= 1.f;

            float s = 0.f;
            if (c.pulse)
            {
                float v = phase < pw ? 1.f : -1.f;
                v += polyBlep(phase, dt);
                v -= polyBlep(wrap01(phase - pw), dt);
                s += 0.5f * v;
            }
            if (c.saw)
            {
                float v = 2.f * phase - 1.f;
                v -= polyBlep(phase, dt);
                s += 0.5f * v;
            }
            if (c.sub > 0.0005f)
            {
                float v = subPhase < 0.5f ? 1.f : -1.f;
                v += polyBlep(subPhase, sdt);
                v -= polyBlep(wrap01(subPhase - 0.5f), sdt);
                s += 0.5f * c.sub * v;
            }
            if (c.noise > 0.0005f)
                s += 0.5f * c.noise * (rng.nextFloat() * 2.f - 1.f);

            if (useHpf)
            {
                const float v = hG * (s - hz);
                const float lp = v + hz;
                hz = lp + v;
                s = s - lp;
            }

            // 24 dB/oct resonant low-pass with soft saturation
            const float S = omG * (G3 * z1 + G2 * z2 + G * z3 + z4);
            float u = (s - k * S) / denom;
            u = std::tanh(u);

            float v = G * (u - z1);
            const float y1 = v + z1;
            z1 = y1 + v;
            v = G * (y1 - z2);
            const float y2 = v + z2;
            z2 = y2 + v;
            v = G * (y2 - z3);
            const float y3 = v + z3;
            z3 = y3 + v;
            v = G * (y3 - z4);
            const float y4 = v + z4;
            z4 = y4 + v;

            out[i] += y4 * ae * velGain * c.level;
        }

        if (!std::isfinite(z1 + z2 + z3 + z4 + hz))
            resetFilter();

        if (aEnv.stage == Envelope::Idle)
            gate = false;
    }
};

//==============================================================================
/** Juno-style stereo chorus: two modulated short delays, opposite LFO phase. */
struct Chorus
{
    void prepare(double sampleRate)
    {
        sr = (float) sampleRate;
        buf.assign(8192, 0.f);
        w = 0;
        ph = 0.f;
        lpL = lpR = 0.f;
        lpCoef = 1.f - std::exp(-2.f * juce::MathConstants<float>::pi * 7000.f / sr);
    }

    void setMode(int m) { mode = m; }

    void process(float in, float& outL, float& outR)
    {
        buf[(size_t) w] = in;
        if (mode == 0)
        {
            outL = outR = in;
            w = (w + 1) & 8191;
            return;
        }

        float rate, depth;
        switch (mode)
        {
            case 1:  rate = 0.513f; depth = 0.0010f; break;
            case 2:  rate = 0.863f; depth = 0.0012f; break;
            default: rate = 9.75f;  depth = 0.00025f; break;
        }
        const float base = 0.0035f;

        ph += rate / sr;
        if (ph >= 1.f) ph -= 1.f;
        const float tri = ph < 0.5f ? 4.f * ph - 1.f : 3.f - 4.f * ph;

        const float wetL = tap((base + depth * tri) * sr);
        const float wetR = tap((base - depth * tri) * sr);
        lpL += lpCoef * (wetL - lpL);
        lpR += lpCoef * (wetR - lpR);

        outL = 0.5f * (in + lpL);
        outR = 0.5f * (in + lpR);
        w = (w + 1) & 8191;
    }

private:
    float tap(float delaySamples) const
    {
        float rp = (float) w - delaySamples;
        while (rp < 0.f) rp += 8192.f;
        const int i0 = (int) rp;
        const float fr = rp - (float) i0;
        const int i1 = (i0 + 1) & 8191;
        return buf[(size_t) (i0 & 8191)] * (1.f - fr) + buf[(size_t) i1] * fr;
    }

    std::vector<float> buf;
    int w = 0;
    float sr = 44100.f, ph = 0.f, lpL = 0.f, lpR = 0.f, lpCoef = 0.5f;
    int mode = 0;
};

//==============================================================================
class Engine
{
public:
    void prepare(double sampleRate)
    {
        sr = (float) sampleRate;
        chorus.prepare(sampleRate);
        for (auto& v : voices) { v.fEnv.kill(); v.aEnv.kill(); v.gate = false; v.resetFilter(); }
        auto init = [&](juce::SmoothedValue<float>& s, float val) { s.reset(sampleRate, 0.02); s.setCurrentAndTargetValue(val); };
        init(sCutoff, p.cutoff); init(sRes, p.res); init(sVcfEnv, p.vcfEnv); init(sVcfLfo, p.vcfLfo);
        init(sPwm, p.pwm); init(sSub, p.sub); init(sNoise, p.noise); init(sLevel, p.level);
        init(sDco, p.dcoLfo); init(sTune, p.detuneCents);
        masterSm.reset(sampleRate, 0.02);
        masterSm.setCurrentAndTargetValue(p.master);
        noteStack.clear();
        pedal = false;
        dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.f;
    }

    void setParams(const SynthParams& np)
    {
        p = np;
        sCutoff.setTargetValue(p.cutoff); sRes.setTargetValue(p.res);
        sVcfEnv.setTargetValue(p.vcfEnv); sVcfLfo.setTargetValue(p.vcfLfo);
        sPwm.setTargetValue(p.pwm); sSub.setTargetValue(p.sub); sNoise.setTargetValue(p.noise);
        sLevel.setTargetValue(p.level); sDco.setTargetValue(p.dcoLfo); sTune.setTargetValue(p.detuneCents);
        masterSm.setTargetValue(p.master);
        chorus.setMode(p.chorus);
    }

    void handleMidi(const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
            noteOn(m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            noteOff(m.getNoteNumber());
        else if (m.isPitchWheel())
            bendNorm = (float) (m.getPitchWheelValue() - 8192) / 8192.f;
        else if (m.isController())
        {
            const int cc = m.getControllerNumber();
            const int val = m.getControllerValue();
            if (cc == 1) modWheel = (float) val / 127.f;
            else if (cc == 64) setPedal(val >= 64);
            else if (cc == 120 || cc == 123) allNotesOff(cc == 120);
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            allNotesOff(m.isAllSoundOff());
    }

    void render(float* left, float* right, int numSamples)
    {
        int done = 0;
        while (done < numSamples)
        {
            const int n = juce::jmin(kChunk, numSamples - done);
            Chunk c = makeChunk(n);

            float mono[kChunk] = {};
            for (auto& v : voices)
                if (v.active())
                    v.render(mono, c, rng);

            for (int i = 0; i < n; ++i)
            {
                float l, r;
                chorus.process(mono[i] * 0.45f, l, r);
                const float mg = masterSm.getNextValue();
                // DC blocker (pulse waves with PWM carry DC)
                const float yl = l - dcX[0] + 0.9985f * dcY[0]; dcX[0] = l; dcY[0] = yl;
                const float yr = r - dcX[1] + 0.9985f * dcY[1]; dcX[1] = r; dcY[1] = yr;
                left[done + i] = std::tanh(yl * mg * 2.5f) * 0.9f;
                right[done + i] = std::tanh(yr * mg * 2.5f) * 0.9f;
            }
            done += n;
        }
    }

private:
    Chunk makeChunk(int n)
    {
        Chunk c;
        c.n = n;
        c.sr = sr;

        // LFO (global)
        lfoPhase += p.lfoRate * (float) n / sr;
        if (lfoPhase >= 1.f)
        {
            lfoPhase -= std::floor(lfoPhase);
            shValue = rng.nextFloat() * 2.f - 1.f;
        }
        switch (p.lfoWave)
        {
            case 0:  c.lfo = lfoPhase < 0.5f ? 4.f * lfoPhase - 1.f : 3.f - 4.f * lfoPhase; break;
            case 1:  c.lfo = lfoPhase < 0.5f ? 1.f : -1.f; break;
            case 2:  c.lfo = 2.f * lfoPhase - 1.f; break;
            default: c.lfo = shValue; break;
        }
        c.lfoDelay = p.lfoDelay;

        const float dco = sDco.skip(n);
        c.vibSemis = juce::jlimit(0.f, 1.f, dco + modWheel) * 2.f;
        c.bendSemis = bendNorm * p.bendRange;
        c.tuneSemis = sTune.skip(n) / 100.f;
        c.rangeSemis = (float) (p.range - 1) * 12.f;
        c.portaTau = p.porta / 3.f;
        c.pulse = p.pulse;
        c.saw = p.saw;
        c.sub = sSub.skip(n);
        c.noise = sNoise.skip(n);
        c.pwmMode = p.pwmMode;
        c.pwm = sPwm.skip(n);
        c.hpf = p.hpf;
        c.cutoff = sCutoff.skip(n);
        c.res = sRes.skip(n);
        c.vcfEnv = sVcfEnv.skip(n);
        c.vcfLfo = sVcfLfo.skip(n);
        c.vcfKey = p.vcfKey;
        c.level = sLevel.skip(n);

        auto rateAttack = [&](float t) { return 1.f - std::exp(-1.466f / (juce::jmax(t, 0.0005f) * sr)); };
        auto rateDecay  = [&](float t) { return 1.f - std::exp(-4.6f / (juce::jmax(t, 0.0005f) * sr)); };

        c.fS = p.fS;
        c.fAc = rateAttack(p.fA);
        c.fDc = rateDecay(p.fD);
        c.fRc = rateDecay(p.fR);
        if (p.vcaMode == 0)
        {
            c.aS = p.aS;
            c.aAc = rateAttack(p.aA);
            c.aDc = rateDecay(p.aD);
            c.aRc = rateDecay(p.aR);
        }
        else // gate
        {
            c.aS = 1.f;
            c.aAc = rateAttack(0.002f);
            c.aDc = rateDecay(0.01f);
            c.aRc = rateDecay(0.004f);
        }
        return c;
    }

    //----------------------------------------------------------------------
    Voice* findVoiceForNote(int note)
    {
        for (auto& v : voices)
            if (v.note == note && v.gate && v.active())
                return &v;
        return nullptr;
    }

    Voice* allocateVoice()
    {
        for (auto& v : voices)
            if (!v.active())
                return &v;

        // steal: quietest releasing voice, otherwise oldest
        Voice* best = nullptr;
        for (auto& v : voices)
            if (v.aEnv.stage == Envelope::Release && (best == nullptr || v.aEnv.value < best->aEnv.value))
                best = &v;
        if (best != nullptr)
            return best;

        best = &voices[0];
        for (auto& v : voices)
            if (v.age < best->age)
                best = &v;
        return best;
    }

    void noteOn(int note, float vel)
    {
        const float glideFrom = (p.porta > 0.f && lastNote >= 0) ? (float) lastNote : (float) note;

        if (p.voiceMode == 0) // poly
        {
            Voice* v = findVoiceForNote(note);
            if (v == nullptr)
                v = allocateVoice();
            v->start(note, vel, p.velocity, glideFrom, true, ++ageCounter);
        }
        else // mono / legato
        {
            const bool wasHeld = !noteStack.empty();
            noteStack.erase(std::remove(noteStack.begin(), noteStack.end(), note), noteStack.end());
            noteStack.push_back(note);

            Voice& v = voices[0];
            for (size_t i = 1; i < voices.size(); ++i)
                if (voices[i].active()) voices[i].release();

            if (p.voiceMode == 2 && wasHeld && v.gate && v.active())
            {
                v.note = note;
                v.targetNote = (float) note;
                v.velGain = 1.f - p.velocity * (1.f - vel);
            }
            else
            {
                v.start(note, vel, p.velocity, glideFrom, true, ++ageCounter);
            }
        }
        lastNote = note;
    }

    void noteOff(int note)
    {
        if (p.voiceMode == 0)
        {
            for (auto& v : voices)
            {
                if (v.note == note && v.gate)
                {
                    if (pedal) v.sustained = true;
                    else v.release();
                }
            }
        }
        else
        {
            noteStack.erase(std::remove(noteStack.begin(), noteStack.end(), note), noteStack.end());
            Voice& v = voices[0];
            if (noteStack.empty())
            {
                v.release();
            }
            else if (v.note == note)
            {
                const int prev = noteStack.back();
                v.note = prev;
                v.targetNote = (float) prev;
                lastNote = prev;
                if (p.voiceMode == 1)
                {
                    v.fEnv.gateOn();
                    v.aEnv.gateOn();
                }
            }
        }
    }

    void setPedal(bool down)
    {
        pedal = down;
        if (!down)
            for (auto& v : voices)
                if (v.sustained)
                    v.release();
    }

    void allNotesOff(bool immediate)
    {
        noteStack.clear();
        for (auto& v : voices)
        {
            if (immediate) { v.fEnv.kill(); v.aEnv.kill(); v.gate = false; }
            else if (v.active()) v.release();
        }
    }

    //----------------------------------------------------------------------
    float sr = 44100.f;
    SynthParams p;
    std::array<Voice, kNumVoices> voices;
    Chorus chorus;
    juce::Random rng { 12345 };

    juce::SmoothedValue<float> sCutoff, sRes, sVcfEnv, sVcfLfo, sPwm, sSub, sNoise, sLevel, sDco, sTune, masterSm;

    float dcX[2] = { 0.f, 0.f }, dcY[2] = { 0.f, 0.f };
    float lfoPhase = 0.f, shValue = 0.f;
    float modWheel = 0.f, bendNorm = 0.f;
    bool  pedal = false;
    int   lastNote = -1;
    uint64_t ageCounter = 0;
    std::vector<int> noteStack;
};

} // namespace aj
