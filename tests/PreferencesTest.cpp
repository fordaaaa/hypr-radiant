#include <hypr-radiant/config/Preferences.hpp>

#include <cassert>
#include <iostream>

using namespace hypr_radiant;

namespace {

void defaultsFollowExistingConfig() {
    const auto preferences = parsePreferences("");
    assert(preferences.workspaceView == WorkspaceViewPreference::FollowConfig);
    assert(preferences.windowView == WindowViewPreference::Spatial);
    assert(preferences.accent == AccentPreference::FollowConfig);
    assert(preferences.motion == MotionPreference::FollowConfig);
}

void parsesEveryPreference() {
    const auto preferences = parsePreferences(R"(
workspace_view = workspace_wall
window_view=grouped
accent = violet
motion = reduced
)");
    assert(preferences.workspaceView == WorkspaceViewPreference::WorkspaceWall);
    assert(preferences.windowView == WindowViewPreference::Grouped);
    assert(preferences.accent == AccentPreference::Violet);
    assert(preferences.motion == MotionPreference::Reduced);
}

void parsesQuattroPreferences() {
    const auto preferences = parsePreferences(R"(
workspace_view = carousel
window_view = deck
motion = quattro
)");
    assert(preferences.workspaceView == WorkspaceViewPreference::Carousel);
    assert(preferences.windowView == WindowViewPreference::Deck);
    assert(preferences.motion == MotionPreference::Quattro);
    assert(label(preferences.workspaceView) == "CAROUSEL");
    assert(label(preferences.windowView) == "DECK");
    assert(label(preferences.motion) == "QUATTRO");
}

void ignoresUnknownKeysAndFallsBackOnUnknownValues() {
    const auto preferences = parsePreferences(R"(
unknown = preserved-nowhere
workspace_view = surprise
window_view = surprise
accent = surprise
motion = surprise
)");
    assert(preferences == PreferencesState{});
}

void serializationRoundTrips() {
    const PreferencesState expected{
        .workspaceView = WorkspaceViewPreference::Carousel,
        .windowView = WindowViewPreference::Deck,
        .accent = AccentPreference::Blue,
        .motion = MotionPreference::Quattro,
    };
    assert(parsePreferences(serializePreferences(expected)) == expected);
}

void accentNavigationFollowsArrowDirectionAndWraps() {
    assert(stepAccentPreference(AccentPreference::FollowConfig, 1) == AccentPreference::Green);
    assert(stepAccentPreference(AccentPreference::Green, 1) == AccentPreference::Blue);
    assert(stepAccentPreference(AccentPreference::Blue, -1) == AccentPreference::Green);
    assert(stepAccentPreference(AccentPreference::FollowConfig, -1) == AccentPreference::Violet);
    assert(stepAccentPreference(AccentPreference::Violet, 1) == AccentPreference::FollowConfig);
}

} // namespace

int main() {
    defaultsFollowExistingConfig();
    parsesEveryPreference();
    parsesQuattroPreferences();
    ignoresUnknownKeysAndFallsBackOnUnknownValues();
    serializationRoundTrips();
    accentNavigationFollowsArrowDirectionAndWraps();
    std::cout << "PreferencesTest passed\n";
    return 0;
}
