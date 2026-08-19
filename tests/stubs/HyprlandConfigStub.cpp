#include <hyprland/src/config/ConfigValue.hpp>

#include <vector>

// Hyprland's pkg-config does not link a libhyprland shared object, so
// CConfigValueBase's out-of-line constructor/destructor/registry are not
// available to test executables.  This stub provides the minimal symbols
// needed by ConfigParserTest.cpp without mocking any compositor globals.

#if HYPR_RADIANT_HYPRLAND_CONFIG_VALUE_BASE
CConfigValueBase::CConfigValueBase()  = default;
CConfigValueBase::~CConfigValueBase() = default;

std::vector<CConfigValueBase*>& CConfigValueBase::registry() {
    static std::vector<CConfigValueBase*> reg;
    return reg;
}

void CConfigValueBase::flushCaches() {
}
#endif

#if HYPR_RADIANT_HYPRLAND_LOGGER_NEEDS_ENV_STUB
namespace Env {
bool isTrace() {
    return false;
}
} // namespace Env
#endif
