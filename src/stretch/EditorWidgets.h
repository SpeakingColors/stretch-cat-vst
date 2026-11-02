// Stretch Cat editor widgets (cat-ui window built from docs/mockup/stretchcat.html). Colours and sizes are the mockup's.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <BinaryData.h>
#include "../editor/Layout.h"
#include "StretchCat.h"
#include <functional>

namespace sc
{
using juce::Colour; using juce::Graphics; using juce::String; using juce::Rectangle;
const Colour kBg(0xff0b0b0d), kPanel(0xff15161a), kPanel2(0xff1c1d22), kLine(0xff2b2d34), kInk(0xfff4f5f7), kDim(0xffa9adb8), kDisp(0xff05070a),
    kPurple(0xffb388ff), kLav(0xffcd92fe), kPressed(0xff241238), kYellow(0xffffd100), kOrange(0xffff8a3d), kMint(0xff02ebba), kPink(0xffff1383), kHead(0xff9a94e8), kGrid(0xff1a2422), kTick(0xff6c7080);

enum class F { label, semi, bold, mono };
inline juce::Font font(F k, float px)
{
    static const auto med  = juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedMedium_ttf, BinaryData::BarlowCondensedMedium_ttfSize);
    static const auto semi = juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize);
    static const auto bold = juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedBold_ttf, BinaryData::BarlowCondensedBold_ttfSize);
    static const auto mono = juce::Typeface::createSystemTypefaceFor(BinaryData::JetBrainsMonoRegular_ttf, BinaryData::JetBrainsMonoRegular_ttfSize);
    const auto& tf = k == F::label ? med : k == F::semi ? semi : k == F::bold ? bold : mono;
    return juce::Font(juce::FontOptions(tf).withHeight(px));
}
inline Rectangle<float> rc(const layout::Rc& r) { return { r.x, r.y, r.w, r.h }; }
inline Rectangle<int> irc(const layout::Rc& r) { return { juce::roundToInt(r.x), juce::roundToInt(r.y), juce::roundToInt(r.w), juce::roundToInt(r.h) }; }
// text with letter spacing in px
inline void text(Graphics& g, const String& s, Rectangle<float> r, juce::Font f, Colour c, juce::Justification j = juce::Justification::centred, float spacing = 0.f)
{
    if (spacing != 0.f) f = f.withExtraKerningFactor(spacing / std::max(1.f, f.getHeight()));
    g.setFont(f); g.setColour(c); g.drawText(s, r.expanded(40.f, 0.f), j, false);
}
inline String noteName(int m) { static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return String(n[((m % 12) + 12) % 12]) + String(m / 12 - 2); }
inline String fmtHz(double v) { return v >= 1000 ? String(v / 1000.0, v >= 10000 ? 1 : 2).trimCharactersAtEnd("0").trimCharactersAtEnd(".") + " kHz" : String(juce::roundToInt(v)) + " Hz"; }
inline String fmtMs(double v) { return v < 10 ? String(v, 1) + " ms" : v < 1000 ? String(juce::roundToInt(v)) + " ms" : String(v / 1000.0, 2) + " s"; }
inline String fmtDb(double v) { return String(v > 0.05 ? "+" : "") + String(v, 1) + " dB"; }

// Push button in the mockup's button style; toggles light up purple.
class Btn : public juce::Button
{
public:
    explicit Btn(const String& t, float px = 13.f) : juce::Button(t), size(px) {}
    float size; bool pad = false;
    void paintButton(Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(0.5f); const bool on = getToggleState(), dis = !isEnabled();
        g.setColour(on ? kPressed : (down ? Colour(0xff2a2b33) : Colour(0xff1c1d22))); g.fillRoundedRectangle(r, 3.f);
        g.setColour(on ? kPurple : (over && !dis ? Colour(0xff4a4d58) : kLine)); g.drawRoundedRectangle(r, 3.f, 1.f);
        sc::text(g, getButtonText().toUpperCase(), r, font(F::semi, size), dis ? kDim.withAlpha(0.45f) : (on ? kLav : kInk), juce::Justification::centred, 1.0f);
    }
};

// Rotary knob: 44 px body, 270 degree arc, value text below, as the mockup's .kw.
class Knob : public juce::Slider
{
public:
    Knob() { setSliderStyle(juce::Slider::RotaryVerticalDrag); setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0); setMouseDragSensitivity(160); setScrollWheelEnabled(true); }
    layout::Item it {}; String label; std::function<String(double)> fmt; bool bipolar = false; double midValue = 0.0;
    void paint(Graphics& g) override
    {
        const auto kn = rc(it.knob).translated(-it.r.x, -it.r.y); const auto c = kn.getCentre(); const float r0 = 23.f;
        const double lo = getMinimum(), hi = getMaximum(); const auto norm = [&](double v) { return (double) valueToProportionOfLength(v); };
        const float a0 = juce::degreesToRadians(-135.f), a1 = juce::degreesToRadians(135.f), av = a0 + (a1 - a0) * (float) norm(getValue()); (void) lo; (void) hi;
        juce::Path trk; trk.addCentredArc(c.x, c.y, r0, r0, 0.f, a0, a1, true); g.setColour(Colour(0xff33353d)); g.strokePath(trk, juce::PathStrokeType(3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float am = bipolar ? a0 + (a1 - a0) * (float) norm(midValue) : a0;
        if (std::fabs(av - am) > 0.01f) { juce::Path v; v.addCentredArc(c.x, c.y, r0, r0, 0.f, std::min(av, am), std::max(av, am), true); g.setColour(isMouseOverOrDragging() ? kLav : kPurple); g.strokePath(v, juce::PathStrokeType(3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
        g.setGradientFill(juce::ColourGradient(Colour(0xff33363f), c.x - 6, c.y - 7, Colour(0xff23252c), c.x + 14, c.y + 14, true)); g.fillEllipse(kn);
        g.setColour(Colour(0xff3a3d48)); g.drawEllipse(kn.reduced(0.5f), 1.f);
        juce::Path t; t.addRoundedRectangle(-1.f, -19.f, 2.f, 15.f, 1.f); g.setColour(kInk); g.fillPath(t, juce::AffineTransform::rotation(av).translated(c.x, c.y));
        text(g, label.toUpperCase(), rc(it.kl).translated(-it.r.x, -it.r.y), font(F::label, 11.f), kDim, juce::Justification::centred, 1.5f);
        text(g, fmt ? fmt(getValue()) : String(getValue(), 2), rc(it.kv).translated(-it.r.x, -it.r.y), font(F::mono, 12.f), kPurple);
    }
};

// A drop down in the mockup's select style.
class Sel : public juce::ComboBox
{
public:
    Sel() { setColour(ComboBox::backgroundColourId, Colour(0xff0a0a0c)); setColour(ComboBox::outlineColourId, kLine); setColour(ComboBox::textColourId, kInk); setColour(ComboBox::arrowColourId, kInk); setColour(ComboBox::focusedOutlineColourId, kPurple); }
};

// Wave of peaks, shared by the source and result views.
inline void drawPeaks(Graphics& g, const std::vector<float>& p, Rectangle<float> r, Colour c, float alpha = 1.f, float x0 = 0.f, float x1 = 1e9f, Colour dim = Colour())
{
    const int n = (int) p.size(), w = (int) r.getWidth(); if (n == 0 || w < 2) return; const float m = r.getCentreY(), amp = r.getHeight() * 0.5f * 0.92f;
    for (int x = 0; x < w; ++x)
    {
        float pk = 0.f; const int i0 = x * n / w, i1 = std::max(i0 + 1, (x + 1) * n / w); for (int i = i0; i < i1 && i < n; ++i) pk = std::max(pk, p[(size_t) i]);
        g.setColour((x >= x0 && x <= x1 ? c : (dim.isTransparent() ? c : dim)).withAlpha(alpha)); const float h = std::max(1.f, std::min(1.f, pk) * amp); g.fillRect(r.getX() + (float) x, m - h, 1.f, 2.f * h);
    }
}
}
