#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

// Simple biquad (TDF-II), coefficients via RBJ cookbook
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] {}, z2[2] {};
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    inline float process (float x, int ch)
    {
        double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    { b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0; a1 = na1 / na0; a2 = na2 / na0; }
    void highShelf (double fs, double f, double dB, double S = 0.8);
    void peak (double fs, double f, double dB, double Q);
    void highPass (double fs, double f, double Q = 0.7071);
    void lowPass (double fs, double f, double Q = 0.7071);
    void bandPass (double fs, double f, double Q);
    double magnitudeDb (double fs, double f) const;
};

class AiryProcessor : public juce::AudioProcessor
{
public:
    AiryProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Airy v2.2"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // For the editor: stereo scope ring buffer + meters
    static constexpr int scopeSize = 1024;
    std::array<std::atomic<float>, scopeSize> scopeL {}, scopeR {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<float> correlation { 1.0f }, deessGR { 0.0f }, sparkleLevel { 0.0f }, airDyn { 1.0f };
    double getSR() const { return sr; }

private:
    double sr = 44100.0;
    Biquad shelf, tone, exHP1, exHP2, exPost, dsHP, dsBP, xoLP1, xoLP2, xoHP1, xoHP2;
    float dsEnv[2] {}, dcX[2] {}, dcY[2] {};
    Biquad dynHP; float dynHF = 0, dynFB = 0, dynGain = 1.0f;
    float corrNum = 0, corrDen1 = 1e-9f, corrDen2 = 1e-9f;
    juce::SmoothedValue<float> sMix, sOut, sWidth, sMono, sSpk;
    struct Cache { float f = -1, a = -1, t = -1, fo = -1, xo = -1; } cache;
    void updateFilters();
    float decorrelate (float x);
    std::vector<float> ap[3];
    int apLen[3] { 1, 1, 1 }, apPos[3] {};
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AiryProcessor)
};
