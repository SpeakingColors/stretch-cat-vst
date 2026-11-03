# Roadmap: Stretch Cat

Sampler and track effect.

## Done in 0.2
- [x] Stretch engine in pure C++ with unit tests (speed, pitch, modes, lanes, tape link, transient slices)
- [x] Sampler voices (loop, one shot, gate, note repitch, slices to keys, voice stealing), tested
- [x] Render on a background thread. The audio thread only reads a finished buffer, swapped lock free (5 slots, tested under ThreadSanitizer) with a 20 ms crossfade. The previous render plays until the new one is ready, so knob changes that need a render are NOT instant (about 0.1 to 1 s for short samples)
- [x] Slices: even and by transient, draggable marks, 8 pads, MIDI notes from the root note play the slices
- [x] Over-time lanes: pitch, speed, formant (rendered in the background), cutoff and width (applied live per voice)
- [x] FX rack with 10 ms smoothed parameters: filter, EQ, compressor, formant
- [x] Real formant shift in the core: true-envelope estimate with a pitch-following lifter (fixes the smearing above about 400 Hz the first version had), see limits below
- [x] Editor built from the mockup layout (generated from the mockup DOM), wood cheeks, resizable, info bubbles, dB meter, presets
- [x] Track effect: pure C++ engine with tests (ring buffer capture, freeze, grain scheduler, loop, key, no clicks, no allocation) and mockup v2 page. See "Track effect" below for what is missing
- [x] Saved states carry a version (2). Version 1 states load (tested with a fixture in CI)

## Next
- [ ] Track effect plugin: an effect build (audio input, HOLD host parameter, window page from the mockup v2) that uses src/core/TrackFx.h. The engine and tests exist; the plugin target and the editor page do not
- [ ] Listen on real material and tune the modes, the formant shift and the track effect by ear (nothing in this repo has been judged by ear yet)
- [ ] Rack sections "Detune and widen" and "Output" from the mockup (the mockup rack scrolls; the plugin omits them so no scrollbar is needed), and a limiter
- [ ] Glide knob (the mockup has it; the plugin shows Gain in that place)
- [ ] Mockup preview rig and test signal source (not part of the plugin on purpose), Stretch FX tab in the window (the window shows it greyed out)
- [ ] Three-look theme dropdown from the mockup
- [ ] BPM and key analysis of the loaded sound

## Known limits and approximations
- Formant shift (0.3 engine): measured on synthetic vowels (harmonics under a known three-formant envelope, shifts of -5, +3 and +7 st, `tests/test_formant.cpp`). Worst error of the harmonic amplitudes against the envelope moved by the requested amount, after removing the overall gain: f0 100 to 220 Hz 1.1 to 1.4 dB rms, 330 and 440 Hz about 3.0 dB, 600 Hz 4.5 dB, 800 Hz 5.1 dB, 1 kHz 4.2 dB. The first version gave 10 to 30 dB at 600 Hz and above. The pitch never moved (more than 99.9 percent of the energy stays on the harmonics), white noise stays white (level +0.04 dB, bands within 0.7 dB), a click train stays in time, a gliding shift makes no clicks.
- Formant shift, what it cannot do: one pitch per frame (dense mixes and chords get one common lifter); a formant narrower than the gap between two harmonics is not in the signal, so the test widens the formants with the pitch (bandwidth scale f0 / 90 Hz); above about 600 Hz the error is several dB because only a few harmonics describe the envelope; shifts beyond plus or minus 12 st get artefacts. About 1.4 s of CPU per 10 s of stereo audio, in the background render. Not judged by ear.
- Formant shift has no latency and no audio-thread work: it runs inside the background render (an offline pass), so a formant change is heard when the new render swaps in, like speed and pitch.
- Width lane is a plain side gain from 0 to 200 percent, no Haas delay.
- Compressor uses a peak detector with half automatic make-up gain.
- Track effect controls whose meaning is not standardised are defined by us: Wave spacing (extra read position shift per grain), Depth (random pan and level spread), Rand (up to 20 ms of timing and position jitter), Hold amount (share of each transient region that is repeated). Overlapping grains are not phase aligned, so dense settings sound rough by design.
- Mutation testing of the new tests was only done for the Track effect core (a broken freeze, pitch, speed and crossfade were each caught), not for the rest.
- CI renders the real window offscreen on Linux; Windows and macOS windows, real DAWs and installers on real machines are not verified.

## Every cat plugin
- Draw through `catui::RenderGate` so meters and animations stop when the window is not in front (audio, automation and MIDI keep running)
- Settings page with MIDI filters on by default, selectable preset/backup folders, UI size reset to 100%
