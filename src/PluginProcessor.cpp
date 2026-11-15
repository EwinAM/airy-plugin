#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

// ---------- Biquad (RBJ cookbook) ----------
void Biquad::highShelf (double fs, double f, double dB, double S)
{
    double A = std::pow (10.0, dB / 40.0), w = MathConstants<double>::twoPi * f / fs;
    double cs = std::cos (w), sn = std::sin (w);
    double al = sn / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
    double sa = 2.0 * std::sqrt (A) * al;
    set (A * ((A + 1) + (A - 1) * cs + sa), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - sa),
         (A + 1) - (A - 1) * cs + sa, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - sa);
}
void Biquad::peak (double fs, double f, double dB, double Q)
{
    double A = std::pow (10.0, dB / 40.0), w = MathConstants<double>::twoPi * f / fs, al = std::sin (w) / (2 * Q), cs = std::cos (w);
    set (1 + al * A, -2 * cs, 1 - al * A, 1 + al / A, -2 * cs, 1 - al / A);
}
void Biquad::highPass (double fs, double f, double Q)
{
    double w = MathConstants<double>::twoPi * f / fs, al = std::sin (w) / (2 * Q), cs = std::cos (w);
    set ((1 + cs) / 2, -(1 + cs), (1 + cs) / 2, 1 + al, -2 * cs, 1 - al);
}
void Biquad::lowPass (double fs, double f, double Q)
{
    double w = MathConstants<double>::twoPi * f / fs, al = std::sin (w) / (2 * Q), cs = std::cos (w);
    set ((1 - cs) / 2, 1 - cs, (1 - cs) / 2, 1 + al, -2 * cs, 1 - al);
}
void Biquad::bandPass (double fs, double f, double Q)
{
    double w = MathConstants<double>::twoPi * f / fs, al = std::sin (w) / (2 * Q), cs = std::cos (w);
    set (al, 0, -al, 1 + al, -2 * cs, 1 - al);
}
double Biquad::magnitudeDb (double fs, double f) const
{
    std::complex<double> z = std::polar (1.0, -MathConstants<double>::twoPi * f / fs);
    auto num = b0 + b1 * z + b2 * z * z, den = 1.0 + a1 * z + a2 * z * z;
    return 20.0 * std::log10 (std::abs (num / den) + 1e-12);
}

float AiryProcessor::decorrelate (float x)
{
    // Three nested Schroeder allpasses with prime-ish delays
    for (int k = 0; k < 3; ++k)
    {
        auto& d = ap[k];
        const int len = apLen[k];
        float z = d[(size_t) apPos[k]];
        float y = -0.6f * x + z;
        d[(size_t) apPos[k]] = x + 0.6f * y;
        apPos[k] = (apPos[k] + 1) % len;
        x = y;
    }
    return x;
}

// ---------- Parameters ----------
AudioProcessorValueTreeState::ParameterLayout AiryProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> v;
    auto pct = [] (float x, int) { return String (roundToInt (x)) + "%"; };
    auto khz = [] (float x, int) { return String (x / 1000.0f, 1) + " kHz"; };
    auto db  = [] (float x, int) { return (x > 0 ? "+" : "") + String (x, 1) + " dB"; };
    auto add = [&] (const char* id, const char* name, NormalisableRange<float> r, float def, std::function<String (float, int)> fmt)
    {
        v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
            AudioParameterFloatAttributes().withStringFromValueFunction (fmt)));
    };
    NormalisableRange<float> fr (8000.f, 20000.f); fr.setSkewForCentre (12000.f);
    add ("freq", "Air Freq", fr, 12000.f, khz);
    add ("air", "Air", { 0.f, 12.f }, 5.f, db);
    add ("tone", "Tone", { -100.f, 100.f }, 0.f, pct);
    add ("drive", "Drive", { 0.f, 100.f }, 25.f, pct);
    add ("sparkle", "Sparkle", { 0.f, 100.f }, 40.f, pct);
    NormalisableRange<float> fo (2000.f, 10000.f); fo.setSkewForCentre (5000.f);
    add ("focus", "Focus", fo, 5000.f, khz);
    NormalisableRange<float> xo (1000.f, 10000.f); xo.setSkewForCentre (4000.f);
    add ("from", "Width From", xo, 4000.f, khz);
    add ("width", "Width", { 0.f, 200.f }, 130.f, pct);
    add ("mono", "Mono Low", { 0.f, 100.f }, 0.f, pct);
    add ("deess", "De-ess", { 0.f, 100.f }, 40.f, pct);
    add ("mix", "Mix", { 0.f, 100.f }, 100.f, pct);
    add ("out", "Output", { -12.f, 12.f }, 0.f, db);
    v.push_back (std::make_unique<AudioParameterBool> (ParameterID { "dynamic", 2 }, "Dynamic Air", true));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Saturation", StringArray { "Tube", "Tape" }, 0));
    v.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));
    return { v.begin(), v.end() };
}

AiryProcessor::AiryProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AIRY", createLayout())
{
    for (auto& s : scopeL) s = 0; for (auto& s : scopeR) s = 0;
}

bool AiryProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto o = l.getMainOutputChannelSet(), in = l.getMainInputChannelSet();
    if (o != AudioChannelSet::mono() && o != AudioChannelSet::stereo()) return false;
    if (in == o) return true;
    return in == AudioChannelSet::mono() && o == AudioChannelSet::stereo();
}

void AiryProcessor::prepareToPlay (double s, int)
{
    sr = s;
    for (auto* b : { &shelf, &tone, &exHP1, &exHP2, &exPost, &dsHP, &dsBP, &xoLP1, &xoLP2, &xoHP1, &xoHP2 }) b->reset();
    dsEnv[0] = dsEnv[1] = dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0;
    cache = {};
    const double ms[3] = { 3.1, 5.3, 8.9 };
    for (int k = 0; k < 3; ++k)
    {
        apLen[k] = jmax (1, (int) (ms[k] * 0.001 * sr));
        ap[k].assign ((size_t) apLen[k], 0.0f);
        apPos[k] = 0;
    }
    dsHP.highPass (sr, 4500.0);
    dynHP.highPass (sr, jmin (5000.0, sr * 0.45)); dynHP.reset(); dynHF = dynFB = 0; dynGain = 1.0f;
    dsBP.bandPass (sr, 6500.0, 1.2);
    for (auto* sv : { &sMix, &sOut, &sWidth, &sMono, &sSpk }) sv->reset (sr, 0.03);
    sMix.setCurrentAndTargetValue (p ("mix") / 100.f);
    sOut.setCurrentAndTargetValue (Decibels::decibelsToGain (p ("out")));
    sWidth.setCurrentAndTargetValue (p ("width") / 100.f);
    sMono.setCurrentAndTargetValue (p ("mono") / 100.f);
    sSpk.setCurrentAndTargetValue (p ("sparkle") / 100.f);
    updateFilters();
}

void AiryProcessor::updateFilters()
{
    const double ny = sr * 0.45;
    float f = p ("freq"), a = p ("air"), t = p ("tone"), fo = p ("focus"), xo = p ("from");
    if (f != cache.f || a != cache.a) { shelf.highShelf (sr, jmin ((double) f, ny), a, 0.7); cache.f = f; cache.a = a; }
    if (t != cache.t) { tone.peak (sr, jmin (3500.0, ny), t / 100.0 * 4.0, 0.8); cache.t = t; }
    if (fo != cache.fo) { exHP1.highPass (sr, jmin ((double) fo, ny)); exHP2.highPass (sr, jmin ((double) fo, ny)); exPost.highPass (sr, jmin (fo * 0.8, ny)); cache.fo = fo; }
    if (xo != cache.xo) { xoHP1.highPass (sr, jmin ((double) xo, ny)); xoHP2.highPass (sr, jmin ((double) xo, ny)); cache.xo = xo; }
}

void AiryProcessor::processBlock (AudioBuffer<float>& buf, MidiBuffer&)
{
    ScopedNoDenormals nd;
    const int n = buf.getNumSamples(), nch = jmin (2, buf.getNumChannels());
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buf.clear (c, 0, n);
    if (getTotalNumInputChannels() == 1 && buf.getNumChannels() > 1) buf.copyFrom (1, 0, buf, 0, 0, n);

    const bool byp = p ("bypass") > 0.5f;
    updateFilters();
    sMix.setTargetValue (p ("mix") / 100.f);
    sOut.setTargetValue (Decibels::decibelsToGain (p ("out")));
    sWidth.setTargetValue (p ("width") / 100.f);
    sMono.setTargetValue (p ("mono") / 100.f);
    sSpk.setTargetValue (p ("sparkle") / 100.f);

    const float drive = 1.0f + p ("drive") / 100.f * 19.f;
    const float exNorm = 1.0f / std::sqrt (drive);
    const bool tape = p ("mode") > 0.5f;
    const float dsAmt = p ("deess") / 100.f;
    const float dsThr = Decibels::decibelsToGain (-18.f - dsAmt * 30.f);
    const float atk = std::exp (-1.0f / (0.001f * (float) sr)), rel = std::exp (-1.0f / (0.06f * (float) sr));
    const float bias = 0.25f, biasOff = std::tanh (bias);
    float* L = buf.getWritePointer (0);
    float* R = nch > 1 ? buf.getWritePointer (1) : nullptr;
    float sumLR = 0, sumL2 = 0, sumR2 = 0, grMax = 0, exPeak = 0, dynMin = 1.0f;
    const bool dynOn = p ("dynamic") > 0.5f;
    const float dAtk = std::exp (-1.0f / (0.002f * (float) sr)), dRel = std::exp (-1.0f / (0.12f * (float) sr));
    const float gAtk = std::exp (-1.0f / (0.003f * (float) sr)), gRel = std::exp (-1.0f / (0.15f * (float) sr));

    for (int i = 0; i < n; ++i)
    {
        const float mix = sMix.getNextValue(), og = sOut.getNextValue(), wd = sWidth.getNextValue(),
                    mo = sMono.getNextValue(), spk = sSpk.getNextValue();
        float in[2] = { L[i], R ? R[i] : L[i] };
        float wet[2];

        // De-ess detection (linked)
        // Dynamic air detector: how bright the vocal is right now (top band vs whole)
        {
            const float mono = 0.5f * (in[0] + in[1]);
            const float hf = std::abs (dynHP.process (mono, 0)), fb = std::abs (mono);
            dynHF = hf > dynHF ? dAtk * dynHF + (1 - dAtk) * hf : dRel * dynHF + (1 - dRel) * hf;
            dynFB = fb > dynFB ? dAtk * dynFB + (1 - dAtk) * fb : dRel * dynFB + (1 - dRel) * fb;
            float target = 1.0f;
            if (dynOn && dynFB > 1.0e-4f)
            {
                const float ratioDb = 20.0f * std::log10 ((dynHF + 1.0e-9f) / dynFB);
                const float loudDb = 20.0f * std::log10 (dynFB);
                const float excess = jmax (0.0f, ratioDb + 16.0f) / 10.0f + jmax (0.0f, loudDb + 14.0f) / 14.0f;
                target = 1.0f - jlimit (0.0f, 0.65f, excess * 0.65f);
            }
            dynGain = target < dynGain ? gAtk * dynGain + (1 - gAtk) * target : gRel * dynGain + (1 - gRel) * target;
            dynMin = jmin (dynMin, dynGain);
        }
        float det = 0;
        for (int c = 0; c < nch; ++c) det = jmax (det, std::abs (dsBP.process (in[c], c)));
        for (int c = 0; c < nch; ++c)
        {
            float x = in[c];
            // De-esser: duck only the high band
            float& e = dsEnv[c];
            e = det > e ? atk * e + (1 - atk) * det : rel * e + (1 - rel) * det;
            float g = 1.0f;
            if (dsAmt > 0.001f && e > dsThr)
                g = jlimit (0.25f, 1.0f, std::pow (dsThr / e, 0.75f));
            float hb = dsHP.process (x, c);
            x -= hb * (1.0f - g);
            grMax = jmax (grMax, 1.0f - g);

            // Sparkle exciter: saturate the top band, add back
            float h = exHP2.process (exHP1.process (x, c), c);
            float d = h * drive, s;
            if (tape) s = d / (1.0f + std::abs (d)) + 0.12f * d / (1.0f + d * d);
            else      s = std::tanh (d + bias) - biasOff;
            s = exPost.process (s * exNorm, c);
            // DC blocker
            float dc = s - dcX[c] + 0.995f * dcY[c]; dcX[c] = s; dcY[c] = dc;
            exPeak = jmax (exPeak, std::abs (dc * spk));
            x += dc * spk * 0.9f;

            // Air shelf + presence tone
            { const float sh = shelf.process (x, c); x += (sh - x) * dynGain; }
            x = tone.process (x, c);
            wet[c] = x;
        }
        if (nch == 1) wet[1] = wet[0];

        if (nch > 1)
        {
            // Mid/Side imager: widen the top, optionally mono the bottom
            float m = 0.5f * (wet[0] + wet[1]), sd = 0.5f * (wet[0] - wet[1]);
            float hiS = xoHP1.process (sd, 0);
            float loS = sd - hiS;
            // Artificial width: decorrelate the top of the Mid into a new Side.
            // Works on mono sources; L+R cancels it, so mono sum stays clean.
            float hiM = xoHP2.process (m, 0);
            float syn = decorrelate (hiM);
            float synAmt = jmax (0.0f, wd - 1.0f) * 0.9f;
            sd = loS * (1.0f - mo) + hiS * jmin (wd, 1.0f + (wd - 1.0f) * 0.5f) + syn * synAmt;
            wet[0] = m + sd; wet[1] = m - sd;
        }

        float o[2];
        for (int c = 0; c < 2; ++c) o[c] = byp ? in[c] : (in[c] + (wet[c] - in[c]) * mix) * og;
        L[i] = o[0];
        if (R) R[i] = o[1];

        sumLR += o[0] * o[1]; sumL2 += o[0] * o[0]; sumR2 += o[1] * o[1];
        if ((i & 1) == 0)
        {
            int w = scopeWrite.load (std::memory_order_relaxed);
            scopeL[(size_t) w].store (o[0], std::memory_order_relaxed);
            scopeR[(size_t) w].store (o[1], std::memory_order_relaxed);
            scopeWrite.store ((w + 1) % scopeSize, std::memory_order_relaxed);
        }
    }

    corrNum = 0.8f * corrNum + 0.2f * sumLR;
    corrDen1 = 0.8f * corrDen1 + 0.2f * sumL2;
    corrDen2 = 0.8f * corrDen2 + 0.2f * sumR2;
    float den = std::sqrt (corrDen1 * corrDen2);
    correlation.store (den > 1e-7f ? jlimit (-1.0f, 1.0f, corrNum / den) : 1.0f);
    deessGR.store (byp ? 0.0f : grMax);
    sparkleLevel.store (byp ? 0.0f : exPeak);
    airDyn.store (byp ? 1.0f : dynMin);
}

void AiryProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto x = apvts.copyState().createXml()) copyXmlToBinary (*x, d);
}
void AiryProcessor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size))
        if (x->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*x));
}

AudioProcessorEditor* AiryProcessor::createEditor() { return new AiryEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AiryProcessor(); }
