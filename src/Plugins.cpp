// Plugin entry point.
#include "stretch/StretchCat.h"
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StretchCatProcessor(); }
