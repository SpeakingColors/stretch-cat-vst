// Stretch Cat Track Effect window: the Track Effect page of docs/mockup/stretchcat.html. Positions, sizes, texts and tooltips are the
// mockup's (read from the page with Playwright: canvas 1312 x 400, four boxes of 322 x 255, knobs 84 x 77, same colours and fonts).
// The mockup's mode tabs, sample rig and presets belong to the sampler and are left out; the host saves and loads presets.
// Fixed aspect ratio, resizable from every corner and edge, wood cheeks, info bubbles after 1 s, drawing paused by catui::RenderGate.
#include "../stretch/EditorWidgets.h"
#include "TrackFxProcessor.h"
#include "TrackFxEditorFactory.h"
#include <catui/CatLookAndFeel.h>
#include <catui/RenderGate.h>
#include <catui/Logo.h>
#include <cmath>

namespace
{
using namespace sc;
constexpr float kPanelW = 1340.f, kH = 830.f, kCheek = 30.f;
constexpr int kBaseW = 1400, kBaseH = 830;
const layout::Rc kCanvas { 14.f, 77.f, 1312.f, 400.f }, kMeterR { 14.f, 748.f, 1312.f, 40.f }, kHeader { 14.f, 10.f, 1312.f, 57.f }, kFoot { 14.f, 798.f, 1312.f, 23.f };
const float kBoxX[4] = { 14.f, 344.f, 674.f, 1004.f }; constexpr float kBoxY = 485.f, kBoxW = 322.f, kBoxH = 255.f;
const char* kBoxName[4] = { "Time", "Grains", "Effects", "Transients" };

// ---- the 8 second history with the held part, as the mockup's canvas
class TrackView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit TrackView(TrackFxProcessor& pr) : p(pr) {}
    void paint(Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight(); g.fillAll(kDisp);
        const float secs = 8.f, l = 40.f, r = w - 10.f, top = 10.f, bot = h - 18.f, mid = (top + bot) / 2.f;
        for (int i = 0; i <= 8; ++i) { const float x = r - (r - l) * (float) i / 8.f; g.setColour(kGrid); g.fillRect(std::round(x), top, 1.f, bot - top);
            text(g, i == 0 ? "now" : "-" + String(i) + " s", { x - 20.f, h - 16.f, 40.f, 12.f }, font(F::mono, 10.f), kTick); }
        // history: newest column on the right edge
        const int wi = p.writeIndex(), n = TrackFxProcessor::kCols; g.setColour(kPurple);
        for (float px = l; px < r; px += 2.f)
        {
            const int col = juce::jlimit(0, n - 1, (int) ((px - l) / (r - l) * (float) n)); const int back = n - 1 - col;      // columns back from the newest
            const int idx = ((wi - 1 - back) % n + n) % n; if (wi - 1 - back < 0) continue;
            const float amp = std::pow(juce::jlimit(0.f, 1.f, p.historyAt(idx)), 0.6f) * (bot - top) * 0.5f;       // a little compression so quiet parts show
            g.fillRect(px, mid - amp, 1.4f, std::max(1.f, 2.f * amp));
        }
        const float start = (float) p.apvts.getRawParameterValue("start")->load(), len = (float) p.apvts.getRawParameterValue("len")->load();
        const bool loop = p.apvts.getRawParameterValue("loop")->load() > 0.5f, held = p.holding();
        const float sx = r - (r - l) * (start / 1000.f) / secs, ex = std::min(r, sx + (r - l) * (len / 1000.f) / secs);
        g.setColour(kOrange.withAlpha(0.14f)); g.fillRect(sx, top, ex - sx, bot - top); g.setColour(kOrange); g.fillRect(sx - 2.f, top, 4.f, bot - top); g.fillRect(ex - 1.f, top, 2.f, bot - top); g.fillRect(sx - 2.f, top, 9.f, 8.f);
        const float hl = p.heldLengthMs(), ph = p.playheadMs(); const bool done = held && !loop && hl > 0.f && ph >= hl - 1.f;
        text(g, held ? (done ? "LIVE" : "HOLDING") : "LISTENING", { l + 6.f, top, 160.f, 14.f }, font(F::mono, 10.f), kLav, juce::Justification::left);
        if (held && hl > 0.f) { const float x = sx + (r - l) * (ph / 1000.f) / secs; g.setColour(done ? kTick : juce::Colours::white); g.fillRect(x, top, 2.f, bot - top);
            if (done) text(g, "played once, live signal is back", { x + 6.f, top + 14.f, 260.f, 14.f }, font(F::mono, 10.f), kLav, juce::Justification::left); }
        else { g.setColour(kTick); g.fillRect(r - 1.f, top, 2.f, bot - top); }
        g.setColour(kLine); g.drawRect(getLocalBounds().toFloat(), 1.f);
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        const float x = e.position.x, l = 40.f, r = (float) getWidth() - 10.f; const float start = get("start"), len = get("len");
        const float sx = r - (r - l) * (start / 1000.f) / 8.f, ex = sx + (r - l) * (len / 1000.f) / 8.f;
        mode = std::fabs(x - ex) < 9.f && std::fabs(x - ex) < std::fabs(x - sx) ? 2 : 1; mouseDrag(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        const float l = 40.f, r = (float) getWidth() - 10.f; const float x = juce::jlimit(l, r, e.position.x);
        if (mode == 1) set("start", juce::jlimit(0.f, 8000.f, (r - x) / (r - l) * 8000.f));
        else if (mode == 2) { const float sx = r - (r - l) * (get("start") / 1000.f) / 8.f; set("len", juce::jlimit(20.f, 4000.f, (x - sx) / (r - l) * 8000.f)); }
    }
    void mouseUp(const juce::MouseEvent&) override { mode = 0; }
private:
    TrackFxProcessor& p; int mode = 0;
    float get(const char* id) const { return p.apvts.getRawParameterValue(id)->load(); }
    void set(const char* id, float v) { if (auto* q = p.apvts.getParameter(id)) q->setValueNotifyingHost(q->convertTo0to1(v)); }
};

// ---- output level in dB (same meter as the sampler's)
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
    void mouseDown(const juce::MouseEvent&) override { clipL = clipR = false; hl = hr = -90.f; }
    void paint(Graphics& g) override
    {
        const float w = (float) getWidth(), x0 = 18.f, x1 = w - 8.f; auto Xd = [&](float d) { return x0 + (juce::jlimit(-54.f, 3.f, d) + 54.f) / 57.f * (x1 - x0); };
        g.fillAll(Colour(0xff06080a)); g.setColour(kLine); g.drawRect(getLocalBounds().toFloat(), 1.f);
        const float barH = 9.f, yL = 4.f, yR = 4.f + barH + 2.f, yS = yR + barH + 1.f;
        for (int d = -54; d <= 0; d += 6) { const float x = std::round(Xd((float) d)) + 0.5f; g.setColour(kGrid); g.drawVerticalLine((int) x, yL, yS); g.setColour(Colour(0xff3a3f4a)); g.drawVerticalLine((int) x, yS, yS + 3.f);
            if (d == 0) text(g, "0 dB", { x - 28.f, yS + 2.f, 46.f, 12.f }, font(F::mono, 10.f), Colour(0xffffb35c), juce::Justification::right);
            else text(g, String(d), { x - 14.f, yS + 2.f, 28.f, 12.f }, font(F::mono, 10.f), d >= -6 ? Colour(0xffffb35c) : Colour(0xff8f8bd6)); }
        auto bar = [&](float y, float val, float hold, bool clip, const char* name)
        {
            g.setColour(Colour(0xff0e1114)); g.fillRect(x0, y, x1 - x0, barH);
            const float xv = Xd(val), z0 = Xd(-54.f), z1 = Xd(-12.f), z2 = Xd(0.f), z3 = Xd(3.f);
            auto seg = [&](float a, float b, Colour c) { const float e = std::min(b, xv); if (e > a) { g.setColour(c); g.fillRect(a, y, e - a, barH); } };
            seg(z0, z1, Colour(0xff6cc9f5)); seg(z1, z2, Colour(0xffa43dff)); seg(z2, z3, kPink);
            g.setColour(clip ? kPink : juce::Colours::white); g.fillRect(Xd(hold) - 1.f, y, 2.f, barH); if (clip) g.fillRect(x1 - 5.f, y, 5.f, barH);
            text(g, name, { 2.f, y, 14.f, barH }, font(F::mono, 10.f), Colour(0xffa9adb8));
        };
        bar(yL, l, hl, clipL, "L"); bar(yR, r, hr, clipR, "R");
    }
private:
    bool clipL = false, clipR = false;
};

layout::Item knobItem(float x, float y)
{
    return { "", "knob", "", { x, y, 84.f, 77.f }, { x + 20.f, y, 44.f, 44.f }, { x, y + 46.f, 84.f, 13.f }, { x, y + 61.f, 84.f, 16.f }, { 0, 0, 0, 0 }, "" };
}
String keyText(int q)
{
    String s = (q > 0 ? "+" : "") + String(q) + " st";
    if (q == 0) s += " (original)"; else if (q == 12) s += " (octave up)"; else if (q == -12) s += " (octave down)"; else if (q == 24) s += " (2 octaves up)"; else if (q == -24) s += " (2 octaves down)"; else if (q == 7) s += " (fifth up)"; else if (q == -7) s += " (fifth down)";
    return s;
}
String ms0(double v) { return String(juce::roundToInt(v)) + " ms"; }
String pct(double v) { return String(juce::roundToInt(v * 100)) + " %"; }

class TrackFxEditor : public juce::AudioProcessorEditor
{
public:
    explicit TrackFxEditor(TrackFxProcessor& pr) : AudioProcessorEditor(pr), proc(pr), gate(*this)
    {
        setLookAndFeel(&laf); content.setLookAndFeel(&laf); addAndMakeVisible(content);
        setResizable(true, true); constrainer.setFixedAspectRatio((double) kBaseW / (double) kBaseH); constrainer.setSizeLimits(kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
        setConstrainer(&constrainer); addAndMakeVisible(border); border.setAlwaysOnTop(true); border.setInterceptsMouseClicks(false, true);
        setSize(kBaseW, kBaseH);
        content.addAndMakeVisible(view); view.setBounds(irc(kCanvas));
        view.setTooltip("The last 8 seconds of your track. Drag the orange marker to set where the held part starts (measured back from now) and its right edge to set the length.");
        content.addAndMakeVisible(meter); meter.setBounds(irc(kMeterR)); meter.setTooltip("Output level of the plugin in dB: left and right peak, with a hold line. Click to reset the hold and the clip marks.");
        auto box = [&](int b, float x, float y) { return std::make_pair(kBoxX[b] + x, kBoxY + y); };
        // Time
        { auto [x, y] = box(0, 9.f, 28.f); addToggle("hold", "HOLD", { x, y, 112.f, 56.f }, 22.f, "Freeze the last moments of the track and repeat the held part. Release to return to the live signal. Automatable, so you can hold on a beat."); }
        { auto [x, y] = box(0, 128.9f, 28.f); addToggle("loop", "Loop", { x, y, 51.6f, 32.f }, 13.f, "On: the held part repeats until you release HOLD. Off: it plays once, then the live signal returns."); }
        { auto [x, y] = box(0, 188.5f, 28.f); addKnob("start", "Start", x, y, ms0, "Where the held part begins, measured back from the moment you press HOLD (0 = now, up to 8 s back)."); }
        { auto [x, y] = box(0, 9.f, 113.f); addKnob("len", "Length", x, y, ms0, "Length of the held part."); }
        { auto [x, y] = box(0, 101.f, 113.f); addKnob("speed", "Speed", x, y, [](double v) { return "x" + String(v, 2); }, "How fast the playhead moves through the held part. Slow gives a freeze, fast a stutter. The pitch stays the same."); }
        { auto [x, y] = box(0, 9.f, 198.f); keyBox.setBounds(irc({ x, y + 16.f, 156.f, 32.f })); for (int q = 24; q >= -24; --q) keyBox.addItem(keyText(q), 100 + q);
          content.addAndMakeVisible(keyBox); keyBox.setTooltip("Transposes the grains in semitones, like a playback speed change of each grain, so the grain length changes with it.");
          keyCaption = { x, y, 156.f, 14.f }; keyAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, "key", keyBox); }
        // Grains
        { auto [x, y] = box(1, 9.f, 28.f); addKnob("atk", "Attack", x, y, ms0, "Fade in and fade out time of every grain. Short is rough and clicky-free, long is soft."); }
        { auto [x, y] = box(1, 101.f, 28.f); addKnob("hld", "Hold", x, y, ms0, "How long each grain stays at full level between its fades."); }
        { auto [x, y] = box(1, 193.f, 28.f); addKnob("spc", "Grain spacing", x, y, ms0, "Time between the starts of two grains. Smaller than the grain length makes them overlap."); }
        { auto [x, y] = box(1, 9.f, 113.f); addKnob("wav", "Wave spacing", x, y, [](double v) { return String(v, 1) + " ms"; }, "Extra shift of the read position for each next grain. It smears or stacks copies of the wave (approximation, see the manual)."); }
        // Effects
        { auto [x, y] = box(2, 9.f, 28.f); addKnob("pan", "Pan", x, y, [](double v) { return v == 0 ? String("C") : (v < 0 ? "L " : "R ") + String(juce::roundToInt(std::fabs(v) * 100)); }, "Where the grains sit between left and right.", true, 0.0); }
        { auto [x, y] = box(2, 101.f, 28.f); addKnob("depth", "Depth", x, y, pct, "Spreads the grains left and right and varies their level (approximation, see the manual)."); }
        { auto [x, y] = box(2, 193.f, 28.f); addKnob("rand", "Rand", x, y, pct, "Random jitter of grain timing and read position (up to 20 ms)."); }
        // Transients
        { const char* nm[3] = { "Off", "Use regions", "Detect" };
          const char* tp[3] = { "Off: the held part is one piece.", "Use regions: the held part is cut into 8 equal regions (this plugin has no slice markers).", "Detect: transients in the held audio are found automatically." };
          for (int i = 0; i < 3; ++i) { auto [x, y] = box(3, 9.f, 44.f + 36.f * (float) i); auto b = std::make_unique<sc::Btn>(nm[i]); content.addAndMakeVisible(*b); b->setBounds(irc({ x, y, 94.4f, 32.f })); b->setClickingTogglesState(false);
            b->setTooltip(String("Mode. ") + tp[i]); b->onClick = [this, i] { if (auto* q = proc.apvts.getParameter("mode")) q->setValueNotifyingHost(q->convertTo0to1((float) i)); }; modeBtns.push_back(std::move(b)); }
          modeCaption = { kBoxX[3] + 9.f, kBoxY + 28.f, 94.f, 14.f }; }
        { auto [x, y] = box(3, 111.4f, 28.f); addKnob("amt", "Hold amount", x, y, pct, "How much of each region is repeated. Small repeats only the hit itself."); }
        for (auto* b : { &guide, &about }) content.addAndMakeVisible(*b);
        guide.setBounds(irc({ 1120.f, 24.f, 74.f, 32.f })); about.setBounds(irc({ 1200.f, 24.f, 124.f, 32.f })); guide.setTooltip("Quick guide: what each part does."); about.setTooltip("About Studio 87, privacy, and ways to support the project.");
        guide.onClick = [this] { showGuide(); }; about.onClick = [this] { showAbout(); };
        gate.onFrame = [this] { tick(); }; gate.start(30);
    }
    ~TrackFxEditor() override { setLookAndFeel(nullptr); content.setLookAndFeel(nullptr); }
    void paint(Graphics& g) override { g.fillAll(kBg); catui::drawWoodCheeks(g, getLocalBounds(), juce::roundToInt(kCheek * scale())); }
    void resized() override { const float s = scale(); content.setBounds(juce::roundToInt(kCheek * s), 0, (int) kPanelW, (int) kH); content.setTransform(juce::AffineTransform::scale(s)); border.setBounds(getLocalBounds()); }
    void mouseDoubleClick(const juce::MouseEvent& e) override { const auto p = e.getEventRelativeTo(this).getPosition(); if (p.x < kCheek * scale() || p.x > getWidth() - kCheek * scale()) setSize(kBaseW, kBaseH); }
    void tickForTesting() { tick(); tick(); }

private:
    TrackFxProcessor& proc; catui::CatLookAndFeel laf; juce::ComponentBoundsConstrainer constrainer; juce::ResizableBorderComponent border { this, &constrainer };
    catui::RenderGate gate; juce::TooltipWindow tooltip { this, 1000 };
    struct Content : juce::Component { TrackFxEditor& o; explicit Content(TrackFxEditor& e) : o(e) {} void paint(Graphics& g) override { o.paintContent(g); } } content { *this };
    TrackView view { proc }; MeterView meter; sc::Sel keyBox; sc::Btn guide { "Guide" }, about { "About & Support" };
    std::vector<std::unique_ptr<sc::Knob>> knobs; std::vector<std::unique_ptr<sc::Btn>> toggles, modeBtns;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtt; std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt; layout::Rc keyCaption {}, modeCaption {}; juce::uint32 lastTick = 0;

    float scale() const { return (float) getWidth() / (float) kBaseW; }
    void addKnob(const char* id, const char* label, float x, float y, std::function<String(double)> fmt, const char* tip, bool bipolar = false, double mid = 0)
    {
        auto k = std::make_unique<sc::Knob>(); k->it = knobItem(x, y); k->label = label; k->fmt = std::move(fmt); k->bipolar = bipolar; k->midValue = mid;
        content.addAndMakeVisible(*k); k->setBounds(irc(k->it.r)); k->setTooltip(tip);
        k->setDoubleClickReturnValue(true, proc.apvts.getParameter(id)->convertFrom0to1(proc.apvts.getParameter(id)->getDefaultValue()));
        sliderAtt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, *k)); knobs.push_back(std::move(k));
    }
    void addToggle(const char* id, const char* label, layout::Rc r, float px, const char* tip)
    {
        auto b = std::make_unique<sc::Btn>(label, px); content.addAndMakeVisible(*b); b->setClickingTogglesState(true); b->setBounds(irc(r)); b->setTooltip(tip);
        buttonAtt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, std::string(id) == "HOLD" ? "hold" : "loop", *b)); toggles.push_back(std::move(b));
    }
    void tick()
    {
        const auto now = juce::Time::getMillisecondCounter(); const float dt = lastTick ? std::min(0.1f, (float) (now - lastTick) * 0.001f) : 0.033f; lastTick = now;
        meter.update(proc.meterL(), proc.meterR(), dt);
        const int m = juce::roundToInt(proc.apvts.getRawParameterValue("mode")->load()); for (size_t i = 0; i < modeBtns.size(); ++i) modeBtns[i]->setToggleState((int) i == m, juce::dontSendNotification);
        view.repaint(); meter.repaint(); content.repaint(juce::Rectangle<int>(0, 0, (int) kPanelW, 80));
    }
    void paintContent(Graphics& g)
    {
        g.fillAll(kBg); g.setColour(kPanel); g.fillRect(0.f, 0.f, kPanelW, kH);
        // header: logo and name as the mockup's brand
        catui::drawCatHead(g, rc({ 14.f, 14.f, 38.f, 38.f }), kInk, kPanel);
        text(g, "Stretch Cat Track Effect", { 62.f, 10.f, 500.f, 34.f }, font(F::bold, 28.f), kInk, juce::Justification::left, 1.f);
        text(g, "STUDIO 87", { 62.f, 42.f, 294.f, 14.f }, font(F::label, 11.f), kDim, juce::Justification::left, 2.f);
        text(g, "effect: hold and repeat", { 560.f, 24.f, 400.f, 20.f }, font(F::label, 14.f), kLav, juce::Justification::left, 1.f);
        for (int b = 0; b < 4; ++b)
        {
            const layout::Rc r { kBoxX[b], kBoxY, kBoxW, kBoxH }; g.setColour(kPanel2); g.fillRoundedRectangle(rc(r), 4.f); g.setColour(kLine); g.drawRoundedRectangle(rc(r).reduced(0.5f), 4.f, 1.f);
            text(g, String(kBoxName[b]).toUpperCase(), { kBoxX[b] + 9.f, kBoxY + 7.f, 200.f, 14.f }, font(F::semi, 11.f), kLav, juce::Justification::left, 2.2f);
        }
        text(g, "KEY TO (PITCH)", rc(keyCaption), font(F::label, 11.f), kDim, juce::Justification::left, 1.5f);
        text(g, "MODE", rc(modeCaption), font(F::label, 11.f), kDim, juce::Justification::left, 1.5f);
        text(g, "OUTPUT LEVEL, dB", { 14.f, 730.f, 200.f, 14.f }, font(F::label, 11.f), kDim, juce::Justification::left, 1.5f);
        text(g, u8("Studio 87 \xc2\xb7 Stretch Cat Track Effect v") + String(JucePlugin_VersionString) + u8("  \xc2\xb7  latency 0 samples"), { 14.f, 802.f, 800.f, 14.f }, font(F::label, 12.f), kDim, juce::Justification::left, 0.5f);
        text(g, "Window " + String(juce::roundToInt(scale() * 100.f)) + " %", { 1224.f, 802.f, 100.f, 14.f }, font(F::label, 12.f), kDim, juce::Justification::right, 0.5f);
    }
    void showAbout()
    {
        auto* w = new juce::AlertWindow("About & Support", "Stretch Cat Track Effect by Studio 87. Free and open source (AGPL-3.0-or-later). It collects nothing and never connects to the network.\n\n"
            "Version " + String(JucePlugin_VersionString) + ".\nTo uninstall, use the uninstaller from the download.\n\nIf it helps your music, a coffee on Ko-fi keeps new plugins coming.", juce::MessageBoxIconType::NoIcon, this);
        w->addButton("Support Studio 87 on Ko-fi", 1); w->addButton("GitHub", 2); w->addButton("Close", 0);
        w->enterModalState(true, juce::ModalCallbackFunction::create([](int r) { if (r == 1) juce::URL("https://ko-fi.com/studio87").launchInDefaultBrowser(); else if (r == 2) juce::URL("https://github.com/SpeakingColors/stretch-cat-vst").launchInDefaultBrowser(); }), true);
    }
    void showGuide()
    {
        auto* w = new juce::AlertWindow("Quick guide", "Put the plugin on a track. It listens to the last 8 seconds in the background and passes your sound untouched.\n\n"
            "Press HOLD (or automate it) to freeze that moment and repeat it as grains. Release to go back to the live signal.\n\n"
            "Time: Start is how far back the held part begins, Length how long it is, Speed how fast the playhead moves through it (the pitch stays), Key transposes the grains. Loop off plays the part once.\n\n"
            "Grains: Attack and Hold shape each grain, Grain spacing sets how often one starts. Effects: Pan, Depth and Rand spread and scatter them.\n\n"
            "Transients: Use regions cuts the held part into 8 equal pieces, Detect finds the hits in it; Hold amount repeats only the start of each piece.\n\nHover a control for help.", juce::MessageBoxIconType::NoIcon, this);
        w->addButton("Close", 0); w->enterModalState(true, nullptr, true);
    }
};
}

juce::AudioProcessorEditor* makeTrackFxEditor(TrackFxProcessor& p) { return new TrackFxEditor(p); }
