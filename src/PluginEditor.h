#pragma once
#include "PluginProcessor.h"

// Airy v2.2 UI: FreqRoom-style hardware look.
// Dark display deck on top (Air, Sparkle, Space screens), white segment cards below
// with small-big-small 3D knobs, and an output strip. Designed at 690x636 and scaled.
namespace AiryUI
{
    constexpr int baseW = 690, baseH = 636;

    const juce::Colour deck     { 0xff141821 }, screenBg { 0xff0a0c10 }, screenLine { 0xff232934 };
    const juce::Colour txt      { 0xffeef1f5 }, dim      { 0xff8d97a8 };
    const juce::Colour air      { 0xff2fb8e6 }, airGlow  { 0xff62d8ff };
    const juce::Colour spk      { 0xffff7a3c }, spkGlow  { 0xffff8a5c }, tapeGlow { 0xffff4f8b };
    const juce::Colour spc      { 0xff1fcf9c }, spcGlow  { 0xff4ff0c0 };
    const juce::Colour outCol   { 0xff8a93a3 };
    const juce::Colour ink      { 0xff2c3038 }, label    { 0xff6c7380 };
}

class AiryLook : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
    juce::Font tipFont;
};

class Knob : public juce::Component
{
public:
    Knob (AiryProcessor&, const juce::String& paramId, const juce::String& label,
          juce::Colour accent, int dialSize, const juce::String& tip);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Slider slider;
    int dialSize;
private:
    juce::String label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class Screen : public juce::Component
{
public:
    enum Kind { Air, Sparkle, Space };
    Screen (AiryProcessor& p, Kind k) : proc (p), kind (k) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    float phase = 0, liveMix = 0, scopeK = 0, dynShown = 1, dsShown = 0;
private:
    AiryProcessor& proc;
    Kind kind;
    void drawAir (juce::Graphics&, juce::Rectangle<float>);
    void drawSparkle (juce::Graphics&, juce::Rectangle<float>);
    void drawSpace (juce::Graphics&, juce::Rectangle<float>);
};

class AiryPanel : public juce::Component, public juce::TooltipClient, private juce::Timer
{
public:
    explicit AiryPanel (AiryProcessor&);
    ~AiryPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String getTooltip() override;
    void setTipParent (juce::Component* c) { tipWindow.reset(); tipParent = c; setHints (hints); }

private:
    struct Switch { juce::Rectangle<float> a, b; juce::String la, lb, param, tipA, tipB; juce::Colour accent; };
    void timerCallback() override;
    void setHints (bool);
    void setParam (const char* id, float v);
    void drawSwitch (juce::Graphics&, const Switch&, bool second);
    void drawCard (juce::Graphics&, juce::Rectangle<float>);

    AiryProcessor& proc;
    AiryLook look;
    bool hints = true;
    std::unique_ptr<juce::TooltipWindow> tipWindow;
    juce::Component* tipParent = nullptr;

    Screen sAir, sSpk, sSpc;
    Knob kFreq, kAir, kTone, kDrive, kSpk, kFocus, kFrom, kWidth, kMono, kDeess, kMix, kOut;
    juce::Rectangle<float> segs[3], footer, hintBtn, powerBtn;
    Switch swAir, swSat;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AiryPanel)
};

// Resizable host window: scales the fixed-size panel to any size, keeping its proportions
class AiryEditor : public juce::AudioProcessorEditor
{
public:
    explicit AiryEditor (AiryProcessor&);
    void paint (juce::Graphics& g) override { g.fillAll (AiryUI::deck); }
    void resized() override;
private:
    AiryProcessor& proc;
    AiryPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AiryEditor)
};
