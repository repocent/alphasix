// Offline render test: drives the processor with MIDI, writes WAV files for analysis.
#include "../Source/PluginProcessor.h"

#include <cstdio>
#include <cstdint>
#include <fstream>
#include <juce_graphics/juce_graphics.h>

static void writeWav(const std::string& path, const std::vector<float>& l, const std::vector<float>& r, int sr)
{
    std::ofstream f(path, std::ios::binary);
    const uint32_t n = (uint32_t) l.size();
    const uint32_t dataBytes = n * 4;
    auto w32 = [&](uint32_t v) { f.write((const char*) &v, 4); };
    auto w16 = [&](uint16_t v) { f.write((const char*) &v, 2); };
    f.write("RIFF", 4); w32(36 + dataBytes); f.write("WAVEfmt ", 8);
    w32(16); w16(1); w16(2); w32((uint32_t) sr); w32((uint32_t) sr * 4); w16(4); w16(16);
    f.write("data", 4); w32(dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        const int16_t a = (int16_t) juce::jlimit(-32767.f, 32767.f, l[i] * 32767.f);
        const int16_t b = (int16_t) juce::jlimit(-32767.f, 32767.f, r[i] * 32767.f);
        f.write((const char*) &a, 2);
        f.write((const char*) &b, 2);
    }
}

struct Ev { double t; int note; bool on; float vel; };

static bool renderPreset(AlphaSixProcessor& p, int program, const std::vector<Ev>& evs, double seconds,
                         const std::string& out, int sr, int block)
{
    p.setCurrentProgram(program);
    p.prepareToPlay(sr, block);

    const int total = (int) (seconds * sr);
    std::vector<float> L, R;
    L.reserve((size_t) total); R.reserve((size_t) total);

    juce::AudioBuffer<float> buf(2, block);
    size_t evIdx = 0;
    bool bad = false;
    for (int pos = 0; pos < total; pos += block)
    {
        juce::MidiBuffer midi;
        const int n = juce::jmin(block, total - pos);
        while (evIdx < evs.size() && (int) (evs[evIdx].t * sr) < pos + n)
        {
            const auto& e = evs[evIdx++];
            const int off = juce::jmax(0, (int) (e.t * sr) - pos);
            midi.addEvent(e.on ? juce::MidiMessage::noteOn(1, e.note, e.vel)
                               : juce::MidiMessage::noteOff(1, e.note), off);
        }
        buf.setSize(2, n, false, false, true);
        p.processBlock(buf, midi);
        for (int i = 0; i < n; ++i)
        {
            const float a = buf.getSample(0, i), b = buf.getSample(1, i);
            if (!std::isfinite(a) || !std::isfinite(b) || std::abs(a) > 1.5f || std::abs(b) > 1.5f) bad = true;
            L.push_back(a); R.push_back(b);
        }
    }
    writeWav(out, L, R, sr);
    float peak = 0.f; double rms = 0.0;
    for (size_t i = 0; i < L.size(); ++i) { peak = std::max(peak, std::abs(L[i])); rms += L[i] * L[i]; }
    rms = std::sqrt(rms / (double) L.size());
    std::printf("%-14s peak=%.3f rms=%.4f %s -> %s\n", p.getProgramName(program).toRawUTF8(), peak, rms,
                bad ? "BAD(NaN/clip)" : "ok", out.c_str());
    return !bad;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init; // processors may construct GUI-related statics
    const std::string dir = argc > 1 ? argv[1] : ".";
    const int sr = 48000;

    AlphaSixProcessor proc;
    bool ok = true;

    // chord + held note + release for each preset, odd block size to stress chunk splitting
    std::vector<Ev> chord = {
        { 0.10, 48, true, 0.8f }, { 0.10, 55, true, 0.8f }, { 0.10, 60, true, 0.8f }, { 0.10, 64, true, 0.8f },
        { 0.10, 67, true, 0.8f }, { 0.10, 72, true, 0.8f }, { 0.10, 76, true, 0.8f }, // 7 notes -> voice stealing
        { 2.00, 48, false, 0 }, { 2.00, 55, false, 0 }, { 2.00, 60, false, 0 }, { 2.00, 64, false, 0 },
        { 2.00, 67, false, 0 }, { 2.00, 72, false, 0 }, { 2.00, 76, false, 0 } };

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        ok &= renderPreset(proc, i, chord, 4.0, dir + "/preset_" + std::to_string(i) + ".wav", sr, 437);

    // Mono / legato melody on "Square Lead"
    std::vector<Ev> mel = {
        { 0.1, 60, true, 0.9f }, { 0.6, 64, true, 0.9f }, { 0.9, 64, false, 0 }, { 1.1, 60, false, 0 },
        { 1.3, 67, true, 0.7f }, { 2.0, 67, false, 0 } };
    ok &= renderPreset(proc, 4, mel, 3.0, dir + "/lead_mono.wav", sr, 256);

    // Extreme settings: max resonance, cutoff sweep via params, different sample rate
    for (const char* pid : { "res", "cutoff" })
        if (auto* prm = proc.apvts.getParameter(pid)) prm->setValueNotifyingHost(1.0f);
    ok &= renderPreset(proc, 0, chord, 2.0, dir + "/extreme_res.wav", 44100, 64);
    {
        // renderPreset resets to program -> force the extremes again after reset
        proc.setCurrentProgram(0);
        for (const char* pid : { "res", "cutoff" })
            if (auto* prm = proc.apvts.getParameter(pid)) prm->setValueNotifyingHost(1.0f);
        proc.prepareToPlay(96000, 128);
        juce::AudioBuffer<float> b(2, 128);
        juce::MidiBuffer m;
        m.addEvent(juce::MidiMessage::noteOn(1, 96, 1.0f), 0);
        float peak = 0.f; bool bad = false;
        for (int blk = 0; blk < 400; ++blk)
        {
            proc.processBlock(b, m);
            m.clear();
            for (int i = 0; i < 128; ++i)
            {
                const float a = b.getSample(0, i);
                if (!std::isfinite(a)) bad = true;
                peak = std::max(peak, std::abs(a));
            }
        }
        std::printf("96k max-res high note: peak=%.3f %s\n", peak, bad ? "BAD" : "ok");
        ok &= !bad;
    }

    // State round trip
    {
        juce::MemoryBlock mb;
        proc.setCurrentProgram(3);
        proc.getStateInformation(mb);
        AlphaSixProcessor other;
        other.setStateInformation(mb.getData(), (int) mb.getSize());
        const bool same = std::abs(other.apvts.getRawParameterValue("cutoff")->load()
                                   - proc.apvts.getRawParameterValue("cutoff")->load()) < 1e-6f
                          && other.getCurrentProgram() == 3;
        std::printf("state round trip: %s\n", same ? "ok" : "FAILED");
        ok &= same;
    }

    // Headless GUI snapshot
    {
        proc.setCurrentProgram(1);
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.5f);
        juce::File f(juce::String(dir) + "/editor.png");
        f.deleteFile();
        juce::FileOutputStream os(f);
        juce::PNGImageFormat png;
        const bool wrote = os.openedOk() && png.writeImageToStream(img, os);
        std::printf("editor snapshot %dx%d: %s\n", ed->getWidth(), ed->getHeight(), wrote ? "ok" : "FAILED");
        ok &= wrote;
    }

    std::printf(ok ? "ALL OK\n" : "FAILURES\n");
    return ok ? 0 : 1;
}
