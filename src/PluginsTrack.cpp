// Plugin entry point of Stretch Cat Track Effect.
#include "trackfx/TrackFxProcessor.h"
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TrackFxProcessor(); }
