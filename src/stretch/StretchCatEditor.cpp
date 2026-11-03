// Stretch Cat window, built from docs/mockup/stretchcat.html through src/editor/Layout.h (read from the mockup by tools/dump_layout.py):
// header with presets, sample bar, source waveform with selection handles and slice marks, slice pads, result waveform, over-time lanes,
// stretch and playback controls, FX rack (filter, EQ, compressor, formant) and the level meter. Fixed aspect ratio, resizable from
// every corner and edge, wood cheeks, info bubbles after 1 s, drawing paused by catui::RenderGate when the window is not in front.
#include "EditorWidgets.h"
#include "EditorPresets.h"
#include <catui/CatLookAndFeel.h>
#include <catui/RenderGate.h>
#include <catui/Logo.h>
#include <cmath>

namespace sc
{
using stretchcat::Lane;

// ------------------------------------------------------------------ source view: selection handles, slice marks
class SrcView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit SrcView(StretchCatProcessor& pr) : p(pr) {}
    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat(); const float w = r.getWidth(), h = r.getHeight(); g.fillAll(kDisp);
        g.setColour(kLine); g.drawRect(r, 1.f);
        if (!p.hasSource()) { text(g, "No sound loaded. Press Load file.", r, font(F::label, 18.f), kDim); return; }
        const float a = (float) p.selA() * w, b = (float) p.selB() * w;
        drawPeaks(g, p.srcPeaks(), r, kPurple, 1.f, a, b, Colour(0xff44425a));
        g.setColour(Colour(0x80000000)); g.fillRect(0.f, 0.f, a, h); g.fillRect(b, 0.f, w - b, h);
        const auto& m = p.slices().marks; std::vector<double> cuts { p.selA() }; cuts.insert(cuts.end(), m.begin(), m.end()); cuts.push_back(p.selB());
        for (double c : m) { g.setColour(kYellow); g.fillRect((float) c * w - 1.f, 0.f, 2.f, h); }
        for (size_t i = 0; i + 1 < cuts.size(); ++i) text(g, String((int) i + 1), { (float) cuts[i] * w + 11.f, 3.f, 20.f, 14.f }, font(F::mono, 11.f), kYellow, juce::Justification::left);
        g.setColour(kOrange); g.fillRect(a - 2.f, 0.f, 4.f, h); g.fillRect(b - 2.f, 0.f, 4.f, h); g.fillRect(a - 2.f, 0.f, 9.f, 8.f); g.fillRect(b - 7.f, 0.f, 9.f, 8.f);
        const float pp = p.playPosition(); if (pp >= 0.f && p.sourceSamples() > 0) { const double s = stretchcat::outToSource(p.resultMap(), (double) pp * (double) p.resultLength()) / (double) p.sourceSamples(); g.setColour(juce::Colours::white); g.fillRect((float) s * w - 1.f, 0.f, 2.f, h); }
        text(g, "drag the orange handles and the yellow marks", { w - 304.f, h - 16.f, 300.f, 14.f }, font(F::mono, 10.f), Colour(0xff8f93a3), juce::Justification::right);
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!p.hasSource()) return; const float x = e.position.x, w = (float) getWidth(); drag = -3; float bd = 12.f;
        if (std::fabs(x - (float) p.selA() * w) < bd) { drag = -1; bd = std::fabs(x - (float) p.selA() * w); }
        if (std::fabs(x - (float) p.selB() * w) < bd) drag = -2;
        if (drag == -3) { const auto& m = p.slices().marks; for (size_t i = 0; i < m.size(); ++i) if (std::fabs(x - (float) m[i] * w) < 8.f) drag = (int) i; }
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (drag == -3) return; const double f = juce::jlimit(0.0, 1.0, (double) e.position.x / (double) getWidth());
        if (drag == -1) p.setSelection(std::min(f, p.selB() - 0.02), p.selB()); else if (drag == -2) p.setSelection(p.selA(), std::max(f, p.selA() + 0.02));
        else { auto s = p.slices(); s.move(drag, f, p.selA(), p.selB()); p.setSlices(s); }
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { drag = -3; }
private:
    StretchCatProcessor& p; int drag = -3;
};

class ResView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ResView(StretchCatProcessor& pr) : p(pr) {}
    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat(); g.fillAll(kDisp); g.setColour(kLine); g.drawRect(r, 1.f);
        if (p.resultPeaks().empty() || !p.hasSource()) { text(g, p.rendering() ? "Rendering..." : "", r.withTrimmedLeft(6.f), font(F::label, 13.f), kDim, juce::Justification::topLeft); return; }
        drawPeaks(g, p.resultPeaks(), r, kMint, 0.85f);
        const auto& e = p.resultSliceEdges(); for (size_t i = 1; i + 1 < e.size(); ++i) { g.setColour(kYellow.withAlpha(0.7f)); g.fillRect((float) e[i] * r.getWidth() - 0.5f, 0.f, 1.f, r.getHeight()); }
        const float pp = p.playPosition(); if (pp >= 0.f) { g.setColour(juce::Colours::white); g.fillRect(pp * r.getWidth() - 1.f, 0.f, 2.f, r.getHeight()); }
        text(g, "Result  " + String(p.resultSeconds(), 2) + " s" + (p.rendering() ? "   rendering..." : ""), r.withTrimmedLeft(6.f).withHeight(18.f).withTop(4.f), font(F::mono, 11.f), Colour(0xff8f93a3), juce::Justification::topLeft);
    }
private:
    StretchCatProcessor& p;
};

// ------------------------------------------------------------------ over-time lane editor (as the mockup)
class LaneView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit LaneView(StretchCatProcessor& pr) : p(pr) {}
    char id = 'p';
    struct Def { const char* name; double lo, hi; bool log; };
    static Def def(char id) { switch (id) { case 'p': return { "Pitch", -12, 12, false }; case 's': return { "Speed", 0.25, 4, true }; case 'c': return { "Cutoff", 100, 16000, true }; case 'f': return { "Formant", -12, 12, false }; default: return { "Width", 0, 2, false }; } }
    static double val(char id, double y) { auto d = def(id); return d.log ? d.lo * std::pow(d.hi / d.lo, y) : d.lo + y * (d.hi - d.lo); }
    static double yOf(char id, double v) { auto d = def(id); return d.log ? std::log(v / d.lo) / std::log(d.hi / d.lo) : (v - d.lo) / (d.hi - d.lo); }
    static String fmtVal(char id, double v) { switch (id) { case 'p': case 'f': return String(v > 0 ? "+" : "") + String(v, 1) + " st"; case 's': return "x" + String(v, 2); case 'c': return fmtHz(v); default: return String(juce::roundToInt(v * 100)) + " %"; } }
    void paint(Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight(), l = 40.f, r = w - 12.f, t = 12.f, b = h - 18.f; g.fillAll(kDisp); g.setColour(kLine); g.drawRect(getLocalBounds().toFloat(), 1.f);
        auto Y = [&](double y) { return b - (float) y * (b - t); }; auto X = [&](double x) { return l + (float) x * (r - l); };
        std::vector<double> ticks = (id == 'p' || id == 'f') ? std::vector<double> { -12, -6, 0, 6, 12 } : id == 's' ? std::vector<double> { 0.25, 0.5, 1, 2, 4 } : id == 'c' ? std::vector<double> { 100, 300, 1000, 3000, 10000 } : std::vector<double> { 0, 0.5, 1, 1.5, 2 };
        for (double v : ticks) { const float y = std::round(Y(yOf(id, v))) + 0.5f; g.setColour(kGrid); g.drawHorizontalLine((int) y, l, r);
            String s = id == 'c' ? (v >= 1000 ? String(v / 1000) + "k" : String((int) v)) : id == 'w' ? String(juce::roundToInt(v * 100)) + "%" : id == 's' ? "x" + String(v) : String(v > 0 ? "+" : "") + String((int) v);
            text(g, s, { 0.f, y - 7.f, l - 4.f, 14.f }, font(F::mono, 10.f), kTick, juce::Justification::right); }
        for (int i = 0; i <= 8; ++i) { const float x = std::round(X(i / 8.0)) + 0.5f; g.setColour(i % 2 ? Colour(0xff12181a) : kGrid); g.drawVerticalLine((int) x, t, b); }
        text(g, "start of selection", { l, b + 3.f, 120.f, 12.f }, font(F::mono, 10.f), kTick, juce::Justification::left); text(g, "end", { r - 40.f, b + 3.f, 40.f, 12.f }, font(F::mono, 10.f), kTick, juce::Justification::right);
        const auto lane = p.getLane(id); const auto& pts = lane.pts;
        juce::Path line, fill; line.startNewSubPath(X(0), Y(pts[0].second)); for (auto& q : pts) line.lineTo(X(q.first), Y(q.second)); line.lineTo(X(1), Y(pts.back().second));
        fill.startNewSubPath(X(0), Y(0)); for (auto& q : pts) fill.lineTo(X(q.first), Y(q.second)); fill.lineTo(X(1), Y(0)); fill.closeSubPath();
        g.setColour(Colour(0x1fb388ff)); g.fillPath(fill); g.setColour(kPurple); g.strokePath(line, juce::PathStrokeType(2.f));
        for (size_t i = 0; i < pts.size(); ++i) { const float rad = (dragI == (int) i) ? 8.f : 6.f; g.setColour(kYellow); g.fillEllipse(X(pts[i].first) - rad, Y(pts[i].second) - rad, 2 * rad, 2 * rad); g.setColour(juce::Colours::black); g.drawEllipse(X(pts[i].first) - rad, Y(pts[i].second) - rad, 2 * rad, 2 * rad, 1.f); }
        const float pp = p.playPosition(); if (pp >= 0.f && p.sourceSamples() > 0) { const double n = (double) p.sourceSamples(), s = stretchcat::outToSource(p.resultMap(), (double) pp * (double) p.resultLength()); const double tn = juce::jlimit(0.0, 1.0, (s - p.selA() * n) / std::max(1.0, (p.selB() - p.selA()) * n)); g.setColour(juce::Colours::white); g.fillRect(X(tn) - 1.f, t, 2.f, b - t); }
        text(g, readout, { l + 6.f, t, 300.f, 14.f }, font(F::mono, 11.f), kLav, juce::Justification::left);
    }
    void setLaneId(char c) { id = c; readout = {}; repaint(); }
    void flat() { Lane l; const double v = id == 'c' ? 1.0 : 0.5; l.pts = { { 0.0, v }, { 1.0, v } }; p.setLane(id, l); repaint(); }
private:
    StretchCatProcessor& p; int dragI = -1; String readout;
    struct Geo { float l, r, t, b; }; Geo geo() const { return { 40.f, (float) getWidth() - 12.f, 12.f, (float) getHeight() - 18.f }; }
    juce::Point<double> pos(const juce::MouseEvent& e) const { auto g = geo(); return { juce::jlimit(0.0, 1.0, (double) ((e.position.x - g.l) / (g.r - g.l))), juce::jlimit(0.0, 1.0, (double) ((g.b - e.position.y) / (g.b - g.t))) }; }
    int hit(const Lane& l, const juce::MouseEvent& e) const { auto g = geo(); int best = -1; for (size_t i = 0; i < l.pts.size(); ++i) if (std::hypot(g.l + l.pts[i].first * (g.r - g.l) - e.position.x, g.b - l.pts[i].second * (g.b - g.t) - e.position.y) < 12.0) best = (int) i; return best; }
    void mouseDown(const juce::MouseEvent& e) override
    {
        auto l = p.getLane(id); auto q = pos(e); int h = hit(l, e);
        if (h < 0) { l.pts.push_back({ q.x, q.y }); std::sort(l.pts.begin(), l.pts.end()); for (size_t i = 0; i < l.pts.size(); ++i) if (l.pts[i].first == q.x && l.pts[i].second == q.y) h = (int) i; p.setLane(id, l); }
        dragI = h; repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragI < 0) return; auto l = p.getLane(id); auto q = pos(e); const int i = dragI; if (i >= (int) l.pts.size()) return;
        if (i > 0 && i + 1 < (int) l.pts.size()) l.pts[(size_t) i].first = juce::jlimit(l.pts[(size_t) i - 1].first + 0.005, l.pts[(size_t) i + 1].first - 0.005, q.x);
        l.pts[(size_t) i].second = q.y; p.setLane(id, l); readout = def(id).name + String(": ") + fmtVal(id, val(id, q.y)); repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { dragI = -1; repaint(); }
    void mouseMove(const juce::MouseEvent& e) override { auto q = pos(e); readout = def(id).name + String(" at ") + String(juce::roundToInt(q.x * 100)) + "%: " + fmtVal(id, val(id, q.y)); repaint(); }
    void mouseExit(const juce::MouseEvent&) override { readout = {}; repaint(); }
    void mouseDoubleClick(const juce::MouseEvent& e) override { auto l = p.getLane(id); int h = hit(l, e); if (h > 0 && h + 1 < (int) l.pts.size()) { l.pts.erase(l.pts.begin() + h); p.setLane(id, l); repaint(); } }
};

// Output meter: L and R peak bars with a dB scale; colours at fixed dB positions (blue to -12, purple to 0, pink above).
class MeterView : public juce::Component, public juce::SettableTooltipClient
{
public:
    float l = -90, r = -90, hl = -90, hr = -90;
    void update(float pl, float pr, float dt)
    {
        auto db = [](float v) { return v > 1e-5f ? 20.f * std::log10(v) : -90.f; }; const float a = db(pl), b = db(pr);
        l = a > l ? a : std::max(a, l - 24.f * dt); r = b > r ? b : std::max(b, r - 24.f * dt); hl = a > hl ? a : std::max(-90.f, hl - 6.f * dt); hr = b > hr ? b : std::max(-90.f, hr - 6.f * dt);
        if (a >= 0.f) clipL = true; if (b >= 0.f) clipR = true;
    }
    void resetClip() { clipL = clipR = false; hl = hr = -90.f; }
    void mouseDown(const juce::MouseEvent&) override { resetClip(); }
    void paint(Graphics& g) override
    {
        // Horizontal meter: L bar above R bar, dB scale along the bottom. Colours sit at fixed dB positions: blue up to -12, purple -12 to 0, pink above 0.
        const float w = (float) getWidth(), h = (float) getHeight(), x0 = 18.f, x1 = w - 8.f; auto Xd = [&](float d) { return x0 + (juce::jlimit(-54.f, 3.f, d) + 54.f) / 57.f * (x1 - x0); };
        g.fillAll(Colour(0xff06080a)); g.setColour(kLine); g.drawRect(getLocalBounds().toFloat(), 1.f);
        const float barH = 22.f, yL = 8.f, yR = 8.f + barH + 6.f, yS = yR + barH + 4.f;
        for (int d = -54; d <= 0; d += 6) { const float x = std::round(Xd((float) d)) + 0.5f; g.setColour(kGrid); g.drawVerticalLine((int) x, yL, yS); g.setColour(Colour(0xff3a3f4a)); g.drawVerticalLine((int) x, yS, yS + 4.f);
            if (d == 0) text(g, "0 dB", { x - 28.f, yS + 5.f, 46.f, 12.f }, font(F::mono, 10.f), Colour(0xffffb35c), juce::Justification::right);
            else text(g, String(d), { x - 14.f, yS + 5.f, 28.f, 12.f }, font(F::mono, 10.f), d >= -6 ? Colour(0xffffb35c) : Colour(0xff8f8bd6)); }
        auto bar = [&](float y, float val, float hold, bool clip, const char* name)
        {
            g.setColour(Colour(0xff0e1114)); g.fillRect(x0, y, x1 - x0, barH);
            const float xv = Xd(val), z0 = Xd(-54.f), z1 = Xd(-12.f), z2 = Xd(0.f), z3 = Xd(3.f);
            auto seg = [&](float a, float b, Colour c) { const float e = std::min(b, xv); if (e > a) { g.setColour(c); g.fillRect(a, y, e - a, barH); } };
            seg(z0, z1, Colour(0xff6cc9f5)); seg(z1, z2, Colour(0xffa43dff)); seg(z2, z3, kPink);
            g.setColour(clip ? kPink : juce::Colours::white); g.fillRect(Xd(hold) - 1.f, y, 2.f, barH); if (clip) g.fillRect(x1 - 5.f, y, 5.f, barH);
            text(g, name, { 2.f, y, 14.f, barH }, font(F::mono, 11.f), Colour(0xffa9adb8));
        };
        bar(yL, l, hl, clipL, "L"); bar(yR, r, hr, clipR, "R");
        (void) h;
    }
private:
    bool clipL = false, clipR = false;
};
}

// ====================================================================================== the editor
class StretchCatEditor : public juce::AudioProcessorEditor
{
public:
    explicit StretchCatEditor(StretchCatProcessor& pr);
    ~StretchCatEditor() override { setLookAndFeel(nullptr); content.setLookAndFeel(nullptr); }
    void paint(juce::Graphics& g) override { g.fillAll(sc::kBg); catui::drawWoodCheeks(g, getLocalBounds(), juce::roundToInt(layout::Cheek * scale())); }
    void resized() override
    {
        const float s = scale(); content.setBounds(juce::roundToInt(layout::Cheek * s), 0, (int) layout::W, (int) layout::H); content.setTransform(juce::AffineTransform::scale(s)); border.setBounds(getLocalBounds()); sizeLabelDirty = true;
    }
    void mouseDoubleClick(const juce::MouseEvent& e) override { const auto p = e.getEventRelativeTo(this).getPosition(); if (p.x < layout::Cheek * scale() || p.x > getWidth() - layout::Cheek * scale()) setSize(kBaseW, kBaseH); }
    void openDialogForTesting(const juce::String& n) { if (n == "settings") showSettings(); else if (n == "about") showAbout(); else if (n == "guide") showGuide(); else if (n == "presets") showPresetMenu(); else if (n == "save") savePresetDialog(); }
    void tickForTesting() { tick(); tick(); }
    void selectLaneForTesting(int i) { if (i >= 0 && i < (int) laneTabs.size()) { laneTabs[(size_t) i]->setToggleState(true, juce::sendNotification); laneTabs[(size_t) i]->onClick(); } }

private:
    static constexpr int kBaseW = 1400, kBaseH = 940;
    StretchCatProcessor& proc; catui::CatLookAndFeel laf; juce::ComponentBoundsConstrainer constrainer; juce::ResizableBorderComponent border { this, &constrainer };
    catui::RenderGate gate; juce::TooltipWindow tooltip { this, 1000 };
    struct Content : juce::Component { StretchCatEditor& o; explicit Content(StretchCatEditor& e) : o(e) {} void paint(juce::Graphics& g) override { o.paintContent(g); } } content { *this };
    sc::SrcView src { proc }; sc::ResView res { proc }; sc::LaneView lane { proc }; sc::MeterView meter;
    sc::Btn play { "Play" }, load { "Load file" }, prev { "<" }, next { ">" }, save { "Save" }, more { "..." }, guide { "Guide" }, settings { "Settings" }, about { "About & Support" };
    sc::Btn pName { "Init" }, byTrans { "By transient" }, even { "Even" }, normalize { "Normalize" }, reverse { "Reverse" }, reset { "Reset" }, flat { "Flat" };
    std::vector<std::unique_ptr<sc::Btn>> pads, laneTabs; std::vector<std::unique_ptr<sc::Knob>> knobs;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtt; std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<sc::Sel>> combos; std::vector<std::unique_ptr<sc::Btn>> toggles;
    sc::Sel linkBox, rootBox, *modeBox = nullptr; sc::Presets presets; int shownRev = -1; bool sizeLabelDirty = true, wasPlaying = false; int lastPads = -1; juce::String shownInfo;
    std::unique_ptr<juce::FileChooser> chooser; juce::uint32 lastTick = 0;

    float scale() const { return (float) getWidth() / (float) kBaseW; }
    void place(juce::Component& c, const layout::Item& it, const juce::String& tip = {}) { c.setBounds(sc::irc(it.r)); if (auto* t = dynamic_cast<juce::SettableTooltipClient*>(&c)) t->setTooltip(tip.isEmpty() ? sc::u8(it.tip) : tip); }
    sc::Knob& addKnob(const char* key, const char* id, std::function<juce::String(double)> fmt, const char* label = nullptr, bool bipolar = false, double mid = 0);
    sc::Btn& addToggle(const char* key, const char* id);
    sc::Sel& addChoice(const char* key, const char* id);
    void chooseFile();
    void tick();
    void paintContent(juce::Graphics& g);
    void refreshPresetName() { pName.setButtonText(proc.presetName()); }
    void showPresetMenu(); void showMoreMenu(); void savePresetDialog(); void showSettings(); void showAbout(); void showGuide();
    juce::String lenText() const;
};

using namespace sc;

sc::Knob& StretchCatEditor::addKnob(const char* key, const char* id, std::function<String(double)> fmt, const char* label, bool bipolar, double mid)
{
    auto k = std::make_unique<sc::Knob>(); const auto& it = layout::find(key); k->it = it; k->label = label ? String(label) : u8(it.text); k->fmt = std::move(fmt); k->bipolar = bipolar; k->midValue = mid;
    content.addAndMakeVisible(*k); k->setBounds(irc(it.r)); k->setTooltip(u8(it.tip).isEmpty() ? String(k->label) : u8(it.tip));
    k->setDoubleClickReturnValue(true, proc.apvts.getParameter(id)->convertFrom0to1(proc.apvts.getParameter(id)->getDefaultValue()));
    sliderAtt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, *k)); knobs.push_back(std::move(k)); return *knobs.back();
}
sc::Btn& StretchCatEditor::addToggle(const char* key, const char* id)
{
    auto b = std::make_unique<sc::Btn>(u8(layout::find(key).text)); const auto& it = layout::find(key); content.addAndMakeVisible(*b); b->setClickingTogglesState(true); b->setBounds(irc(it.r)); b->setTooltip(u8(it.tip));
    buttonAtt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, id, *b)); toggles.push_back(std::move(b)); return *toggles.back();
}
sc::Sel& StretchCatEditor::addChoice(const char* key, const char* id)
{
    auto c = std::make_unique<sc::Sel>(); const auto& it = layout::find(key); content.addAndMakeVisible(*c); auto* ch = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id)); c->addItemList(ch->choices, 1);
    c->setBounds(irc(it.r)); c->setTooltip(u8(it.tip)); comboAtt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, id, *c)); combos.push_back(std::move(c)); return *combos.back();
}

StretchCatEditor::StretchCatEditor(StretchCatProcessor& pr) : AudioProcessorEditor(pr), proc(pr), gate(*this)
{
    setLookAndFeel(&laf); content.setLookAndFeel(&laf); addAndMakeVisible(content);
    auto hdr = [&](sc::Btn& b, const char* key) { content.addAndMakeVisible(b); place(b, layout::find(key)); };
    hdr(prev, "pPrev"); hdr(next, "pNext"); hdr(save, "pSave"); hdr(more, "pMore"); hdr(guide, "bHelp"); hdr(settings, "bSet"); hdr(about, "bAbout"); hdr(pName, "pName");
    pName.setButtonText(proc.presetName()); pName.pad = true;
    hdr(play, "bPlay"); hdr(load, "bFile");
    play.setClickingTogglesState(true); play.onClick = [this] { proc.triggerPlay(play.getToggleState()); play.setButtonText(play.getToggleState() ? "Stop" : "Play"); };
    load.onClick = [this] { chooseFile(); };
    prev.onClick = [this] { presets.step(proc, -1); refreshPresetName(); }; next.onClick = [this] { presets.step(proc, 1); refreshPresetName(); };
    pName.onClick = [this] { showPresetMenu(); }; save.onClick = [this] { savePresetDialog(); }; more.onClick = [this] { showMoreMenu(); };
    guide.onClick = [this] { showGuide(); }; settings.onClick = [this] { showSettings(); }; about.onClick = [this] { showAbout(); };
    // views
    content.addAndMakeVisible(src); content.addAndMakeVisible(res); content.addAndMakeVisible(lane); content.addAndMakeVisible(meter);
    src.setBounds(irc(layout::srcBox)); res.setBounds(irc(layout::resBox)); lane.setBounds(irc(layout::laneBox)); meter.setBounds(irc(layout::cvM));
    src.setTooltip(u8(layout::tipSrc)); res.setTooltip(u8(layout::tipRes)); lane.setTooltip(u8(layout::tipLane)); meter.setTooltip("Output level in dB: left and right peak, with a hold line. Click to reset the hold and the clip marks.");
    // slice row
    for (int i = 0; i < 8; ++i)
    {
        const auto& it = layout::find((String::fromUTF8("sliceRow.C3\xc2\xb7") + String(i + 1)).toRawUTF8()); auto b = std::make_unique<sc::Btn>(u8(it.text)); b->pad = true; content.addAndMakeVisible(*b); b->setBounds(irc(it.r));
        b->setTooltip("Slice " + String(i + 1) + ": press to play it. In Slices to keys mode the MIDI note root + " + String(i) + " plays it too. " + u8(it.tip));
        auto* bp = b.get(); bp->onStateChange = [this, i, bp] { const bool d = bp->isDown(); if (d != (bool) bp->getProperties().getWithDefault("was", false)) { bp->getProperties().set("was", d); proc.triggerSlice(i, d); } };
        pads.push_back(std::move(b));
    }
    auto sbtn = [&](sc::Btn& b, const char* key, std::function<void()> f, const char* tip) { content.addAndMakeVisible(b); place(b, layout::find(key), tip); b.onClick = std::move(f); };
    sbtn(byTrans, "sliceRow.By transient", [this] { proc.slicesByTransient(); }, "Put the slice marks on the strongest hits of the selection (up to 8 slices).");
    sbtn(even, "sliceRow.Even", [this] { proc.slicesEven(); }, "Cut the selection into 8 equal slices.");
    sbtn(normalize, "sliceRow.Normalize", [this] { proc.normalizeSource(); }, "Scale the source so its loudest peak is just below full level.");
    sbtn(reverse, "sliceRow.Reverse", [this] { proc.reverseSource(); }, "Play the source backwards (the selection and slices flip with it). Press again to undo.");
    sbtn(reset, "sliceRow.Reset", [this] { proc.resetSource(); }, "Go back to the sound as it was loaded (undo Normalize and Reverse).");
    // lane tabs
    const char* tabs[] = { "Pitch", "Speed", "Cutoff", "Formant", "Width" }; const char ids[] = { 'p', 's', 'c', 'f', 'w' };
    const char* tabTip[] = { "Transposition over the length of the selection. Drag the dots, click to add one, double-click to remove it.", "Playback speed over time (with Keep pitch it only changes length, with Tape linked it changes pitch too).",
        "Low-pass cutoff over time: close it to make a swell or a wash. Applies live, no new render.", "Formant shift over time: the vowel colour moves while the pitch stays (a real spectral envelope shift).", "Stereo width over time. Applies live, no new render." };
    for (int i = 0; i < 5; ++i)
    {
        auto b = std::make_unique<sc::Btn>(tabs[i]); content.addAndMakeVisible(*b); b->setBounds(irc(layout::find((String("laneTabs.") + tabs[i]).toRawUTF8()).r)); b->setTooltip(tabTip[i]); b->setRadioGroupId(77); b->setClickingTogglesState(true);
        auto* bp = b.get(); const char c = ids[i]; bp->onClick = [this, c] { lane.setLaneId(c); }; if (i == 0) b->setToggleState(true, juce::dontSendNotification); laneTabs.push_back(std::move(b));
    }
    sbtn(flat, "laneTabs.Flat", [this] { lane.flat(); }, "Reset this lane to a straight line.");
    // stretch controls
    modeBox = &addChoice("cSt.Stretch mode", "mode");
    linkBox.addItem("Keep pitch", 1); linkBox.addItem("Tape linked", 2); content.addAndMakeVisible(linkBox); linkBox.setBounds(irc(layout::find("cSt.Link").r));
    linkBox.setTooltip("Keep pitch: speed changes only the length. Tape linked: speed and pitch move together, like a record.");
    linkBox.onChange = [this] { proc.setReal("tape", linkBox.getSelectedId() == 2 ? 1.f : 0.f); };
    addChoice("cSt.Length", "len"); addChoice("cPb.Playback", "pb"); addChoice("Filter.Type", "ft");
    for (int m = 24; m <= 96; ++m) rootBox.addItem(noteName(m), m - 23);
    content.addAndMakeVisible(rootBox); rootBox.setBounds(irc(layout::find("rigHost.MIDI root").r)); rootBox.setTooltip("The MIDI note that plays the sound at its original pitch. In Slices to keys mode this note plays slice 1 and the next notes play the next slices.");
    rootBox.onChange = [this] { proc.setReal("root", (float) (rootBox.getSelectedId() + 23)); };
    auto pct = [](double v) { return String(juce::roundToInt(v * 100)) + " %"; }; auto st = [](double v) { const int r = juce::roundToInt(v); return String(r > 0 ? "+" : "") + String(r) + " st"; };
    addKnob("cSt.Grain", "grain", pct); addKnob("cSt.Transients", "trans", pct); addKnob("cSt.Smooth", "smooth", pct);
    addKnob("cSt.Transpose", "tr", st, nullptr, true, 0); addKnob("cSt.Fine", "fine", [](double v) { const int r = juce::roundToInt(v); return String(r > 0 ? "+" : "") + String(r) + " c"; }, nullptr, true, 0);
    addKnob("cSt.Speed", "speed", [](double v) { return "x" + String(v, 2); });
    addKnob("cPb.Attack", "atk", fmtMs); addKnob("cPb.Release", "rel", fmtMs);
    addKnob("cPb.Glide", "gain", fmtDb, "Gain", true, 0).setTooltip("Output level of the sampler in dB. Double-click resets.");
    addKnob("rigHost.Host BPM", "hostbpm", [](double v) { return String(v, 1); }).setTooltip("Tempo used by Length. It follows the host tempo while a Length is chosen. Double-click resets.");
    addKnob("Filter.Cutoff", "fc", fmtHz); addKnob("Filter.Reso", "fr", [](double v) { return "Q " + String(v, 1); });
    addKnob("EQ.Low", "elo", fmtDb, nullptr, true, 0); addKnob("EQ.Mid", "emi", fmtDb, nullptr, true, 0); addKnob("EQ.High", "ehi", fmtDb, nullptr, true, 0);
    addKnob("Compressor.Thresh", "cth", fmtDb); addKnob("Compressor.Ratio", "crt", [](double v) { return String(v, 1) + ":1"; }); addKnob("Compressor.Attack", "cat", fmtMs); addKnob("Compressor.Release", "crl", fmtMs);
    addKnob("Formant.Shift", "fms", [](double v) { return String(v > 0.05 ? "+" : "") + String(v, 1) + " st"; }, nullptr, true, 0);
    addToggle("Filter.On", "fo"); addToggle("EQ.On", "eo"); addToggle("Compressor.On", "co"); addToggle("Formant.On", "fmo"); addToggle("Formant.Keep", "fmk");
    constrainer.setFixedAspectRatio((double) kBaseW / kBaseH); constrainer.setSizeLimits(kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
    setConstrainer(&constrainer); setResizable(true, false); addAndMakeVisible(border); border.setBorderThickness(juce::BorderSize<int>(6)); setSize(kBaseW, kBaseH);
    tooltip.setMillisecondsBeforeTipAppears(proc.bubbles() ? 1000 : 100000000);
    gate.onFrame = [this] { tick(); }; gate.start(proc.eco() ? 12 : 30);
    tick();                                  // fill the drop downs and pads at once, not only after the first frame
    if (const char* f = std::getenv("CAT_TOUR_LOAD"))   // documentation tool for the screenshots, never set for users
        juce::Timer::callAfterDelay(1500, [sp = juce::Component::SafePointer<StretchCatEditor>(this), path = String(f)] { if (sp != nullptr) sp->proc.loadFile(juce::File(path)); });
}

void StretchCatEditor::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser>("Pick a sound", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) proc.loadFile(fc.getResult()); });
}

String StretchCatEditor::lenText() const
{
    if (!proc.hasSource()) return "";
    if ((int) proc.getReal("len") == 0) return "Free: " + String(proc.resultSeconds(), 2) + " s";
    return String(proc.resultSeconds(), 2) + " s at " + String(proc.getReal("hostbpm"), 1) + " BPM";
}

void StretchCatEditor::tick()
{
    const float dt = 1.f / (proc.eco() ? 12.f : 30.f);
    meter.update(proc.meterL(), proc.meterR(), dt);
    if (proc.dataRevision() != shownRev) { shownRev = proc.dataRevision(); src.repaint(); res.repaint(); lane.repaint(); content.repaint(); }
    const bool playing = proc.playPosition() >= 0.f; if (playing || wasPlaying || proc.rendering()) { src.repaint(); res.repaint(); lane.repaint(); }
    wasPlaying = playing;
    const int sc2 = proc.hasSource() ? proc.slices().count() : 0; if (sc2 != lastPads) { lastPads = sc2; for (int i = 0; i < 8; ++i) pads[(size_t) i]->setEnabled(i < sc2); content.repaint(); }
    const int tape = (int) proc.getReal("tape") != 0 ? 2 : 1; if (linkBox.getSelectedId() != tape) linkBox.setSelectedId(tape, juce::dontSendNotification);
    const int root = (int) proc.getReal("root") - 23; if (rootBox.getSelectedId() != root) rootBox.setSelectedId(root, juce::dontSendNotification);
    meter.repaint();
    const String info = proc.sourceName() + "|" + String(proc.renderMs(), 0) + "|" + lenText() + "|" + String(getWidth()) + String(proc.rendering() ? 1 : 0);
    if (info != shownInfo || sizeLabelDirty) { shownInfo = info; sizeLabelDirty = false; content.repaint(); }
}

void StretchCatEditor::paintContent(juce::Graphics& g)
{
    using namespace layout;
    g.fillAll(kPanel); g.setGradientFill(juce::ColourGradient(Colour(0xff1e1f23), W / 2, 0.f, kPanel, W / 2, 560.f, true)); g.fillRect(0.f, 0.f, W, H);
    auto box = [&](const Rc& r, Colour bg) { g.setColour(bg); g.fillRoundedRectangle(rc(r), 4.f); g.setColour(kLine); g.drawRoundedRectangle(rc(r).reduced(0.5f), 4.f, 1.f); };
    box(rig, kPanel2); box(ctlBox, kPanel2); box(rackcol, kPanel2);
    g.setColour(kLine); g.drawHorizontalLine((int) (header.y + header.h), header.x, header.x + header.w); g.drawHorizontalLine((int) foot.y - 1, foot.x, foot.x + foot.w);
    catui::drawCatHead(g, rc(brandSvg), kInk, kPanel);
    text(g, "STRETCH CAT SAMPLER", { layout::brandName.x, layout::brandName.y, 420.f, layout::brandName.h }, font(F::bold, 30.f), kInk, juce::Justification::centredLeft, 3.f);
    text(g, "STUDIO 87", rc(brandSub), font(F::semi, 12.f), kYellow, juce::Justification::centredLeft, 3.6f);
    // mode tabs: Stretch Sampler is this plugin; the two effects are on the roadmap
    const char* tabName[3] = { "STRETCH SAMPLER", "STRETCH FX", "TRACK EFFECT" };
    const char* tabSub[3] = { "instrument, VST3 and Standalone", "effect: planned, see the roadmap", "effect: engine built, plugin planned" };
    for (int i = 0; i < 3; ++i)
    {
        const bool on = i == 0; g.setColour(on ? kPressed : Colour(0xff17181c)); g.fillRoundedRectangle(rc(modeTab[i]), 3.f); g.setColour(on ? kPurple : kLine); g.drawRoundedRectangle(rc(modeTab[i]).reduced(0.5f), 3.f, 1.f);
        auto r = rc(modeTab[i]); text(g, tabName[i], r.withTrimmedBottom(r.getHeight() * 0.38f), font(F::bold, 17.f), on ? kLav : kDim.withAlpha(0.6f), juce::Justification::centred, 2.8f);
        text(g, tabSub[i], r.withTrimmedTop(r.getHeight() * 0.6f), font(F::label, 11.f), kDim, juce::Justification::centred, 1.2f);
    }
    // rig row: the sample's name in the place of the mockup's source list (the plugin has no test signals)
    text(g, "SAMPLE", { 28.f, 164.f, 70.f, 32.f }, font(F::label, 12.f), kDim, juce::Justification::centredLeft, 2.0f);
    text(g, "SOURCE", { 104.f, 148.f, 100.f, 13.f }, font(F::label, 11.f), kDim, juce::Justification::centredLeft, 2.0f);
    g.setColour(Colour(0xff0a0a0c)); g.fillRoundedRectangle(104.f, 164.f, 144.f, 32.f, 3.f); g.setColour(kLine); g.drawRoundedRectangle(104.5f, 164.5f, 143.f, 31.f, 3.f, 1.f);
    text(g, proc.hasSource() ? proc.sourceName() : String("(none)"), { 112.f, 164.f, 130.f, 32.f }, font(F::label, 15.f), kInk, juce::Justification::centredLeft);
    for (const auto& it : items)
    {
        const String kind(it.kind), key(it.key);
        if (kind == "cap" && key != "rackcol.rackInfo" && key != "rackcol.latInfo")
            text(g, u8(it.text).toUpperCase(), rc(it.r), font(F::label, 11.f), kDim, juce::Justification::centredLeft, 2.0f);
        else if (kind == "head")
        {
            const String sec = key.upToFirstOccurrenceOf(".", false, false);          // the head's own text is all its children run together
            const auto& on = find((sec + ".On").toRawUTF8()); text(g, sec.toUpperCase(), { it.r.x, on.r.y, on.r.x - it.r.x - 6.f, on.r.h }, font(F::label, 11.f), kHead, juce::Justification::centredLeft, 2.2f);
        }
    }
    for (int i = 0; i < 4; ++i) { g.setColour(kLine); g.drawHorizontalLine((int) rackSec[i].y, rackSec[i].x, rackSec[i].x + rackSec[i].w); }
    text(g, proc.rendering() ? "RENDERING..." : (proc.renderMs() > 0 ? "RENDER " + String(juce::roundToInt(proc.renderMs())) + " MS" : String()), rc(find("rackcol.rackInfo").r), font(F::label, 11.f), kDim, juce::Justification::centredRight, 2.0f);
    text(g, "LATENCY 0 MS", rc(find("rackcol.latInfo").r), font(F::label, 11.f), kDim, juce::Justification::centredRight, 2.0f);
    const String ana = proc.hasSource() ? proc.sourceName() + String::fromUTF8("   \xc2\xb7   ") + String(proc.sourceSeconds(), 1) + " s" : "No sound loaded. Press Load file.";
    text(g, ana, rc(anaInfo), font(F::label, 12.f), kDim, juce::Justification::centredRight, 0.5f);
    text(g, lenText(), rc(find("lenInfo").r), font(F::mono, 12.f), kYellow, juce::Justification::centredLeft);
    text(g, String::fromUTF8("STUDIO 87  \xc2\xb7  STRETCH CAT"), rc(footL), font(F::label, 12.f), kDim, juce::Justification::centredLeft, 1.9f);
    text(g, "WINDOW " + String(juce::roundToInt(100.f * scale())) + " %", rc(sizeT), font(F::label, 12.f), kDim, juce::Justification::centredRight, 1.9f);
}

void StretchCatEditor::showPresetMenu()
{
    juce::PopupMenu m = presets.menu(proc);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&pName), [this](int r) { if (r > 0) { presets.choose(proc, r); refreshPresetName(); } });
}
void StretchCatEditor::showMoreMenu()
{
    juce::PopupMenu m; m.addItem(1, "Rename your preset..."); m.addItem(2, "Delete your preset"); m.addSeparator(); m.addItem(3, "Export preset to a file..."); m.addItem(4, "Import preset from a file...");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&more), [this](int r)
    {
        if (r == 1) presets.renameDialog(proc, [this] { refreshPresetName(); });
        else if (r == 2) { presets.deleteCurrent(proc); refreshPresetName(); }
        else if (r == 3) presets.exportDialog(proc, chooser);
        else if (r == 4) presets.importDialog(proc, chooser, [this] { refreshPresetName(); });
    });
}
void StretchCatEditor::savePresetDialog() { presets.saveDialog(proc, [this] { refreshPresetName(); }); }
void StretchCatEditor::showSettings()
{
    juce::PopupMenu m; m.addItem(1, "Info bubbles after 1 s of hovering", true, proc.bubbles()); m.addItem(2, "Power saving (slower meters and animation)", true, proc.eco());
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings), [this](int r)
    {
        if (r == 1) { proc.setBubbles(!proc.bubbles()); tooltip.setMillisecondsBeforeTipAppears(proc.bubbles() ? 1000 : 100000000); }
        if (r == 2) { proc.setEco(!proc.eco()); gate.start(proc.eco() ? 12 : 30); }
    });
}
void StretchCatEditor::showAbout()
{
    auto* w = new juce::AlertWindow("About & Support", "Stretch Cat by Studio 87. Free and open source (AGPL-3.0-or-later). It collects nothing and never connects to the network.\n\n"
        "Version " + String(JucePlugin_VersionString) + ".\nYour presets live in the Studio 87 / StretchCat / presets folder of your user data folder (on Windows %APPDATA%, on macOS Application Support, on Linux ~/.config).\n"
        "To uninstall, use the uninstaller from the download (your presets are kept unless you choose to delete them).\n\nIf it helps your music, a coffee on Ko-fi keeps new plugins coming.", juce::MessageBoxIconType::NoIcon, this);
    w->addButton("Support Studio 87 on Ko-fi", 1); w->addButton("GitHub", 2); w->addButton("Close", 0);
    w->enterModalState(true, juce::ModalCallbackFunction::create([](int r) { if (r == 1) juce::URL("https://ko-fi.com/studio87").launchInDefaultBrowser(); else if (r == 2) juce::URL("https://github.com/SpeakingColors/stretch-cat-vst").launchInDefaultBrowser(); }), true);
}
void StretchCatEditor::showGuide()
{
    auto* w = new juce::AlertWindow("Quick guide", "Load a sound with Load file (wav, aiff, flac, ogg, mp3). Drag the orange handles to choose the part to stretch.\n\n"
        "Stretch mode, Grain, Transients and Smooth shape the stretch; Speed changes the length, Transpose and Fine change the pitch. Length fits the sound to a note value or bars at the Host BPM.\n\n"
        "Slices: Even or By transient puts yellow marks on the source (drag them). Press the C3 pads, or set Playback to Slices to keys and play the MIDI notes from the root note up.\n\n"
        "Over time: draw pitch, speed, cutoff, formant or width along the selection. Pitch, speed and formant make a new render in the background (the old sound keeps playing and fades into the new one); cutoff and width act at once.\n\n"
        "FX rack: filter, EQ, compressor and formant shift after the sampler. Hover a control for help.", juce::MessageBoxIconType::NoIcon, this);
    w->addButton("Close", 0); w->enterModalState(true, nullptr, true);
}

juce::AudioProcessorEditor* StretchCatProcessor::createEditor() { return new StretchCatEditor(*this); }

// Test hooks for tools/snapshot.cpp (screenshots of dialogs and lane tabs); not used by the plugin.
void stretchCatEditorTestHook(juce::AudioProcessorEditor* e, const juce::String& dialog, int lane)
{
    if (auto* ed = dynamic_cast<StretchCatEditor*>(e)) { ed->tickForTesting(); if (lane >= 0) ed->selectLaneForTesting(lane); if (dialog != "none") ed->openDialogForTesting(dialog); }
}
