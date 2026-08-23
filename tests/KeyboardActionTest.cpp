#include <hypr-radiant/input/KeyboardAction.hpp>

#include <linux/input-event-codes.h>

#include <cassert>
#include <iostream>
#include <optional>

using namespace hypr_radiant;

namespace {

void slashOpensSearchBeforeItTypes() {
    assert(resolveKeyboardAction(KEY_SLASH, false, false, '/').type == KeyboardActionType::OpenSearch);

    const auto slash = resolveKeyboardAction(KEY_SLASH, true, false, '/');
    assert(slash.type == KeyboardActionType::TextInput);
    assert(slash.text == '/');
}

void digitsJumpOrBecomeSearchText() {
    const auto nine = resolveKeyboardAction(KEY_9, false, false, '9');
    assert(nine.type == KeyboardActionType::JumpWorkspace);
    assert(nine.workspaceId == 9);
    assert(resolveKeyboardAction(KEY_0, false, false, '0').type == KeyboardActionType::None);

    const auto one = resolveKeyboardAction(KEY_1, true, false, '1');
    assert(one.type == KeyboardActionType::TextInput);
    assert(one.text == '1');

    const auto zero = resolveKeyboardAction(KEY_0, true, false, '0');
    assert(zero.type == KeyboardActionType::TextInput);
    assert(zero.text == '0');
}

void controlShortcutsNeverTypeFallbackCharacters() {
    assert(resolveKeyboardAction(KEY_COMMA, false, true, ',').type == KeyboardActionType::TogglePreferences);
    assert(resolveKeyboardAction(KEY_A, false, true, 'a').type == KeyboardActionType::None);
}

void tabOnlyChangesModeOutsideSearch() {
    assert(resolveKeyboardAction(KEY_TAB, false, false, std::nullopt).type == KeyboardActionType::ToggleMode);
    assert(resolveKeyboardAction(KEY_TAB, true, false, std::nullopt).type == KeyboardActionType::None);
}

void navigationAndCoreControlsRemainMapped() {
    const auto left = resolveKeyboardAction(KEY_LEFT, true, false, std::nullopt);
    assert(left.type == KeyboardActionType::Move);
    assert(left.direction == NavigationDirection::Left);

    assert(resolveKeyboardAction(KEY_RIGHT, false, true, std::nullopt).direction == NavigationDirection::Right);
    assert(resolveKeyboardAction(KEY_UP, false, false, std::nullopt).direction == NavigationDirection::Up);
    assert(resolveKeyboardAction(KEY_DOWN, false, false, std::nullopt).direction == NavigationDirection::Down);
    assert(resolveKeyboardAction(KEY_ESC, false, false, std::nullopt).type == KeyboardActionType::Close);
    assert(resolveKeyboardAction(KEY_ENTER, false, false, std::nullopt).type == KeyboardActionType::Activate);
    assert(resolveKeyboardAction(KEY_KPENTER, false, false, std::nullopt).type == KeyboardActionType::Activate);
    assert(resolveKeyboardAction(KEY_BACKSPACE, true, false, std::nullopt).type == KeyboardActionType::Backspace);
}

void printableKeysFeedSearch() {
    const auto letter = resolveKeyboardAction(KEY_A, false, false, 'a');
    assert(letter.type == KeyboardActionType::TextInput);
    assert(letter.text == 'a');
    assert(resolveKeyboardAction(KEY_A, false, false, std::nullopt).type == KeyboardActionType::None);
}

} // namespace

int main() {
    slashOpensSearchBeforeItTypes();
    digitsJumpOrBecomeSearchText();
    controlShortcutsNeverTypeFallbackCharacters();
    tabOnlyChangesModeOutsideSearch();
    navigationAndCoreControlsRemainMapped();
    printableKeysFeedSearch();
    std::cout << "KeyboardActionTest passed\n";
    return 0;
}
