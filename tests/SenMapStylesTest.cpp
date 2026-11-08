/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenMapStyles.h: the table of the map styles is complete, no style needs a key, and the address of a tile is made from the template. Plain C++, no Haiku API. */

#include "TestHarness.h"
#include "../src/cpp/include/SenMapStyles.h"

#include <set>
#include <string.h>

using namespace sen::mapstyles;

TEST(EveryStyleIsComplete)
{
	// a unique id, a label, an https address with the place of the zoom and of the two coordinates, no place for a key, and an attribution that names OpenStreetMap
	std::set<std::string> ids;
	for (const Style& style : All()) {
		CHECK(ids.insert(style.id).second);
		CHECK(strlen(style.label) > 0);
		const std::string url = style.url;
		CHECK(url.compare(0, 8, "https://") == 0);
		CHECK(url.find("{z}") != std::string::npos && url.find("{x}") != std::string::npos && url.find("{y}") != std::string::npos);
		CHECK(url.find("{key}") == std::string::npos && url.find("apikey") == std::string::npos && url.find("api_key") == std::string::npos);
		CHECK(style.attribution[0] != NULL && std::string(style.attribution[0]).find("OpenStreetMap") != std::string::npos);
	}
	CHECK(ids.size() >= 4);
}

TEST(DefaultAndFallback)
{
	CHECK(Find(DefaultId()) != NULL && Find(FallbackId()) != NULL);
	CHECK(Find("voyager") == NULL);			// (it needs a key now, and is not offered)
	CHECK(&Get("nowhere") == Find(DefaultId()));
	CHECK(&Get("hot") == Find("hot"));
	CHECK_STR(SettingKey(), "map_style");
}

TEST(TileAddress)
{
	CHECK_STR(TileUrl(*Find("osm"), 15, 17874, 11363), "https://tile.openstreetmap.org/15/17874/11363.png");
	CHECK_STR(TileUrl(*Find("hot"), 3, 4, 2), "https://a.tile.openstreetmap.fr/hot/3/4/2.png");
	CHECK_STR(TileUrl(*Find("cyclosm"), 17, 0, 131071), "https://a.tile-cyclosm.openstreetmap.fr/cyclosm/17/0/131071.png");
}

TEST_MAIN()
