#include "PluginEditor.h"

using juce::Colour;

namespace
{
const Colour kBg      { 0xff17191c };
const Colour kPanel   { 0xff23272c };
const Colour kText    { 0xffd8dce0 };
const Colour kDim     { 0xff8b929a };
} // namespace

namespace ajui
{
//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, kText);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, kText);
    setColour(juce::ComboBox::textColourId, kText);
    setColour(juce::ComboBox::backgroundColourId, Colour(0xff111316));
    setColour(juce::ComboBox::outlineColourId, Colour(0xff3a4048));
    setColour(juce::ComboBox::arrowColourId, kDim);
    setColour(juce::PopupMenu::backgroundColourId, Colour(0xff1b1e22));
    setColour(juce::PopupMenu::textColourId, kText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Colour(0xff3a4048));
}

juce::Font LookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions(12.f));
}

juce::Font LookAndFeel::getLabelFont(juce::Label& l)
{
    return l.getFont();
}

void LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                   float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cx = (float) x + (float) width * 0.5f;
    const float top = (float) y + 6.f, bottom = (float) (y + height) - 6.f;
    const auto accent = s.findColour(juce::Slider::trackColourId);

    // ticks
    g.setColour(Colour(0xff3a4048));
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = top + (bottom - top) * (float) i / 10.f;
        g.drawHorizontalLine((int) ty, cx - 11.f, cx - 7.f);
        g.drawHorizontalLine((int) ty, cx + 7.f, cx + 11.f);
    }

    // groove
    g.setColour(Colour(0xff0c0d0f));
    g.fillRoundedRectangle(cx - 2.5f, top, 5.f, bottom - top, 2.5f);

    // filled part (from centre for bipolar, from bottom otherwise)
    const double lo = s.getMinimum(), hi = s.getMaximum();
    const bool bipolar = lo < 0.0 && hi > 0.0;
    float from = bottom;
    if (bipolar)
        from = (float) (bottom - (bottom - top) * ((0.0 - lo) / (hi - lo)));
    g.setColour(accent.withAlpha(0.85f));
    g.fillRoundedRectangle(cx - 1.5f, juce::jmin(from, sliderPos), 3.f, std::abs(from - sliderPos), 1.5f);

    // cap
    const juce::Rectangle<float> cap(cx - 13.f, sliderPos - 7.f, 26.f, 14.f);
    juce::ColourGradient gr(Colour(0xffe9ecef), cap.getX(), cap.getY(), Colour(0xff9aa1a8), cap.getX(), cap.getBottom(), false);
    g.setGradientFill(gr);
    g.fillRoundedRectangle(cap, 2.5f);
    g.setColour(Colour(0xff111316));
    g.drawRoundedRectangle(cap, 2.5f, 1.f);
    g.setColour(accent);
    g.fillRect(cap.getX() + 3.f, cap.getCentreY() - 1.f, cap.getWidth() - 6.f, 2.f);
}

void LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced(2.f, 4.f);
    const bool on = b.getToggleState();
    const auto accent = b.findColour(juce::ToggleButton::tickColourId);
    g.setColour(on ? accent.withAlpha(0.9f) : Colour(0xff111316));
    g.fillRoundedRectangle(r, 4.f);
    g.setColour(highlighted ? kText : Colour(0xff3a4048));
    g.drawRoundedRectangle(r, 4.f, 1.f);
    g.setColour(on ? Colour(0xff101214) : kDim);
    g.setFont(juce::Font(juce::FontOptions(12.f, juce::Font::bold)));
    g.drawText(b.getButtonText(), r.toNearestInt(), juce::Justification::centred);
}

void LookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0.f, 0.f, (float) w, (float) h).reduced(0.5f);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(r, 4.f);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(r, 4.f, 1.f);
    juce::Path p;
    const float ax = (float) w - 13.f, ay = (float) h * 0.5f;
    p.addTriangle(ax - 3.5f, ay - 2.f, ax + 3.5f, ay - 2.f, ax, ay + 3.f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(p);
}

//==============================================================================
Fader::Fader(APVTS& state, const juce::String& paramId, const juce::String& name, Colour accent,
             int decimals, const juce::String& suffix)
{
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(label);

    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 46, 16);
    slider.setNumDecimalPlacesToDisplay(decimals);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::trackColourId, accent);
    addAndMakeVisible(slider);

    attachment = std::make_unique<APVTS::SliderAttachment>(state, paramId, slider);

    // The attachment installs its own text formatter; override it with a compact one.
    slider.textFromValueFunction = [decimals, suffix](double v) { (void) suffix; return juce::String(v, decimals); };
    slider.updateText();
}

void Fader::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromTop(22));
    slider.setBounds(r);
}

Chooser::Chooser(APVTS& state, const juce::String& paramId, const juce::String& name)
{
    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(label);

    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(paramId)))
        for (int i = 0; i < p->choices.size(); ++i)
            box.addItem(p->choices[i], i + 1);
    addAndMakeVisible(box);
    attachment = std::make_unique<APVTS::ComboBoxAttachment>(state, paramId, box);
}

void Chooser::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromTop(14));
    box.setBounds(r.reduced(0, 1));
}

Toggle::Toggle(APVTS& state, const juce::String& paramId, const juce::String& name, Colour accent)
{
    button.setButtonText(name);
    button.setColour(juce::ToggleButton::tickColourId, accent);
    addAndMakeVisible(button);
    attachment = std::make_unique<APVTS::ButtonAttachment>(state, paramId, button);
}
} // namespace ajui

//==============================================================================
AlphaSixEditor::AlphaSixEditor(AlphaSixProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&laf);
    using namespace aj;

    const Colour cLfo   { 0xffb48cf2 };
    const Colour cDco   { 0xff4fc3f7 };
    const Colour cHpf   { 0xff81c784 };
    const Colour cVcf   { 0xffffb74d };
    const Colour cEnv   { 0xffff8a65 };
    const Colour cVca   { 0xffe57373 };
    const Colour cOut   { 0xff4db6ac };

    sections.push_back({ "LFO", cLfo,
        { addFader(id::lfoRate, "RATE", cLfo, 2, " Hz"), addFader(id::lfoDelay, "DELAY", cLfo, 2, " s") },
        { addChooser(id::lfoWave, "WAVE") }, {} });

    sections.push_back({ "DCO", cDco,
        { addFader(id::dcoLfo, "VIB", cDco), addFader(id::dcoPwm, "PWM", cDco),
          addFader(id::dcoSub, "SUB", cDco), addFader(id::dcoNoise, "NOISE", cDco),
          addFader(id::detune, "TUNE", cDco, 0, " ct") },
        { addChooser(id::dcoRange, "RANGE"), addChooser(id::pwmMode, "PWM MODE"),
          addToggle(id::dcoPulse, "PULSE", cDco), addToggle(id::dcoSaw, "SAW", cDco) }, {} });

    sections.push_back({ "HPF", cHpf,
        { addFader(id::hpf, "HPF", cHpf, 0) }, {}, {} });

    sections.push_back({ "VCF", cVcf,
        { addFader(id::cutoff, "FREQ", cVcf), addFader(id::res, "RES", cVcf), addFader(id::vcfEnv, "ENV", cVcf),
          addFader(id::vcfLfo, "LFO", cVcf), addFader(id::vcfKey, "KEY", cVcf) }, {}, {} });

    sections.push_back({ "VCF ENV", cEnv,
        { addFader(id::fA, "A", cEnv), addFader(id::fD, "D", cEnv), addFader(id::fS, "S", cEnv),
          addFader(id::fR, "R", cEnv) }, {}, {} });

    sections.push_back({ "VCA", cVca,
        { addFader(id::aA, "A", cVca), addFader(id::aD, "D", cVca), addFader(id::aS, "S", cVca),
          addFader(id::aR, "R", cVca), addFader(id::level, "LEVEL", cVca) },
        { addChooser(id::vcaMode, "MODE") }, {} });

    sections.push_back({ "OUTPUT", cOut,
        { addFader(id::porta, "PORTA", cOut, 2, " s"), addFader(id::bend, "BEND", cOut, 0),
          addFader(id::velocity, "VEL", cOut), addFader(id::master, "VOL", cOut) },
        { addChooser(id::voiceMode, "VOICE"), addChooser(id::chorus, "CHORUS") }, {} });

    int total = 20;
    for (auto& s : sections) total += sectionWidth(s) + 6;
    int maxH = 0;
    (void) maxH;
    setSize(total, 470);

    programLabel.setText("PROGRAM", juce::dontSendNotification);
    programLabel.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    programLabel.setColour(juce::Label::textColourId, kDim);
    programLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(programLabel);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        programBox.addItem(proc.getProgramName(i), i + 1);
    programBox.setSelectedId(proc.getCurrentProgram() + 1, juce::dontSendNotification);
    programBox.onChange = [this] { proc.setCurrentProgram(programBox.getSelectedId() - 1); };
    addAndMakeVisible(programBox);
}

AlphaSixEditor::~AlphaSixEditor()
{
    setLookAndFeel(nullptr);
}

ajui::Fader* AlphaSixEditor::addFader(const char* pid, const char* name, Colour c, int decimals,
                                      const juce::String& suffix)
{
    faders.push_back(std::make_unique<ajui::Fader>(proc.apvts, pid, name, c, decimals, suffix));
    addAndMakeVisible(*faders.back());
    return faders.back().get();
}

ajui::Chooser* AlphaSixEditor::addChooser(const char* pid, const char* name)
{
    choosers.push_back(std::make_unique<ajui::Chooser>(proc.apvts, pid, name));
    addAndMakeVisible(*choosers.back());
    return choosers.back().get();
}

ajui::Toggle* AlphaSixEditor::addToggle(const char* pid, const char* name, Colour c)
{
    toggles.push_back(std::make_unique<ajui::Toggle>(proc.apvts, pid, name, c));
    addAndMakeVisible(*toggles.back());
    return toggles.back().get();
}

int AlphaSixEditor::sectionWidth(const Section& s)
{
    return juce::jmax((int) s.faders.size() * faderW, 92) + 16;
}

void AlphaSixEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBg);

    // header
    g.setColour(kText);
    g.setFont(juce::Font(juce::FontOptions(26.f, juce::Font::bold)));
    g.drawText("ALPHA SIX", 20, 8, 240, 30, juce::Justification::centredLeft);
    g.setColour(kDim);
    g.setFont(juce::Font(juce::FontOptions(11.f)));
    g.drawText("6-voice DCO polysynth  -  Alpha Juno style", 22, 36, 320, 14, juce::Justification::centredLeft);

    for (auto& s : sections)
    {
        auto b = s.bounds.toFloat();
        g.setColour(kPanel);
        g.fillRoundedRectangle(b, 6.f);
        auto head = b.withHeight(22.f);
        g.setColour(s.colour.withAlpha(0.9f));
        g.fillRoundedRectangle(head.withBottom(head.getBottom()), 6.f);
        g.fillRect(head.withTop(head.getY() + 10.f));
        g.setColour(Colour(0xff101214));
        g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
        g.drawText(s.title, head.toNearestInt(), juce::Justification::centred);
    }
}

void AlphaSixEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(headerH);

    programBox.setBounds(getWidth() - 10 - 190, 16, 190, 26);
    programLabel.setBounds(getWidth() - 10 - 190 - 90, 16, 84, 26);

    int x = area.getX();
    for (auto& s : sections)
    {
        const int w = sectionWidth(s);
        s.bounds = { x, area.getY(), w, area.getHeight() };

        auto r = s.bounds.reduced(8, 0);
        r.removeFromTop(26);
        r.removeFromBottom(8);

        auto bottom = r.removeFromBottom((int) s.extras.size() * extraH);
        for (auto* e : s.extras)
            e->setBounds(bottom.removeFromTop(extraH).reduced(2, 2));

        const int fw = faderW;
        int fx = r.getX() + (r.getWidth() - (int) s.faders.size() * fw) / 2;
        for (auto* f : s.faders)
        {
            f->setBounds(fx, r.getY(), fw, r.getHeight());
            fx += fw;
        }
        x += w + 6;
    }
}
