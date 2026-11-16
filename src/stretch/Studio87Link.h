// Studio87Link.h: the link a plugin's About page opens to start Studio 87 Gear Updater.
// Contract: github.com/SpeakingColors/gearupdater, docs/plugin-link.md. The plugin makes no network call and sends
// nothing: it only asks the operating system to open studio87://open?product=<repo name>. Pure C++ (no JUCE) so the
// product id and the link can be unit tested.
#pragma once
#include <string>

namespace studio87link
{
// product = the GitHub repository name exactly as the Gear Updater catalog lists it (empty = no catalog row).
inline std::string updaterUrl (const std::string& product)
{
    return product.empty() ? std::string ("studio87://open") : "studio87://open?product=" + product;
}

inline const char* buttonText() { return "Open Studio 87 Gear Updater"; }

inline const char* fallbackMessage()
{
    return "Install Studio 87 Gear Updater from github.com/SpeakingColors/gearupdater/releases";
}

// For About boxes that cannot show a hover bubble (native message boxes): the same facts as one sentence.
inline const char* aboutLine()
{
    return "Open Studio 87 Gear Updater checks this plugin for updates. Nothing is sent anywhere. "
           "If nothing opens, install Studio 87 Gear Updater from github.com/SpeakingColors/gearupdater/releases.";
}

inline const char* tipText()
{
    return "Opens Studio 87 Gear Updater to check this plugin for updates. Nothing is sent anywhere. "
           "If nothing opens, install Studio 87 Gear Updater from github.com/SpeakingColors/gearupdater/releases.";
}
} // namespace studio87link

#if defined (JUCE_MODULE_AVAILABLE_juce_gui_basics)
#include <juce_gui_basics/juce_gui_basics.h>
namespace studio87link
{
// The About button: asks the OS to open the link. If nothing could open it, shows the fallback message.
// No network call is made by the plugin.
inline bool openUpdater (const char* product)
{
    const bool opened = juce::URL (juce::String (updaterUrl (product))).launchInDefaultBrowser();
    if (! opened)
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::InfoIcon)
                                          .withTitle ("Studio 87 Gear Updater")
                                          .withMessage (fallbackMessage())
                                          .withButton ("OK"),
                                      [] (int) {});
    return opened;
}
} // namespace studio87link
#endif
