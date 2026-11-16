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

// The plugin's home page on GitHub: https://github.com/SpeakingColors/<repo name> (not the releases page).
inline std::string githubUrl (const std::string& product)
{
    return "https://github.com/SpeakingColors/" + product;
}

inline const char* githubButtonText() { return "Open on GitHub"; }

inline const char* githubTipText()
{
    return "Opens this plugin's home page on GitHub in your browser. Nothing is sent by the plugin.";
}

// One line shown when the studio87:// link could not be opened and the GitHub page was opened instead.
inline const char* fallbackNote()
{
    return "Gear Updater not found, opened the GitHub page instead. Get Studio 87 Gear Updater from github.com/SpeakingColors/gearupdater/releases";
}

inline const char* fallbackMessage()
{
    return "Install Studio 87 Gear Updater from github.com/SpeakingColors/gearupdater/releases";
}

// For About boxes that cannot show a hover bubble (native message boxes): the same facts as text.
inline const char* aboutLine()
{
    return "Open Studio 87 Gear Updater checks this plugin for updates. Nothing is sent anywhere. "
           "If it is not installed, the GitHub page opens instead; get Gear Updater from github.com/SpeakingColors/gearupdater/releases. "
           "Open on GitHub opens this plugin's home page in your browser. Nothing is sent by the plugin.";
}

inline const char* tipText()
{
    return "Opens Studio 87 Gear Updater to check this plugin for updates. Nothing is sent anywhere. "
           "If it is not installed, the GitHub page opens instead.";
}
} // namespace studio87link

#if defined (JUCE_MODULE_AVAILABLE_juce_gui_basics)
#include <juce_gui_basics/juce_gui_basics.h>
namespace studio87link
{
// Opens the plugin's home page on GitHub in the browser. No other network call is made by the plugin.
inline bool openGitHub (const char* product)
{
    return juce::URL (juce::String (githubUrl (product))).launchInDefaultBrowser();
}

// The updater button: asks the OS to open studio87://open?product=NAME. If nothing could open it, the GitHub page is
// opened instead and a one-line note says so (as an alert here; About pages with a label show fallbackNote() themselves).
inline bool openUpdater (const char* product, bool showNote = true)
{
    const bool opened = juce::URL (juce::String (updaterUrl (product))).launchInDefaultBrowser();
    if (! opened)
    {
        openGitHub (product);
        if (showNote)
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::InfoIcon)
                                              .withTitle ("Studio 87 Gear Updater")
                                              .withMessage (fallbackNote())
                                              .withButton ("OK"),
                                          [] (int) {});
    }
    return opened;
}
} // namespace studio87link
#endif
