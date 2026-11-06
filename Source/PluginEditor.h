#pragma once

#include "Common.h"
#include "PluginProcessor.h"

namespace ajui
{
using APVTS = juce::AudioProcessorValueTreeState;

/** Dark panel look with thin Juno-style vertical faders. */
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                          float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle,
                          juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                      int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;
};

class Fader : public juce::Component
{
public:
    Fader(APVTS& state, const juce::String& paramId, const juce::String& name, juce::Colour accent,
          int decimals = 2, const juce::String& suffix = {});
    void resized() override;

private:
    juce::Label label;
    juce::Slider slider;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

class Chooser : public juce::Component
{
public:
    Chooser(APVTS& state, const juce::String& paramId, const juce::String& name);
    void resized() override;

private:
    juce::Label label;
    juce::ComboBox box;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

class Toggle : public juce::Component
{
public:
    Toggle(APVTS& state, const juce::String& paramId, const juce::String& name, juce::Colour accent);
    void resized() override { button.setBounds(getLocalBounds()); }

private:
    juce::ToggleButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};
} // namespace ajui

class AlphaSixEditor : public juce::AudioProcessorEditor
{
public:
    explicit AlphaSixEditor(AlphaSixProcessor&);
    ~AlphaSixEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Section
    {
        juce::String title;
        juce::Colour colour;
        std::vector<ajui::Fader*> faders;
        std::vector<juce::Component*> extras;
        juce::Rectangle<int> bounds;
    };

    ajui::Fader* addFader(const char* pid, const char* name, juce::Colour c, int decimals = 2,
                          const juce::String& suffix = {});
    ajui::Chooser* addChooser(const char* pid, const char* name);
    ajui::Toggle* addToggle(const char* pid, const char* name, juce::Colour c);
    static int sectionWidth(const Section& s);

    AlphaSixProcessor& proc;
    ajui::LookAndFeel laf;

    std::vector<std::unique_ptr<ajui::Fader>> faders;
    std::vector<std::unique_ptr<ajui::Chooser>> choosers;
    std::vector<std::unique_ptr<ajui::Toggle>> toggles;
    std::vector<Section> sections;

    juce::ComboBox programBox;
    juce::Label programLabel;

    static constexpr int faderW = 46;
    static constexpr int extraH = 40;
    static constexpr int headerH = 56;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlphaSixEditor)
};
