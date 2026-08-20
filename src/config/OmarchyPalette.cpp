#include <hypr-radiant/config/OmarchyPalette.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace hypr_radiant {
namespace {

constexpr std::string_view QUATTRO_PALETTE_RELATIVE_PATH = "/.local/state/omarchy/current/theme/colors.toml";
constexpr std::string_view LEGACY_PALETTE_RELATIVE_PATH  = "/.config/omarchy/current/theme/colors.toml";

std::string_view trim(std::string_view value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r'))
        value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r'))
        value.remove_suffix(1);
    return value;
}

std::string_view unquote(std::string_view value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value.remove_prefix(1);
        value.remove_suffix(1);
    }
    return value;
}

std::string themeDisplayName(std::string_view slug) {
    std::string name;
    name.reserve(slug.size());
    auto capitalize = true;
    for (const auto character : slug) {
        if (character == '-') {
            name.push_back(' ');
            capitalize = true;
            continue;
        }
        const auto byte = static_cast<unsigned char>(character);
        name.push_back(capitalize ? static_cast<char>(std::toupper(byte)) : character);
        capitalize = false;
    }
    return name;
}

std::filesystem::path userThemesPath() {
    const auto* home = std::getenv("HOME");
    if (home == nullptr || *home == '\0')
        return {};
    return std::filesystem::path{home} / ".config" / "omarchy" / "themes";
}

std::filesystem::path stockThemesPath() {
    if (const auto* omarchyPath = std::getenv("OMARCHY_PATH"); omarchyPath && *omarchyPath)
        return std::filesystem::path{omarchyPath} / "themes";
    return "/usr/share/omarchy/themes";
}

void collectThemes(const std::filesystem::path& root, bool user, std::map<std::string, OmarchyTheme>& themes) {
    if (root.empty())
        return;

    std::error_code error;
    std::filesystem::directory_iterator iterator{root, error};
    if (error)
        return;
    for (const auto& entry : iterator) {
        if (!entry.is_directory(error)) {
            error.clear();
            continue;
        }
        const auto slug = entry.path().filename().string();
        if (slug.empty() || slug.front() == '.')
            continue;
        auto& theme = themes[slug];
        theme.slug = slug;
        theme.name  = themeDisplayName(slug);
        if (user)
            theme.userPath = entry.path();
        else
            theme.stockPath = entry.path();
    }
}

OmarchyPalette readPaletteFile(const std::filesystem::path& path) {
    std::ifstream file{path};
    if (!file)
        return {};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return parseOmarchyPalette(buffer.str());
}

std::string_view alacrittyColorValue(std::string_view value) {
    value = trim(value);
    if (value.empty())
        return {};
    if (value.front() == '"' || value.front() == '\'') {
        const auto quote = value.front();
        const auto end   = value.find(quote, 1);
        if (end == std::string_view::npos)
            return {};
        return value.substr(1, end - 1);
    }
    const auto comment = value.find_first_of(" \t");
    return value.substr(0, comment);
}

OmarchyPalette readAlacrittyPalette(const std::filesystem::path& path) {
    std::ifstream file{path};
    if (!file)
        return {};

    OmarchyPalette palette;
    std::string     section;
    std::string     line;
    while (std::getline(file, line)) {
        auto view = trim(line);
        if (view.empty() || view.front() == '#')
            continue;
        if (view.front() == '[' && view.back() == ']') {
            section = std::string{trim(view.substr(1, view.size() - 2))};
            continue;
        }
        const auto separator = view.find('=');
        if (separator == std::string_view::npos)
            continue;
        const auto key    = trim(view.substr(0, separator));
        const auto parsed = parseAccentColor(alacrittyColorValue(view.substr(separator + 1)));
        if (!parsed)
            continue;
        if (section == "colors.primary" && key == "background") {
            palette.background = *parsed;
            palette.loaded     = true;
        } else if (section == "colors.primary" && key == "foreground") {
            palette.foreground = *parsed;
            palette.loaded     = true;
        } else if (section == "colors.normal" && key == "blue") {
            palette.accent = *parsed;
            palette.loaded = true;
        }
    }
    return palette;
}

} // namespace

OmarchyPalette parseOmarchyPalette(std::string_view contents) {
    OmarchyPalette palette;

    while (!contents.empty()) {
        const auto lineEnd = contents.find('\n');
        const auto line    = trim(contents.substr(0, lineEnd));
        contents           = lineEnd == std::string_view::npos ? std::string_view{} : contents.substr(lineEnd + 1);

        if (line.empty() || line.front() == '#')
            continue;

        const auto separator = line.find('=');
        if (separator == std::string_view::npos)
            continue;

        const auto key    = trim(line.substr(0, separator));
        const auto parsed = parseAccentColor(unquote(trim(line.substr(separator + 1))));
        if (!parsed)
            continue;

        if (key == "background") {
            palette.background = *parsed;
            palette.loaded     = true;
        } else if (key == "foreground") {
            palette.foreground = *parsed;
            palette.loaded     = true;
        } else if (key == "accent") {
            palette.accent = *parsed;
            palette.loaded = true;
        }
    }

    return palette;
}

std::string omarchyPalettePath() {
    const auto* home = std::getenv("HOME");
    if (home == nullptr || *home == '\0')
        return {};

    return std::string{home} + std::string{QUATTRO_PALETTE_RELATIVE_PATH};
}

std::vector<OmarchyTheme> installedOmarchyThemes() {
    std::map<std::string, OmarchyTheme> themes;
    collectThemes(stockThemesPath(), false, themes);
    collectThemes(userThemesPath(), true, themes);

    std::vector<OmarchyTheme> result;
    result.reserve(themes.size());
    for (auto& entry : themes)
        result.push_back(std::move(entry.second));
    std::ranges::sort(result, {}, &OmarchyTheme::name);
    return result;
}

OmarchyPalette loadOmarchyPalette(std::string_view themeSlug) {
    const auto* home = std::getenv("HOME");
    if (home == nullptr || *home == '\0')
        return {};

    if (!themeSlug.empty()) {
        const auto themes = installedOmarchyThemes();
        const auto theme = std::ranges::find(themes, themeSlug, &OmarchyTheme::slug);
        if (theme != themes.end()) {
            const std::array themePaths{theme->userPath, theme->stockPath};
            for (const auto& path : themePaths) {
                if (path.empty())
                    continue;
                const auto palette = readPaletteFile(path / "colors.toml");
                if (palette.loaded)
                    return palette;
            }

            for (const auto& path : themePaths) {
                if (path.empty())
                    continue;
                const auto palette = readAlacrittyPalette(path / "alacritty.toml");
                if (palette.loaded)
                    return palette;
            }
        }
    }

    const std::array paths{
        std::string{home} + std::string{QUATTRO_PALETTE_RELATIVE_PATH},
        std::string{home} + std::string{LEGACY_PALETTE_RELATIVE_PATH},
    };
    for (const auto& path : paths) {
        const auto palette = readPaletteFile(path);
        if (palette.loaded)
            return palette;
    }

    return {};
}

bool isLightPalette(const OmarchyPalette& palette) {
    return relativeLuminance(palette.background) > 0.5F;
}

RadiantRgba liftedSurface(const OmarchyPalette& palette, const RadiantRgba& base, float amount) {
    // Light themes need surfaces to step darker; dark themes need them lighter. Targeting the
    // opposite end of the range keeps every call site direction-agnostic.
    const auto target = isLightPalette(palette) ? 0.0F : 1.0F;
    const auto mix    = std::clamp(amount, 0.0F, 1.0F);
    const auto blend  = [target, mix](float channel) {
        return channel + (target - channel) * mix;
    };

    return RadiantRgba{
        .red   = blend(base.red),
        .green = blend(base.green),
        .blue  = blend(base.blue),
        .alpha = base.alpha,
    };
}

std::array<RadiantRgba, 3> themePreviewColors(const OmarchyPalette& palette) noexcept {
    return {palette.background, palette.foreground, palette.accent};
}

} // namespace hypr_radiant
