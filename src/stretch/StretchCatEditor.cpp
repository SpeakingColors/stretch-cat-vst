// Stretch Cat window, built from docs/mockup/stretchcat.html: header, sample bar, waveform of the loaded sound and of the stretched
// result, stretch and playback controls. Fixed aspect ratio, resizable from every corner and edge, wood cheeks.
// Not built yet (see ROADMAP): slice markers and pads, the over-time lanes (pitch, speed, cutoff, formant, width), the FX rack, the Stretch FX effect.
#include "StretchCat.h"
#include <catui/CatLookAndFeel.h>
#include <catui/RenderGate.h>
#include <catui/Logo.h>
#include <cmath>

namespace {
using juce::Colour; using juce::Graphics; using juce::String;
const Colour kBg(0xff101114), kPanel(0xff17181c), kEdge(0xff2c2e35), kText(0xffe8e6e1), kDim(0xff8c909a), kGold(0xffffd100), kTeal(0xff3fd9c0), kPurple(0xffa97bff);
constexpr int kCheek = 30, kContentW = 1140, kBaseW = kContentW + 2 * kCheek, kBaseH = 700;
juce::Font mono(float h, bool bold = false) { return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), h, bold ? juce::Font::bold : juce::Font::plain)); }
juce::Font sans(float h, bool bold = false) { return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain)); }

class Pill : public juce::Button
{
public:
    explicit Pill(const String& t) : juce::Button(t) {}
    void paintButton(Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(0.5f); const bool on = getToggleState();
        g.setColour(on ? kPurple.withAlpha(0.18f) : (over ? Colour(0xff25272e) : Colour(0xff1b1c21))); g.fillRoundedRectangle(r, 3.f);
        g.setColour(on ? kPurple : Colour(0xff3a3d46)); g.drawRoundedRectangle(r, 3.f, on ? 1.5f : 1.f);
        g.setColour(on ? kPurple : (down ? kGold : kText)); g.setFont(sans(13.f, true)); g.drawFittedText(getButtonText().toUpperCase(), getLocalBounds(), juce::Justification::centred, 1);
    }
};

class WavePane : public juce::Component
{
public:
    WavePane(const String& t, Colour c) : title(t), col(c) {}
    void set(const std::vector<float>& p, const String& info) { peaks = p; note = info; repaint(); }
    void paint(Graphics& g) override
    {
        g.fillAll(Colour(0xff060608)); g.setColour(kEdge); g.drawRect(getLocalBounds(), 1);
        g.setColour(kDim); g.setFont(mono(11.f)); g.drawText(title, 8, 4, 200, 14, juce::Justification::left);
        g.drawText(note, getWidth() - 408, 4, 400, 14, juce::Justification::right);
        if (peaks.empty()) { g.setFont(sans(16.f)); g.drawFittedText("No sound loaded. Press Load file.", getLocalBounds(), juce::Justification::centred, 1); return; }
        const float mid = getHeight() * 0.5f + 6, amp = getHeight() * 0.5f - 18; const int n = (int) peaks.size(), w = getWidth() - 2;
        g.setColour(col);
        for (int x = 0; x < w; ++x) { const float p = std::min(1.f, peaks[(size_t) (x * n / w)]); const float h = std::max(1.f, p * amp); g.fillRect((float) x + 1, mid - h, 1.f, 2 * h); }
    }
private:
    std::vector<float> peaks; String title, note; Colour col;
};

struct Knob
{
    juce::Slider s; juce::Label cap; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a;
    void init(juce::Component& parent, juce::AudioProcessorValueTreeState& apvts, const char* id, const String& name, const String& tip)
    {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 18); s.setTooltip(tip);
        s.setColour(juce::Slider::textBoxTextColourId, kPurple); s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        cap.setText(name.toUpperCase(), juce::dontSendNotification); cap.setJustificationType(juce::Justification::centred); cap.setFont(sans(12.f, true)); cap.setColour(juce::Label::textColourId, kDim);
        parent.addAndMakeVisible(s); parent.addAndMakeVisible(cap); a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, s);
    }
    void place(int x, int y) { cap.setBounds(x, y, 96, 16); s.setBounds(x, y + 16, 96, 96); }
};

class StretchCatEditor : public juce::AudioProcessorEditor
{
public:
    explicit StretchCatEditor(StretchCatProcessor& p) : AudioProcessorEditor(p), proc(p), gate(*this)
    {
        setLookAndFeel(&laf); auto& c = content; addAndMakeVisible(c);
        for (auto* b : { &load, &guide, &about }) c.addAndMakeVisible(*b);
        load.setTooltip("Load an audio file (wav, aiff, flac, ogg, mp3)."); load.onClick = [this] { chooseFile(); };
        guide.onClick = [] { juce::URL("https://github.com/SpeakingColors/stretch-cat-vst/blob/main/docs/Stretch-Cat-manual.pdf").launchInDefaultBrowser(); };
        about.onClick = [] { juce::URL("https://ko-fi.com/studio87").launchInDefaultBrowser(); };
        c.addAndMakeVisible(src); c.addAndMakeVisible(res);
        auto combo = [&](juce::ComboBox& cb, const char* id, const String& tip) { for (int i = 0; i < 1; ++i) {} c.addAndMakeVisible(cb); auto* ch = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id)); cb.addItemList(ch->choices, 1); cb.setTooltip(tip);
            combos.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, id, cb)); };
        combo(mode, "mode", "How the sound is stretched. Beats keeps drums sharp, Tones suits melodies, Texture suits pads and noise, Re-Pitch is tape style, Complex handles mixes, Formant+ keeps voices natural.");
        combo(pb, "pb", "Loop repeats while the key is held, One shot plays to the end, Gate stops at key release.");
        combo(len, "len", "Auto keeps the Speed knob. A note or bar length stretches the sound to fit that length at the Fit tempo.");
        c.addAndMakeVisible(tape); tape.setButtonText("Speed and pitch linked"); tape.setTooltip("On: speed also changes pitch, like tape.");
        tapeA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "tape", tape);
        tape.setColour(juce::ToggleButton::textColourId, kText); tape.setColour(juce::ToggleButton::tickColourId, kPurple);
        auto& ap = proc.apvts;
        k[0].init(c, ap, "grain", "Grain", "Grain size of the stretch engine."); k[1].init(c, ap, "trans", "Transients", "How much attack detail is protected.");
        k[2].init(c, ap, "smooth", "Smooth", "Smoothing between grains."); k[3].init(c, ap, "speed", "Speed", "Playback speed, 1.00 is original. Ignored while a Length is chosen.");
        k[4].init(c, ap, "tr", "Transpose", "Pitch shift in semitones."); k[5].init(c, ap, "fine", "Fine", "Pitch shift in cents.");
        k[6].init(c, ap, "atk", "Attack", "Fade in time in milliseconds."); k[7].init(c, ap, "rel", "Release", "Fade out time in milliseconds.");
        k[8].init(c, ap, "gain", "Gain", "Output level in dB."); k[9].init(c, ap, "root", "Root note", "The MIDI note that plays the sound at its original pitch (60 is C3 in most hosts, C4 in some).");
        k[10].init(c, ap, "hostbpm", "Fit tempo", "Tempo used by Length. Follows the host tempo while a Length is chosen.");
        auto fmt = [](Knob& kb, std::function<String(double)> f) { kb.s.textFromValueFunction = f; kb.s.updateText(); };
        const char* nn[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        for (int i : { 0, 1, 2 }) fmt(k[i], [](double v) { return String(juce::roundToInt(v * 100)) + " %"; });
        fmt(k[3], [](double v) { return "x" + String(v, 2); });
        fmt(k[4], [](double v) { return String(juce::roundToInt(v)) + " st"; });
        fmt(k[5], [](double v) { return String(juce::roundToInt(v)) + " c"; });
        fmt(k[6], [](double v) { return String(v, 1) + " ms"; }); fmt(k[7], [](double v) { return String(juce::roundToInt(v)) + " ms"; });
        fmt(k[8], [](double v) { return String(v, 1) + " dB"; });
        fmt(k[9], [nn](double v) { const int m = juce::roundToInt(v); return String(nn[((m % 12) + 12) % 12]) + String(m / 12 - 2) + "  (" + String(m) + ")"; });
        fmt(k[10], [](double v) { return String(v, 1) + " BPM"; });
        constrainer.setFixedAspectRatio((double) kBaseW / kBaseH); constrainer.setSizeLimits(kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
        setConstrainer(&constrainer); setResizable(true, true); setSize(kBaseW, kBaseH);
        gate.onFrame = [this] { tick(); }; gate.start(10);
        if (const char* f = std::getenv("CAT_TOUR_LOAD"))   // documentation tool for the screenshots, never set for users
            juce::Timer::callAfterDelay(1500, [sp = juce::Component::SafePointer<StretchCatEditor>(this), path = String(f)] { if (sp != nullptr) sp->proc.loadFile(juce::File(path)); });
    }
    ~StretchCatEditor() override { setLookAndFeel(nullptr); }
    void paint(Graphics& g) override { g.fillAll(kBg); catui::drawWoodCheeks(g, getLocalBounds(), juce::roundToInt(kCheek * scale())); }
    void resized() override { const float s = scale(); content.setBounds(juce::roundToInt(kCheek * s), 0, kContentW, kBaseH); content.setTransform(juce::AffineTransform::scale(s)); layout(); }
    void mouseDoubleClick(const juce::MouseEvent& e) override { const auto p = e.getEventRelativeTo(this).getPosition(); if (p.x < kCheek * scale() || p.x > getWidth() - kCheek * scale()) setSize(kBaseW, kBaseH); }

private:
    StretchCatProcessor& proc; catui::CatLookAndFeel laf; juce::ComponentBoundsConstrainer constrainer; catui::RenderGate gate;
    struct Content : juce::Component { StretchCatEditor& o; explicit Content(StretchCatEditor& e) : o(e) {} void paint(Graphics& g) override { o.paintContent(g); } } content { *this };
    Pill load { "Load file" }, guide { "Guide" }, about { "About & Support" };
    WavePane src { "SOURCE", kPurple }, res { "RESULT", kTeal };
    juce::ComboBox mode, pb, len; juce::ToggleButton tape; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tapeA;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos; Knob k[11];
    int shownRev = -1; String shownStatus; std::unique_ptr<juce::FileChooser> chooser;

    float scale() const { return (float) getWidth() / (float) kBaseW; }
    void layout()
    {
        guide.setBounds(900, 20, 90, 34); about.setBounds(1000, 20, 130, 34); load.setBounds(14, 96, 130, 36);
        src.setBounds(14, 148, 1112, 130); res.setBounds(14, 288, 1112, 110);
        mode.setBounds(14, 452, 170, 30); tape.setBounds(14, 492, 200, 24);
        for (int i = 0; i < 6; ++i) k[i].place(240 + i * 150, 420);
        pb.setBounds(14, 584, 170, 30); len.setBounds(14, 634, 170, 30);
        for (int i = 6; i < 11; ++i) k[i].place(240 + (i - 6) * 150, 548);
    }
    void chooseFile()
    {
        chooser = std::make_unique<juce::FileChooser>("Pick a sound", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) proc.loadFile(fc.getResult()); });
    }
    void tick()
    {
        if (proc.dataRevision() != shownRev)
        {
            shownRev = proc.dataRevision();
            src.set(proc.srcPeaks(), proc.sourceName() + "  " + String(proc.sourceSeconds(), 2) + " s"); res.set(proc.resultPeaks(), String(proc.resultSeconds(), 2) + " s");
        }
        if (proc.status() != shownStatus) { shownStatus = proc.status(); content.repaint(); }
    }
    void label(Graphics& g, const String& t, int x, int y) { g.setColour(kDim); g.setFont(sans(12.f, true)); g.drawText(t.toUpperCase(), x, y, 200, 16, juce::Justification::left); }
    void paintContent(Graphics& g)
    {
        g.fillAll(kBg);
        catui::drawCatHead(g, { 22, 14, 60, 56 }, kText, kBg);
        g.setColour(kText); g.setFont(sans(34.f, true)); g.drawText("STRETCH CAT", 96, 8, 400, 46, juce::Justification::left);
        g.setColour(kGold); g.setFont(sans(14.f, true)); g.drawText("STUDIO 87", 98, 52, 200, 20, juce::Justification::left);
        g.setColour(kEdge); g.drawHorizontalLine(84, 14.f, 1126.f);
        g.setColour(kDim); g.setFont(mono(13.f)); g.drawText(shownStatus, 160, 96, 960, 36, juce::Justification::centredLeft);
        label(g, "Stretch mode", 14, 432); label(g, "Playback", 14, 564); label(g, "Length", 14, 614);
        g.setColour(kDim); g.setFont(mono(11.f)); g.drawText("STUDIO 87 | STRETCH CAT | hover a control for help", 14, 676, 700, 16, juce::Justification::left);
    }
};
}
juce::AudioProcessorEditor* StretchCatProcessor::createEditor() { return new StretchCatEditor(*this); }
