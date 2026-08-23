#pragma once

#include <hypr-radiant/OverviewTarget.hpp>

#include <cstdint>
#include <optional>

namespace hypr_radiant {

enum class KeyboardActionType {
    None,
    Close,
    Activate,
    Backspace,
    OpenSearch,
    TogglePreferences,
    ToggleMode,
    JumpWorkspace,
    Move,
    TextInput,
};

struct KeyboardAction {
    KeyboardActionType  type      = KeyboardActionType::None;
    char                text      = '\0';
    std::int64_t       workspaceId = 0;
    NavigationDirection direction = NavigationDirection::Left;
};

[[nodiscard]] KeyboardAction resolveKeyboardAction(
    std::uint32_t key, bool searching, bool controlHeld, std::optional<char> searchCharacter);

} // namespace hypr_radiant
