#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;
using namespace AiryUI;

// ======================= Fonts =======================
static Typeface::Ptr tf (int w)
{
    static Typeface::Ptr med  = Typeface::createSystemTypefaceFor (BinaryData::ManropeMedium_ttf,   BinaryData::ManropeMedium_ttfSize);
    static Typeface::Ptr semi = Typeface::createSystemTypefaceFor (BinaryData::ManropeSemiBold_ttf, BinaryData::ManropeSemiBold_ttfSize);
    static Typeface::Ptr bold = Typeface::createSystemTypefaceFor (BinaryData::ManropeBold_ttf,     BinaryData::ManropeBold_ttfSize);
    return w >= 700 ? bold : (w >= 600 ? semi : med);
}
static Font font (int weight, float h, float trackingPx = 0.0f)
{
    return Font (tf (weight)).withHeight (h).withExtraKerningFactor (trackingPx / h);
}
static Path rounded (Rectangle<float> r, float c) { Path p; p.addRoundedRectangle (r, c); return p; }

// ======================= Look and feel =======================
void AiryLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float u = jmin (b.getWidth(), b.getHeight()) / 100.0f;   // 100-unit design grid
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const Colour acc = s.findColour (Slider::rotarySliderFillColourId);

    // Soft contact shadow
    {
        const Point<float> sc (c.x, c.y + 8 * u);
        ColourGradient sg (Colour (0x591b1f26), sc, Colour (0x001b1f26), sc.translated (49 * u, 0), true);
        sg.addColour (0.7, Colour (0x591b1f26));
        g.setGradientFill (sg);
        g.fillEllipse (sc.x - 49 * u, sc.y - 47 * u, 98 * u, 94 * u);
    }
    // Dark outer ring
    g.setColour (Colour (0xff1c2027));
    g.fillEllipse (c.x - 47 * u, c.y - 47 * u, 94 * u, 94 * u);
    g.setColour (Colour (0xff3a404a));
    g.drawEllipse (c.x - 47 * u, c.y - 47 * u, 94 * u, 94 * u, jmax (0.8f, u));

    // Value arc with glow
    if (pos > 0.003f)
    {
        Path arc; arc.addCentredArc (c.x, c.y, 42 * u, 42 * u, 0, a0, ang, true);
        g.setColour (acc.withAlpha (0.28f));
        g.strokePath (arc, PathStrokeType (9 * u, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (acc);
        g.strokePath (arc, PathStrokeType (5 * u, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Metallic cap
    auto capGrad = [&] (Point<float> hl, float r)
    {
        ColourGradient cg (Colours::white, hl, Colour (0xff9aa1ac), hl.translated (r * 1.5f, 0), true);
        cg.addColour (0.45, Colour (0xffe3e6ea));
        return cg;
    };
    g.setGradientFill (capGrad ({ c.x - 12 * u, c.y - 20 * u }, 36 * u));
    g.fillEllipse (c.x - 36 * u, c.y - 36 * u, 72 * u, 72 * u);
    g.setColour (Colour (0xff7d8592));
    g.drawEllipse (c.x - 36 * u, c.y - 36 * u, 72 * u, 72 * u, jmax (0.5f, 0.6f * u));

    // Knurl ticks
    g.setColour (Colour (0xff9aa1ac));
    for (int i = 0; i < 36; ++i)
    {
        const float a = (float) i * MathConstants<float>::pi / 18.0f;
        g.drawLine (c.x + 33 * u * std::cos (a), c.y + 33 * u * std::sin (a),
                    c.x + 30 * u * std::cos (a), c.y + 30 * u * std::sin (a), jmax (0.6f, 0.8f * u));
    }

    // Inner dish (reversed gradient)
    {
        auto ig = capGrad ({ c.x + 12 * u, c.y + 20 * u }, 27 * u);
        ig.multiplyOpacity (0.55f);
        g.setGradientFill (ig);
        g.fillEllipse (c.x - 27 * u, c.y - 27 * u, 54 * u, 54 * u);
    }

    // Glowing pointer dot
    const Point<float> d (c.x + 27 * u * std::sin (ang), c.y - 27 * u * std::cos (ang));
    g.setColour (acc.withAlpha (0.35f));
    g.fillEllipse (d.x - 6 * u, d.y - 6 * u, 12 * u, 12 * u);
    g.setColour (acc);
    g.fillEllipse (d.x - 3.6f * u, d.y - 3.6f * u, 7.2f * u, 7.2f * u);
}

static AttributedString tipText (const String& tip, const Font& f)
{
    AttributedString a; a.setWordWrap (AttributedString::byWord);
    const int colon = tip.indexOfChar (':');
    if (colon > 0)
    {
        a.append (tip.substring (0, colon + 1), font (700, f.getHeight()), Colour (0xff9fe2ff));
        a.append (tip.substring (colon + 1), f, Colour (0xffdfe5ee));
    }
    else a.append (tip, f, Colour (0xffdfe5ee));
    return a;
}

Rectangle<int> AiryLook::getTooltipBounds (const String& tip, Point<int> pos, Rectangle<int> area)
{
    TextLayout tl; tl.createLayout (tipText (tip, tipFont), 240.0f);
    const int w = (int) std::ceil (tl.getWidth()) + 28, h = (int) std::ceil (tl.getHeight()) + 22;
    return Rectangle<int> (pos.x > area.getCentreX() ? pos.x - (w + 12) : pos.x + 14,
                           pos.y > area.getCentreY() ? pos.y - (h + 10) : pos.y + 18, w, h).constrainedWithin (area);
}

void AiryLook::drawTooltip (Graphics& g, const String& tip, int w, int h)
{
    const auto r = Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (Colour (0xf51d2026));
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (Colour (0xff3a404a));
    g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);
    TextLayout tl; tl.createLayout (tipText (tip, tipFont), 240.0f);
    tl.draw (g, r.reduced (14.0f, 11.0f));
}

void AiryLook::drawCornerResizer (Graphics& g, int w, int h, bool over, bool drag)
{
    const float s = (float) jmin (w, h);
    g.setColour (label.withAlpha (over || drag ? 0.9f : 0.55f));
    g.drawLine (s * 0.9f, s * 0.35f, s * 0.35f, s * 0.9f, 1.4f);
    g.drawLine (s * 0.9f, s * 0.64f, s * 0.64f, s * 0.9f, 1.4f);
}

// ======================= Knob =======================
Knob::Knob (AiryProcessor& p, const String& id, const String& l, Colour accent, int size, const String& tip)
    : dialSize (size), label (l)
{
    slider.setSliderStyle (Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (size > 60 ? 220 : 180);
    slider.setColour (Slider::rotarySliderFillColourId, accent);
    slider.setTooltip (tip);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, slider);
    if (auto* prm = p.apvts.getParameter (id))
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
}

void Knob::resized()
{
    slider.setBounds ((getWidth() - dialSize) / 2, 0, dialSize, dialSize);
}

void Knob::paint (Graphics& g)
{
    const bool big = dialSize > 60;
    auto r = getLocalBounds().toFloat().withTrimmedTop ((float) dialSize + 4.0f);
    g.setColour (ink);
    g.setFont (font (600, big ? 13.0f : 12.0f));
    g.drawText (slider.getTextFromValue (slider.getValue()), r.removeFromTop (17), Justification::centred, false);
    g.setColour (AiryUI::label);
    g.setFont (font (700, 9.5f, 1.6f));
    g.drawText (label.toUpperCase(), r.removeFromTop (13), Justification::centred, false);
}

// ======================= Screens =======================
static void screenGlow (Graphics& g, Rectangle<float> r, Colour c, float k)
{
    const Point<float> cen (r.getCentreX(), r.getY() + r.getHeight() * 0.55f);
    ColourGradient gg (c.withAlpha (jlimit (0.0f, 1.0f, (20.0f + jmin (k, 1.0f) * 80.0f) / 255.0f)), cen,
                       c.withAlpha (0.0f), cen.translated (r.getWidth() * 0.8f, 0), true);
    g.setGradientFill (gg);
    g.fillRect (r);
}

void Screen::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const Path clip = rounded (b, 12.0f);
    g.saveState();
    g.reduceClipRegion (clip);
    g.setColour (screenBg);
    g.fillRect (b);

    switch (kind)
    {
        case Air:     drawAir (g, b); break;
        case Sparkle: drawSparkle (g, b); break;
        case Space:   drawSpace (g, b); break;
    }

    if (proc.apvts.getRawParameterValue ("bypass")->load() > 0.5f)
    {
        g.setColour (Colour (0xa60a0c10));
        g.fillRect (b);
    }
    g.restoreState();
    g.setColour (screenLine);
    g.drawRoundedRectangle (b.reduced (0.5f), 12.0f, 1.0f);

    const char* titles[] = { "AIR", "SPARKLE", "SPACE" };
    const Colour cols[] = { airGlow, spkGlow, spcGlow };
    g.setFont (font (700, 10.0f, 2.0f));
    g.setColour (cols[kind]);
    g.drawText (titles[kind], b.withTrimmedLeft (12).withTrimmedTop (8).withHeight (14), Justification::left, false);
}

void Screen::drawAir (Graphics& g, Rectangle<float> r)
{
    auto& s = proc.apvts;
    const float air = s.getRawParameterValue ("air")->load(), fq = s.getRawParameterValue ("freq")->load(),
                tn = s.getRawParameterValue ("tone")->load();
    const bool dynOn = s.getRawParameterValue ("dynamic")->load() > 0.5f;
    dynShown += (proc.airDyn.load() - dynShown) * 0.3f;
    screenGlow (g, r, airGlow, air / 12.0f);

    g.setColour (Colours::white.withAlpha (0.05f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * i / 4.0f), r.getX(), r.getRight());

    const double fs = 48000.0;
    Biquad sh, to;
    sh.highShelf (fs, jmin (fq, 21000.0f), air, 0.7);
    to.peak (fs, 3500.0, tn / 100.0 * 4.0, 0.8);
    const float mid = r.getY() + r.getHeight() * 0.62f;

    auto curve = [&] (float gain)
    {
        Path p;
        for (int i = 0; i <= (int) r.getWidth(); i += 2)
        {
            const double f = 500.0 * std::pow (40.0, i / r.getWidth());
            const float db = (float) (sh.magnitudeDb (fs, f) * gain + to.magnitudeDb (fs, f));
            const float y = mid - db * r.getHeight() * 0.035f + std::sin (i * 0.09f + phase * 3.0f) * 0.7f;
            if (i == 0) p.startNewSubPath (r.getX(), y); else p.lineTo (r.getX() + (float) i, y);
        }
        return p;
    };

    if (dynOn)
    {
        Path dashed; const float dl[] = { 3.0f, 4.0f };
        PathStrokeType (1.2f).createDashedStroke (dashed, curve (1.0f), dl, 2);
        g.setColour (airGlow.withAlpha (0.4f));
        g.fillPath (dashed);
    }
    const Path live = curve (dynOn ? dynShown : 1.0f);
    g.setColour (airGlow.withAlpha (0.25f));
    g.strokePath (live, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colour (0xffbff0ff));
    g.strokePath (live, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));

    const float held = dynOn ? air * (1.0f - dynShown) : 0.0f;
    g.setFont (font (600, 11.0f));
    g.setColour (airGlow);
    g.drawText ("-" + String (held, 1) + " dB", r.withTrimmedRight (12).withTrimmedTop (8).withHeight (14), Justification::right, false);
}

void Screen::drawSparkle (Graphics& g, Rectangle<float> r)
{
    auto& s = proc.apvts;
    const bool tape = s.getRawParameterValue ("mode")->load() > 0.5f;
    const float spkV = s.getRawParameterValue ("sparkle")->load() / 100.f, drv = s.getRawParameterValue ("drive")->load() / 100.f;
    const Colour c = tape ? tapeGlow : spkGlow;
    const float live = jlimit (0.0f, 1.0f, proc.sparkleLevel.load() * 6.0f);
    screenGlow (g, r, c, spkV * (0.6f + 0.4f * live));

    const int n = 8;
    const float bw = r.getWidth() / (n * 1.9f), base = r.getY() + r.getHeight() * 0.82f;
    const float x0 = r.getCentreX() - n * bw * 1.9f / 2.0f + bw * 0.45f;
    for (int i = 0; i < n; ++i)
    {
        float a = i == 0 ? 0.7f
                         : (drv + 0.15f) * (tape ? (i % 2 ? 0.9f : 0.5f) : (i % 2 ? 0.35f : 0.85f)) / std::pow ((float) i + 1.0f, 0.55f);
        a *= (0.5f + 0.5f * spkV) * (0.78f + 0.22f * live) * (0.9f + 0.1f * std::sin (phase * 4.0f + (float) i));
        const float bh = jmin (a, 1.0f) * r.getHeight() * 0.6f;
        const Rectangle<float> bar (x0 + i * bw * 1.9f, base - bh, bw, bh);
        g.setColour (c.withAlpha (0.25f));
        g.fillRoundedRectangle (bar.expanded (2.5f), 3.0f);
        g.setGradientFill (ColourGradient (Colour (0xffffe2c9), 0, bar.getY(), c, 0, base, false));
        g.fillRect (bar);
    }

    // De-esser reading in dB
    const float gr = jlimit (0.0f, 0.99f, proc.deessGR.load());
    const float dsDb = -20.0f * std::log10 (1.0f - gr);
    dsShown = dsDb > dsShown ? dsDb : dsShown + (dsDb - dsShown) * 0.15f;
    g.setFont (font (600, 11.0f));
    g.setColour (Colour (0xffff9f40));
    g.drawText ("DS -" + String (dsShown, 1) + " dB", r.withTrimmedRight (12).withTrimmedTop (8).withHeight (14), Justification::right, false);
}

void Screen::drawSpace (Graphics& g, Rectangle<float> r)
{
    auto& s = proc.apvts;
    screenGlow (g, r, spcGlow, s.getRawParameterValue ("width")->load() / 250.0f);

    const float sz = jmin (r.getWidth(), r.getHeight()) * 0.36f;
    const Point<float> c (r.getCentreX(), r.getY() + r.getHeight() * 0.56f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawLine (c.x - sz, c.y - sz, c.x + sz, c.y + sz, 1.0f);
    g.drawLine (c.x + sz, c.y - sz, c.x - sz, c.y + sz, 1.0f);

    // Live signal only: empty while idle, fades in and out smoothly
    float peak = 0.0f;
    const int N = AiryProcessor::scopeSize;
    for (int i = 0; i < N; ++i)
        peak = jmax (peak, std::abs (proc.scopeL[(size_t) i].load()), std::abs (proc.scopeR[(size_t) i].load()));
    const float target = peak > 1.0e-4f ? 1.0f : 0.0f;
    liveMix += (target - liveMix) * (target > liveMix ? 0.18f : 0.06f);
    if (peak > 1.0e-4f)
    {
        const float want = sz / jmax (peak * 1.2f, 0.05f);
        scopeK = scopeK <= 0.0f ? want : scopeK + (want - scopeK) * 0.12f;
    }
    if (liveMix > 0.01f && scopeK > 0.0f)
    {
        g.setColour (Colour (0xffc8fff0).withAlpha (0.85f * liveMix));
        for (int i = 0; i < N; ++i)
        {
            const float l = proc.scopeL[(size_t) i].load(), rr = proc.scopeR[(size_t) i].load();
            g.fillRect (c.x + (l - rr) * 0.7071f * scopeK, c.y - (l + rr) * 0.7071f * scopeK, 1.5f, 1.5f);
        }
    }

    const float corr = proc.correlation.load();
    g.setFont (font (600, 11.0f));
    g.setColour (corr < 0.0f ? Colour (0xffff6a5a) : spcGlow);
    g.drawText ("CORR " + String (corr >= 0 ? "+" : "") + String (corr, 2),
                r.withTrimmedRight (12).withTrimmedTop (8).withHeight (14), Justification::right, false);
}

// ======================= Panel =======================
static const char* tipStatic  = "Static: the air boost stays the same all the time, like a normal EQ.";
static const char* tipDynamic = "Dynamic: the air boost follows the vocal. It backs off on loud, sharp moments and opens up on soft passages, so it never turns harsh.";
static const char* tipTube    = "Tube: smooth, warm saturation with soft even harmonics.";
static const char* tipTape    = "Tape: denser, glued saturation with stronger odd harmonics.";

AiryPanel::AiryPanel (AiryProcessor& p)
    : proc (p),
      sAir (p, Screen::Air), sSpk (p, Screen::Sparkle), sSpc (p, Screen::Space),
      kFreq  (p, "freq",    "Freq",    air, 48, "Air frequency: where the high-shelf lift starts. Lower sounds more present, higher sounds more silky."),
      kAir   (p, "air",     "Air",     air, 84, "Air: how much high-shelf boost is added on top of the vocal."),
      kTone  (p, "tone",    "Tone",    air, 48, "Tone: tilts the upper mids around 3.5 kHz. Left is softer, right is more forward."),
      kDrive (p, "drive",   "Drive",   spk, 48, "Drive: how hard the exciter is pushed. More drive gives denser, grittier harmonics."),
      kSpk   (p, "sparkle", "Sparkle", spk, 84, "Sparkle: blends in generated harmonics for shine that plain EQ cannot add."),
      kFocus (p, "focus",   "Focus",   spk, 48, "Focus: lowest frequency sent to the exciter. Higher means only the very top sparkles."),
      kFrom  (p, "from",    "From",    spc, 48, "From: width only applies above this frequency. The vocal body below it stays centered."),
      kWidth (p, "width",   "Width",   spc, 84, "Width: 100% is natural. Above 100% adds stereo spread, even from a mono vocal. Mono-safe."),
      kMono  (p, "mono",    "Mono",    spc, 48, "Mono: collapses stereo below the From frequency to keep the low end centered."),
      kDeess (p, "deess",   "De-ess",  outCol, 52, "De-ess: tames harsh S and SH sounds."),
      kMix   (p, "mix",     "Mix",     outCol, 52, "Mix: blend between the original and the processed vocal."),
      kOut   (p, "out",     "Output",  outCol, 52, "Output: final level after processing.")
{
    look.tipFont = font (500, 13.0f);
    setLookAndFeel (&look);
    for (auto* s : { &sAir, &sSpk, &sSpc }) addAndMakeVisible (*s);
    for (auto* k : { &kFreq, &kAir, &kTone, &kDrive, &kSpk, &kFocus, &kFrom, &kWidth, &kMono, &kDeess, &kMix, &kOut })
        addAndMakeVisible (*k);

    swAir = { {}, {}, "STATIC", "DYNAMIC", "dynamic", tipStatic, tipDynamic, airGlow };
    swSat = { {}, {}, "TUBE", "TAPE", "mode", tipTube, tipTape, spkGlow };

    setSize (baseW, baseH);
    setHints (proc.apvts.state.getProperty ("hints", true));
    startTimerHz (30);
}

AiryPanel::~AiryPanel() { tipWindow.reset(); setLookAndFeel (nullptr); }

void AiryPanel::setHints (bool on)
{
    hints = on;
    proc.apvts.state.setProperty ("hints", on, nullptr);
    if (on && tipWindow == nullptr) tipWindow = std::make_unique<TooltipWindow> (tipParent != nullptr ? tipParent : this, 550);
    if (! on) tipWindow.reset();
    if (tipWindow != nullptr) tipWindow->setLookAndFeel (&look);
    repaint();
}

void AiryPanel::resized()
{
    hintBtn  = { 578, 17, 42, 38 };
    powerBtn = { 632, 17, 42, 38 };
    Screen* sc[] = { &sAir, &sSpk, &sSpc };
    Knob* ks[3][3] = { { &kFreq, &kAir, &kTone }, { &kDrive, &kSpk, &kFocus }, { &kFrom, &kWidth, &kMono } };

    for (int i = 0; i < 3; ++i)
    {
        const int x = 16 + i * 223;
        sc[i]->setBounds (x, 66, 212, 194);
        segs[i] = { (float) x, 288, 212, 222 };

        // small - big - small, bottoms aligned
        const int rowL = x + 6, rowR = x + 212 - 6, bottom = 288 + 222 - 14;
        const int hs = 48 + 34, hb = 84 + 34;
        ks[i][0]->setBounds (rowL, bottom - hs, 58, hs);
        ks[i][1]->setBounds (x + 106 - 42, bottom - hb, 84, hb);
        ks[i][2]->setBounds (rowR - 58, bottom - hs, 58, hs);
    }

    footer = { 16, 522, 658, 104 };
    const int fx = 345 - 118, fy = 522 + 104 - 12 - (52 + 34);
    kDeess.setBounds (fx, fy, 58, 86);
    kMix.setBounds (345 - 29, fy, 58, 86);
    kOut.setBounds (345 + 118 - 58, fy, 58, 86);

    // Mode switches, centered near the top of their segment
    auto layoutSwitch = [&] (Switch& s, Rectangle<float> seg)
    {
        const Font f = font (700, 11.0f, 0.8f);
        const float wa = f.getStringWidthFloat (s.la) + 24.0f, wb = f.getStringWidthFloat (s.lb) + 24.0f;
        const float x0 = seg.getCentreX() - (wa + wb + 6.0f) / 2.0f + 3.0f, y0 = seg.getY() + 38.0f + 3.0f;
        s.a = { x0, y0, wa, 28.0f };
        s.b = { x0 + wa, y0, wb, 28.0f };
    };
    layoutSwitch (swAir, segs[0]);
    layoutSwitch (swSat, segs[1]);
}

void AiryPanel::drawCard (Graphics& g, Rectangle<float> r)
{
    DropShadow (Colour (0x403a4250), 12, { 0, 6 }).drawForPath (g, rounded (r, 14.0f));
    g.setGradientFill (ColourGradient (Colour (0xfff7f8fa), 0, r.getY(), Colour (0xffe9ecf0), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (Colour (0xffcfd4db));
    g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);
    g.setColour (Colours::white);
    g.drawLine (r.getX() + 14, r.getY() + 1.5f, r.getRight() - 14, r.getY() + 1.5f, 1.0f);
}

void AiryPanel::drawSwitch (Graphics& g, const Switch& s, bool second)
{
    const auto bg = s.a.getUnion (s.b).expanded (3.0f);
    DropShadow (Colour (0x33000000), 6, { 0, 3 }).drawForPath (g, rounded (bg, 11.0f));
    g.setColour (Colour (0xff1b1f26));
    g.fillRoundedRectangle (bg, 11.0f);
    g.setFont (font (700, 11.0f, 0.8f));
    for (int i = 0; i < 2; ++i)
    {
        const auto r = i == 0 ? s.a : s.b;
        const bool on = (i == 1) == second;
        if (on)
        {
            g.setColour (Colour (0xff2c333d));
            g.fillRoundedRectangle (r, 8.0f);
            g.setColour (Colour (0xff3d4450));
            g.drawLine (r.getX() + 6, r.getY() + 0.5f, r.getRight() - 6, r.getY() + 0.5f, 1.0f);
        }
        g.setColour (on ? s.accent : Colour (0xff6c7586));
        g.drawText (i == 0 ? s.la : s.lb, r, Justification::centred, false);
    }
}

void AiryPanel::paint (Graphics& g)
{
    // Panel base and dark deck
    g.setGradientFill (ColourGradient (Colour (0xfff2f4f6), 0, 274, Colour (0xffe2e5ea), 0, (float) baseH, false));
    g.fillAll();
    g.setColour (deck);
    g.fillRect (0, 0, baseW, 274);

    // Logo: "Air" + cyan "y" + version
    const Font lf = font (500, 28.0f, 0.5f);
    g.setFont (lf);
    g.setColour (txt);
    const float ax = 24.0f, ay = 14.0f;
    g.drawText ("Air", Rectangle<float> (ax, ay, 80, 44), Justification::centredLeft, false);
    const float wAir = lf.getStringWidthFloat ("Air");
    g.setColour (airGlow);
    g.drawText ("y", Rectangle<float> (ax + wAir, ay, 30, 44), Justification::centredLeft, false);
    const float wY = lf.getStringWidthFloat ("y");
    g.setFont (font (500, 12.0f, 1.0f));
    g.setColour (dim);
    g.drawText ("v2.2", Rectangle<float> (ax + wAir + wY + 8.0f, ay + 6.0f, 60, 44), Justification::centredLeft, false);

    g.setFont (font (500, 14.0f));
    g.drawText ("Developed by Ewin", Rectangle<float> (300, 17, 260, 38), Justification::centredRight, false);

    // Tips and power buttons
    const bool byp = proc.apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    auto drawButton = [&] (Rectangle<float> r, bool on)
    {
        DropShadow (Colour (0xaa000000), 6, { 0, 3 }).drawForPath (g, rounded (r, 11.0f));
        g.setColour (Colour (0xff262c36));
        g.fillRoundedRectangle (r, 11.0f);
        g.setColour (Colour (0xff3a414d));
        g.drawLine (r.getX() + 8, r.getY() + 0.5f, r.getRight() - 8, r.getY() + 0.5f, 1.0f);
        const Rectangle<float> led (r.getRight() - 11, r.getY() + 5, 6, 6);
        if (on) { g.setColour (airGlow.withAlpha (0.35f)); g.fillEllipse (led.expanded (3)); }
        g.setColour (on ? airGlow : Colour (0xff3a414d));
        g.fillEllipse (led);
        return on ? airGlow : Colour (0xff5a6270);
    };
    g.setColour (drawButton (hintBtn, hints));
    g.setFont (font (700, 15.0f));
    g.drawText ("?", hintBtn, Justification::centred, false);
    {
        g.setColour (drawButton (powerBtn, ! byp));
        const auto c = powerBtn.getCentre();
        Path pw;
        pw.addCentredArc (c.x, c.y + 1, 7, 7, 0, 0.65f, MathConstants<float>::twoPi - 0.65f, true);
        pw.startNewSubPath (c.x, c.y - 8); pw.lineTo (c.x, c.y - 1);
        g.strokePath (pw, PathStrokeType (1.9f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Segment cards
    const char* titles[] = { "AIR", "SPARKLE", "SPACE" };
    const Colour tcol[] = { air, Colour (0xfff0643f), spc };
    for (int i = 0; i < 3; ++i)
    {
        drawCard (g, segs[i]);
        g.setFont (font (700, 10.0f, 2.5f));
        g.setColour (tcol[i]);
        g.drawText (titles[i], segs[i].withTrimmedLeft (14).withTrimmedTop (12).withHeight (14), Justification::left, false);
    }
    drawSwitch (g, swAir, proc.apvts.getRawParameterValue ("dynamic")->load() > 0.5f);
    drawSwitch (g, swSat, proc.apvts.getRawParameterValue ("mode")->load() > 0.5f);

    // Output strip with credits
    drawCard (g, footer);
    auto credit = [&] (Rectangle<float> r, const char* a, const char* b, Justification j)
    {
        g.setFont (font (600, 11.0f));
        g.setColour (Colour (0xff7a818c));
        g.drawText (a, r.withHeight (18), j, false);
        g.setFont (font (500, 11.0f));
        g.setColour (Colour (0xff9aa1ac));
        g.drawText (b, r.withTrimmedTop (19).withHeight (18), j, false);
    };
    g.setFont (font (500, 11.0f));
    g.setColour (Colour (0xff9aa1ac));
    g.drawText (String (CharPointer_UTF8 ("VST3 / AU \xc2\xb7 v2.2")), Rectangle<float> (footer.getX() + 20, footer.getY() + 43, 170, 18),
                Justification::left, false);
    credit ({ footer.getRight() - 200, footer.getY() + 34, 170, 40 }, "Made with Claude", "inspired by Fresh Air", Justification::right);
}

void AiryPanel::setParam (const char* id, float v)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (v);
        prm->endChangeGesture();
    }
}

void AiryPanel::mouseDown (const MouseEvent& e)
{
    const auto pos = e.position;
    if (hintBtn.contains (pos)) { setHints (! hints); return; }
    if (powerBtn.contains (pos)) setParam ("bypass", proc.apvts.getRawParameterValue ("bypass")->load() > 0.5f ? 0.0f : 1.0f);
    for (auto* s : { &swAir, &swSat })
    {
        if (s->a.contains (pos)) setParam (s->param.toRawUTF8(), 0.0f);
        if (s->b.contains (pos)) setParam (s->param.toRawUTF8(), 1.0f);
    }
    repaint();
}

String AiryPanel::getTooltip()
{
    const auto pos = getMouseXYRelative().toFloat();
    if (powerBtn.contains (pos)) return "Bypass: turns the whole effect on or off.";
    if (hintBtn.contains (pos))  return "Tips: turn these pop-up explanations on or off.";
    for (auto* s : { &swAir, &swSat })
    {
        if (s->a.contains (pos)) return s->tipA;
        if (s->b.contains (pos)) return s->tipB;
    }
    return {};
}

void AiryPanel::timerCallback()
{
    for (auto* s : { &sAir, &sSpk, &sSpc }) { s->phase += 0.033f; s->repaint(); }
}

// ======================= Resizable editor =======================
AiryEditor::AiryEditor (AiryProcessor& p) : AudioProcessorEditor (&p), proc (p), panel (p)
{
    // Read the saved size first: setting resize limits triggers resized(), which overwrites it
    const int w = jlimit (baseW * 7 / 10, baseW * 2, (int) proc.apvts.state.getProperty ("uiWidth", baseW));
    addAndMakeVisible (panel);
    panel.setTipParent (this);
    setResizable (true, true);
    setResizeLimits (baseW * 7 / 10, baseH * 7 / 10, baseW * 2, baseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / baseH);
    setSize (w, roundToInt (w * (double) baseH / baseW));
}

void AiryEditor::resized()
{
    panel.setTransform (AffineTransform::scale ((float) getWidth() / (float) baseW));
    proc.apvts.state.setProperty ("uiWidth", getWidth(), nullptr);
}
