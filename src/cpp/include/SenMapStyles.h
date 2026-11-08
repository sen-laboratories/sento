/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

#include <string>
#include <vector>

namespace sen {

/**
 * @file SenMapStyles.h
 * @brief The styles of the map of a place (plain C++, no Haiku API; tested on every platform in tests/SenMapStylesTest.cpp).
 *
 * Tiles of OpenStreetMap data that public servers give out without a key (slippy map tiles of 256 pixels, `{z}/{x}/{y}.png`). The default style is chosen in the settings of the spatial
 * navigator (the setting `map_style` of `~/config/settings/sen/spatial-navigator.settings`, the choice list is made of the table below), and can be changed in the view of the map (Shoji),
 * which keeps its own choice; the tiles of each style are kept in a folder of their own, named by its id.
 *
 * Only servers that need no key and no account are listed (the tile usage policies are those of the servers: a User-Agent that names the program, no bulk downloads, tiles kept for days).
 * Not listed, though often named for pretty maps: CARTO (Voyager, Positron: the tiles say "API KEY REQUIRED" now), Stadia (Stamen Watercolor, Alidade) and Thunderforest need a key; vector
 * styles (OpenFreeMap, Protomaps) need a renderer for vector tiles, which Shoji does not have.
 */
namespace mapstyles {

/** the setting of the default style, and the file of the settings of the spatial navigator that has it (below the settings folder of the user) */
inline const char* SettingKey() { return "map_style"; }
inline const char* PluginSettingsFile() { return "sen/spatial-navigator.settings"; }

struct Style {
    const char* id;             ///< the value of the setting, and the name of the folder of the cache
    const char* label;          ///< what the menus show
    const char* url;            ///< the address of a tile, with {z}, {x} and {y}
    const char* attribution[2]; ///< what the map says about whose it is (the licences ask for it); the second line may be NULL
};

/** the style that is used if none is chosen, and the first one asked for a tile that the chosen one does not answer */
inline const char* DefaultId() { return "topo"; }
inline const char* FallbackId() { return "osm"; }

inline const std::vector<Style>& All()
{
    static const std::vector<Style> styles = {
        {"topo", "OpenTopoMap (relief)", "https://tile.opentopomap.org/{z}/{x}/{y}.png",
            {"\xC2\xA9 OpenStreetMap contributors, SRTM", "\xC2\xA9 OpenTopoMap (CC-BY-SA)"}},
        {"osm", "OpenStreetMap (standard)", "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
            {"\xC2\xA9 OpenStreetMap contributors", NULL}},
        {"hot", "Humanitarian (warm, clear)", "https://a.tile.openstreetmap.fr/hot/{z}/{x}/{y}.png",
            {"\xC2\xA9 OpenStreetMap contributors", "Humanitarian OSM Team, hosted by OSM France"}},
        {"cyclosm", "CyclOSM (cycling)", "https://a.tile-cyclosm.openstreetmap.fr/cyclosm/{z}/{x}/{y}.png",
            {"\xC2\xA9 OpenStreetMap contributors", "CyclOSM (CC-BY-SA), OSM France"}},
    };
    return styles;
}

/** the style with the id, or NULL */
inline const Style* Find(const std::string& id)
{
    for (const Style& style : All()) {
        if (id == style.id)
            return &style;
    }
    return NULL;
}

/** the style with the id; the default if there is none of it (a setting that was written by another version) */
inline const Style& Get(const std::string& id)
{
    const Style* style = Find(id);
    return style != NULL ? *style : *Find(DefaultId());
}

/** the address of a tile of a style */
inline std::string TileUrl(const Style& style, int zoom, int x, int y)
{
    std::string url = style.url;
    const struct { const char* name; int value; } parts[] = {{"{z}", zoom}, {"{x}", x}, {"{y}", y}};
    for (const auto& part : parts) {
        const size_t at = url.find(part.name);
        if (at != std::string::npos)
            url.replace(at, 3, std::to_string(part.value));
    }
    return url;
}

}   // namespace mapstyles

}   // namespace sen
