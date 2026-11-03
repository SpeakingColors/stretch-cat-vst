Stretch Cat 0.2.0, a test build. Free and open source (AGPL-3.0-or-later). Collects nothing, never connects to the network.

What is new
- The real plugin window, built from the approved design: waveforms with selection handles, slice marks and 8 slice pads, over-time lanes (pitch, speed, cutoff, formant, width), FX rack (filter, EQ, compressor, formant), dB meter, presets, hover help, wood cheeks, resizable.
- Rendering runs on a background thread. The previous sound keeps playing and fades into the new one, so knobs that need a new render are not instant (about 0.1 to 1 s for short samples). Cutoff, width, the rack, gain, attack and release are instant.
- Slices play from the pads or from MIDI notes (Playback: Slices to keys).
- A real formant shift (spectral envelope warp). Limits are written in the manual.
- Saved projects from 0.1 still load.
- The Track effect (listen, HOLD, repeat as grains) has its engine and tests and a mockup page, but is not in the plugin yet.

Not checked yet (please tell us on GitHub)
- The sound has not been judged by ear, and nothing was tried in a real DAW.
- The installers were built by CI but not run on real Windows, macOS or Linux machines. The Windows and macOS installers are unsigned, so you may see a warning (INSTALL.txt explains how to continue).

One zip: installers for Windows (.exe), macOS (.pkg), Linux (Flatpak extension and a plain .vst3), the PDF manual, INSTALL.txt and checksums.
