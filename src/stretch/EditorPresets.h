// Preset bar logic for the Stretch Cat editor: factory presets (built in) and user presets (files in Studio 87/Stretch Cat/presets).
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "StretchCat.h"

namespace sc
{
class Presets
{
public:
    static juce::File dir() { auto d = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Studio 87").getChildFile("StretchCat").getChildFile("presets"); d.createDirectory(); return d; }

    struct Factory { const char* name; std::vector<std::pair<const char*, float>> v; };
    static const std::vector<Factory>& factory()
    {
        static const std::vector<Factory> f = {
            { "Init", {} },
            { "Half Speed Wash", { { "speed", 0.5f }, { "smooth", 0.7f }, { "grain", 0.7f } } },
            { "Tape Slowdown", { { "tape", 1.f }, { "speed", 0.5f } } },
            { "Octave Down Keep Length", { { "tr", -12.f }, { "fmo", 1.f }, { "fmk", 1.f } } },
            { "Vowel Shift", { { "fmo", 1.f }, { "fms", 5.f }, { "fmk", 0.f } } },
            { "Dark Pad", { { "speed", 0.25f }, { "smooth", 0.9f }, { "fo", 1.f }, { "fc", 1800.f }, { "atk", 120.f }, { "rel", 600.f } } },
            { "Punchy Slices", { { "pb", 3.f }, { "trans", 0.9f }, { "co", 1.f }, { "cth", -18.f }, { "crt", 4.f } } } };
        return f;
    }

    juce::PopupMenu menu(StretchCatProcessor& p)
    {
        juce::PopupMenu m; int id = 1; m.addSectionHeader("Factory");
        for (auto& f : factory()) m.addItem(id++, f.name, true, p.presetName() == f.name);
        users = dir().findChildFiles(juce::File::findFiles, false, "*.scpreset"); users.sort();
        if (users.size() > 0) { m.addSeparator(); m.addSectionHeader("Yours"); for (auto& u : users) m.addItem(1000 + (int) (&u - users.begin()), u.getFileNameWithoutExtension(), true, p.presetName() == u.getFileNameWithoutExtension()); }
        return m;
    }
    void choose(StretchCatProcessor& p, int id)
    {
        if (id >= 1000) { if (id - 1000 < users.size()) loadFile(p, users[id - 1000]); return; }
        const int i = id - 1; if (i < 0 || i >= (int) factory().size()) return;
        resetAll(p); for (auto& kv : factory()[(size_t) i].v) p.setReal(kv.first, kv.second);
        p.applyPreset(juce::ValueTree("PRESET")); p.setPresetName(factory()[(size_t) i].name);
    }
    void step(StretchCatProcessor& p, int dir_)
    {
        juce::StringArray names; for (auto& f : factory()) names.add(f.name);
        users = dir().findChildFiles(juce::File::findFiles, false, "*.scpreset"); users.sort(); for (auto& u : users) names.add(u.getFileNameWithoutExtension());
        int cur = names.indexOf(p.presetName()); cur = (cur + dir_ + names.size()) % names.size();
        if (cur < (int) factory().size()) choose(p, cur + 1); else choose(p, 1000 + cur - (int) factory().size());
    }
    // Save: asks before overwriting (overwrite / save as new / cancel).
    void saveDialog(StretchCatProcessor& p, std::function<void()> done)
    {
        auto* w = new juce::AlertWindow("Save preset", "Name your preset.", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor("name", p.presetName() == "Init" ? juce::String("My preset") : p.presetName());
        w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey)); w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create([this, &p, w, done](int r)
        {
            const auto n = sanitise(w->getTextEditorContents("name")); delete w; if (r != 1 || n.isEmpty()) return;
            auto f = dir().getChildFile(n + ".scpreset");
            if (!f.existsAsFile()) { write(p, f); p.setPresetName(n); done(); return; }
            auto* q = new juce::AlertWindow("Preset exists", "A preset called \"" + n + "\" already exists.", juce::MessageBoxIconType::QuestionIcon);
            q->addButton("Overwrite", 1); q->addButton("Save as new", 2); q->addButton("Cancel", 0);
            q->enterModalState(true, juce::ModalCallbackFunction::create([this, &p, f, n, done, q](int r2)
            {
                delete q; if (r2 == 1) { write(p, f); p.setPresetName(n); done(); }
                else if (r2 == 2) { auto g = f.getNonexistentSibling(); write(p, g); p.setPresetName(g.getFileNameWithoutExtension()); done(); }
            }), false);
        }), false);
    }
    void renameDialog(StretchCatProcessor& p, std::function<void()> done)
    {
        auto f = dir().getChildFile(p.presetName() + ".scpreset"); if (!f.existsAsFile()) return;     // factory sounds are renamed by saving them
        auto* w = new juce::AlertWindow("Rename preset", "New name:", juce::MessageBoxIconType::NoIcon); w->addTextEditor("name", p.presetName());
        w->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey)); w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create([&p, w, f, done](int r)
        {
            const auto n = sanitise(w->getTextEditorContents("name")); delete w; if (r != 1 || n.isEmpty()) return;
            auto g = f.getSiblingFile(n + ".scpreset"); if (g.existsAsFile()) return; if (f.moveFileTo(g)) { p.setPresetName(n); done(); }
        }), false);
    }
    void deleteCurrent(StretchCatProcessor& p) { auto f = dir().getChildFile(p.presetName() + ".scpreset"); if (f.existsAsFile() && f.moveToTrash()) p.setPresetName("Init"); }
    void exportDialog(StretchCatProcessor& p, std::unique_ptr<juce::FileChooser>& ch)
    {
        ch = std::make_unique<juce::FileChooser>("Export preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(p.presetName() + ".scpreset"), "*.scpreset");
        ch->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this, &p](const juce::FileChooser& c) { if (c.getResult() != juce::File()) write(p, c.getResult().withFileExtension("scpreset")); });
    }
    void importDialog(StretchCatProcessor& p, std::unique_ptr<juce::FileChooser>& ch, std::function<void()> done)
    {
        ch = std::make_unique<juce::FileChooser>("Import preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.scpreset");
        ch->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, &p, done](const juce::FileChooser& c)
        { auto f = c.getResult(); if (f.existsAsFile()) { auto g = dir().getChildFile(f.getFileName()).getNonexistentSibling(); f.copyFileTo(g); loadFile(p, g); done(); } });
    }

private:
    juce::Array<juce::File> users;
    static juce::String sanitise(const juce::String& s) { return juce::File::createLegalFileName(s.trim()).substring(0, 48); }
    static void write(StretchCatProcessor& p, const juce::File& f) { if (auto xml = p.capturePreset().createXml()) xml->writeTo(f); }
    void loadFile(StretchCatProcessor& p, const juce::File& f)
    {
        if (auto xml = juce::XmlDocument::parse(f)) if (xml->hasTagName("PRESET")) { p.applyPreset(juce::ValueTree::fromXml(*xml)); p.setPresetName(f.getFileNameWithoutExtension()); }
    }
    static void resetAll(StretchCatProcessor& p)
    {
        for (auto* prm : p.getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm)) { if (r->paramID == "sela" || r->paramID == "selb") continue; r->setValueNotifyingHost(r->getDefaultValue()); }
    }
};
}
