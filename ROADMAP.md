# Roadmap: Stretch Cat

Sampler and track effect.

## Done
- [x] Stretch engine in pure C++ with unit tests (speed, pitch, modes, lanes, tape link, transient slices)
- [x] Sampler voices (loop, one shot, gate, note repitch, voice stealing), tested
- [x] JUCE plugin shell with host parameters, CI (core tests, configure, build)

## Next
- [ ] Move the render to a background thread
- [ ] Lanes, slices and the filter / EQ / compressor rack in the plugin
- [ ] Real formant shift (the mockup says plugin only)
- [ ] Track effect version (stretch the DAW audio)
- [x] cat-ui editor v1: wood cheeks, waveforms, controls, hover help, drawn through `catui::RenderGate`
- [ ] editor: slice markers and pads, over-time lanes, FX rack (as in the mockup)
- [ ] Listen on real material and tune the modes

## Every cat plugin
- Draw through `catui::RenderGate` so meters and animations stop when the window is not in front (audio, automation and MIDI keep running)
- Settings page with MIDI filters on by default, selectable preset/backup folders, UI size reset to 100%
