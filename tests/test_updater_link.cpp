// Unit test for the Studio 87 Gear Updater link (no JUCE needed): the product id and the exact link the About button opens.
#include "../src/stretch/Studio87Link.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

static int fails = 0;
#define CHECK(cond, msg) do { if (cond) std::printf ("ok:   %s\n", msg); else { ++fails; std::printf ("FAIL: %s\n", msg); } } while (0)

int main()
{
    const std::string product = "stretch-cat-vst";
    bool charsOk = ! product.empty() && product.size() <= 100;
    for (char ch : product) charsOk = charsOk && (std::isalnum ((unsigned char) ch) || ch == '.' || ch == '-' || ch == '_');
    CHECK (charsOk, "product id (the catalog repository name) uses only letters, digits, dot, dash and underscore");
    CHECK (studio87link::updaterUrl (product) == "studio87://open?product=stretch-cat-vst", "link is studio87://open?product=<repo name>");
    CHECK (studio87link::updaterUrl ("") == "studio87://open", "no product gives the plain open link");
    CHECK (studio87link::updaterUrl (product).find ("://") == 8, "scheme is studio87");
    CHECK (studio87link::updaterUrl (product).find ("http") == std::string::npos, "link is not a web address (no network call)");
    CHECK (std::string (studio87link::fallbackMessage()) == "Install Studio 87 Gear Updater from github.com/SpeakingColors/gearupdater/releases", "fallback message is the contract text");
    CHECK (std::string (studio87link::tipText()).find ("Nothing is sent anywhere.") != std::string::npos, "hover bubble says nothing is sent");
    CHECK (studio87link::githubUrl (product) == "https://github.com/SpeakingColors/stretch-cat-vst", "GitHub button opens the repo home page");
    CHECK (studio87link::githubUrl (product).find ("/releases") == std::string::npos, "GitHub button is not the releases page");
    CHECK (std::string (studio87link::githubTipText()) == "Opens this plugin's home page on GitHub in your browser. Nothing is sent by the plugin.", "GitHub hover bubble text");
    CHECK (std::string (studio87link::githubButtonText()) == "Open on GitHub", "GitHub button text");
    CHECK (std::string (studio87link::fallbackNote()).find ("Gear Updater not found, opened the GitHub page instead.") == 0, "fallback note starts as agreed");
    CHECK (std::string (studio87link::fallbackNote()).find ("github.com/SpeakingColors/gearupdater/releases") != std::string::npos, "fallback note says where to get Gear Updater");
    const char* texts[] = { studio87link::tipText(), studio87link::githubTipText(), studio87link::fallbackNote(), studio87link::aboutLine(), studio87link::buttonText(), studio87link::githubButtonText() };
    bool noDash = true;
    for (const char* x : texts) noDash = noDash && std::strstr (x, "\xE2\x80\x94") == nullptr;
    CHECK (noDash, "no em dash in any text");
    return fails == 0 ? 0 : 1;
}
