#include <hypr-radiant/config/OmarchyPalette.hpp>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace {

using hypr_radiant::installedOmarchyThemes;
using hypr_radiant::isLightPalette;
using hypr_radiant::liftedSurface;
using hypr_radiant::loadOmarchyPalette;
using hypr_radiant::OmarchyPalette;
using hypr_radiant::omarchyPalettePath;
using hypr_radiant::parseOmarchyPalette;

constexpr std::string_view OSAKA_JADE = R"(accent = "#509475"
cursor = "#D7C995"
foreground = "#C1C497"
background = "#111c18"
selection_background = "#C1C497"

color0 = "#23372B"
color7 = "#F6F5DD"
)";

bool near(float value, float expected) {
    return std::fabs(value - expected) < 0.005F;
}

void parsesThemeColors() {
    const auto palette = parseOmarchyPalette(OSAKA_JADE);

    assert(palette.loaded);
    assert(near(palette.background.red, 0.067F));
    assert(near(palette.background.green, 0.110F));
    assert(near(palette.background.blue, 0.094F));
    assert(near(palette.foreground.red, 0.757F));
    assert(near(palette.accent.red, 0.314F));
    assert(near(palette.accent.green, 0.580F));
    assert(near(palette.accent.blue, 0.459F));
    assert(near(palette.accent.alpha, 1.0F));
}

void keepsGrayDefaultsWhenKeysAreMissingOrMalformed() {
    const OmarchyPalette fallback;
    const auto parsed = parseOmarchyPalette("color1 = \"#FF5345\"\nbackground = \"not-a-color\"\n# comment\n");

    // No recognised key resolved, so the neutral gray defaults survive untouched.
    assert(!parsed.loaded);
    assert(near(parsed.background.red, fallback.background.red));
    assert(near(parsed.foreground.red, fallback.foreground.red));
    assert(near(parsed.accent.red, fallback.accent.red));
}

void toleratesEmptyAndWhitespaceInput() {
    assert(!parseOmarchyPalette("").loaded);
    assert(!parseOmarchyPalette("\n\n   \n").loaded);

    // Padding around the key and value must not defeat the match.
    const auto padded = parseOmarchyPalette("   accent   =   \"#FF0000\"   \n");
    assert(padded.loaded);
    assert(near(padded.accent.red, 1.0F));
    assert(near(padded.accent.green, 0.0F));
}

void detectsLightAndDarkThemes() {
    const auto dark = parseOmarchyPalette("background = \"#111c18\"\n");
    assert(!isLightPalette(dark));

    const auto light = parseOmarchyPalette("background = \"#FFFFFF\"\n");
    assert(isLightPalette(light));
}

void liftsAwayFromTheBackgroundInBothDirections() {
    const auto dark = parseOmarchyPalette("background = \"#111c18\"\n");
    const auto darkLift = liftedSurface(dark, dark.background, 0.20F);
    // Dark theme: surfaces step lighter than the background.
    assert(darkLift.red > dark.background.red);
    assert(darkLift.green > dark.background.green);

    const auto light = parseOmarchyPalette("background = \"#FFFFFF\"\n");
    const auto lightLift = liftedSurface(light, light.background, 0.20F);
    // Light theme: the same call must step darker instead, or surfaces would vanish.
    assert(lightLift.red < light.background.red);
    assert(lightLift.green < light.background.green);
}

void preservesAlphaAndClampsLift() {
    const auto palette = parseOmarchyPalette("background = \"#111c18\"\n");
    const auto lifted = liftedSurface(palette, palette.background, 5.0F);

    assert(near(lifted.alpha, palette.background.alpha));
    // A lift beyond 1.0 clamps to the target end rather than overshooting.
    assert(lifted.red <= 1.0F && lifted.red >= 0.999F);
}

void pointsAtQuattroStateDirectory() {
    const auto* previousHome = std::getenv("HOME");
    const auto  savedHome    = previousHome ? std::string{previousHome} : std::string{};

    setenv("HOME", "/tmp/hypr-radiant-palette-test", 1);
    assert(omarchyPalettePath() == "/tmp/hypr-radiant-palette-test/.local/state/omarchy/current/theme/colors.toml");

    if (previousHome)
        setenv("HOME", savedHome.c_str(), 1);
    else
        unsetenv("HOME");
}

void discoversAndLoadsInstalledThemes() {
    const auto* previousHome        = std::getenv("HOME");
    const auto* previousOmarchyPath = std::getenv("OMARCHY_PATH");
    const auto  savedHome           = previousHome ? std::string{previousHome} : std::string{};
    const auto  savedOmarchyPath    = previousOmarchyPath ? std::string{previousOmarchyPath} : std::string{};
    const auto  root                = std::filesystem::path{"/tmp/hypr-radiant-native-theme-test"};
    const auto  home                = root / "home";
    const auto  stock               = root / "omarchy" / "themes";
    const auto  user                = home / ".config" / "omarchy" / "themes";

    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(stock / "tokyo-night");
    std::filesystem::create_directories(user / "tokyo-night");
    std::filesystem::create_directories(user / "custom-terminal");
    {
        std::ofstream output{stock / "tokyo-night" / "colors.toml"};
        output << "background = \"#111111\"\nforeground = \"#eeeeee\"\naccent = \"#3366ff\"\n";
    }
    {
        std::ofstream output{user / "tokyo-night" / "colors.toml"};
        output << "background = \"#222222\"\nforeground = \"#dddddd\"\naccent = \"#ff6600\"\n";
    }
    {
        std::ofstream output{user / "custom-terminal" / "alacritty.toml"};
        output << "[colors.primary]\nbackground = '#101820'\nforeground = '#f0f4f8'\n"
                  "[colors.normal]\nblue = '#00aaff'\n";
    }

    setenv("HOME", home.c_str(), 1);
    setenv("OMARCHY_PATH", (root / "omarchy").c_str(), 1);

    const auto themes = installedOmarchyThemes();
    assert(themes.size() == 2);
    assert(themes[0].slug == "custom-terminal");
    assert(themes[0].name == "Custom Terminal");
    assert(themes[1].slug == "tokyo-night");
    assert(!themes[1].userPath.empty());
    assert(!themes[1].stockPath.empty());

    const auto overlaid = loadOmarchyPalette("tokyo-night");
    assert(overlaid.loaded);
    assert(near(overlaid.background.red, 0.133F));
    assert(near(overlaid.accent.red, 1.0F));

    const auto alacritty = loadOmarchyPalette("custom-terminal");
    assert(alacritty.loaded);
    assert(near(alacritty.background.red, 0.063F));
    assert(near(alacritty.foreground.red, 0.941F));
    assert(near(alacritty.accent.green, 0.667F));

    if (previousHome)
        setenv("HOME", savedHome.c_str(), 1);
    else
        unsetenv("HOME");
    if (previousOmarchyPath)
        setenv("OMARCHY_PATH", savedOmarchyPath.c_str(), 1);
    else
        unsetenv("OMARCHY_PATH");
    std::filesystem::remove_all(root, error);
}

} // namespace

int main() {
    parsesThemeColors();
    keepsGrayDefaultsWhenKeysAreMissingOrMalformed();
    toleratesEmptyAndWhitespaceInput();
    detectsLightAndDarkThemes();
    liftsAwayFromTheBackgroundInBothDirections();
    preservesAlphaAndClampsLift();
    pointsAtQuattroStateDirectory();
    discoversAndLoadsInstalledThemes();
    return 0;
}
