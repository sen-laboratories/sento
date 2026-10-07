/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

#include <stddef.h>

#include <algorithm>
#include <string>
#include <vector>

#include "SenId.h"

/**
 * @file SenIdList.h
 * @brief Lists of `SEN:ID`s in attributes: `SEN:TO` (targets of normal relations) and `SEN:META` (targets of meta relations).
 *
 * A list is stored comma separated. BFS indexes only the first 255 bytes of a string attribute, so a list holds at most
 * sen::id::kMaxPerAttribute ids and a longer one continues in the next attribute: `SEN:TO`, `SEN:TO:1`, `SEN:TO:2`, ...
 * (at most kMaxChunks attributes, all indexed). The functions here are free of Haiku APIs and work on std types, so
 * that they can be tested everywhere; reading and writing the attributes is done by the SEN server.
 */
namespace sen {
namespace idlist {

/** How many attributes one list may use: `SEN:TO` and `SEN:TO:1` to `SEN:TO:7`. The ontology creates an index for each. */
inline constexpr size_t kMaxChunks = 8;
/** How many ids a list can hold at most. */
inline constexpr size_t kMaxIds = kMaxChunks * id::kMaxPerAttribute;

/**
 * @brief The name of the attribute that holds a chunk of the list.
 * @param base  the attribute of the first chunk, e.g. `SEN:TO`
 * @param index 0 for the first chunk (the base itself), n for `<base>:<n>`
 */
inline std::string
ChunkName(const std::string& base, size_t index)
{
    return index == 0 ? base : base + ":" + std::to_string(index);
}

/** @return the ids of a comma separated list; blanks are removed, empty entries are skipped. */
inline std::vector<std::string>
Split(const std::string& list)
{
    std::vector<std::string> ids;
    size_t start = 0;
    while (start <= list.size()) {
        size_t end = list.find(',', start);
        if (end == std::string::npos)
            end = list.size();

        size_t first = list.find_first_not_of(" \t\r\n", start);
        size_t last = list.find_last_not_of(" \t\r\n", end == 0 ? 0 : end - 1);
        if (first != std::string::npos && first < end && last != std::string::npos && last >= first)
            ids.push_back(list.substr(first, last - first + 1));

        start = end + 1;
    }
    return ids;
}

/** @return the ids joined by commas. */
inline std::string
Join(const std::vector<std::string>& ids, size_t begin = 0, size_t end = static_cast<size_t>(-1))
{
    std::string list;
    end = std::min(end, ids.size());
    for (size_t i = begin; i < end; i++) {
        if (!list.empty())
            list += ',';
        list += ids[i];
    }
    return list;
}

/**
 * @brief Distribute ids over the attributes of a list.
 * @param ids   all ids of the list
 * @param chunks receives the value of each attribute (the first for the base name, ...); empty if there are no ids
 * @return false if the list is too long (more than kMaxIds); chunks is then not changed
 */
inline bool
Chunk(const std::vector<std::string>& ids, std::vector<std::string>* chunks)
{
    if (ids.size() > kMaxIds)
        return false;

    chunks->clear();
    for (size_t begin = 0; begin < ids.size(); begin += id::kMaxPerAttribute)
        chunks->push_back(Join(ids, begin, begin + id::kMaxPerAttribute));
    return true;
}

/** @return true if the id is in the list. */
inline bool
Contains(const std::vector<std::string>& ids, const std::string& id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

/** @return true if the id was added, false if it was in the list already. */
inline bool
Add(std::vector<std::string>* ids, const std::string& id)
{
    if (Contains(*ids, id))
        return false;
    ids->push_back(id);
    return true;
}

/** @return true if the id was removed, false if it was not in the list. */
inline bool
Remove(std::vector<std::string>* ids, const std::string& id)
{
    auto position = std::find(ids->begin(), ids->end(), id);
    if (position == ids->end())
        return false;
    ids->erase(position);
    return true;
}

/**
 * @brief The BFS predicate that finds files whose list contains an id, over all attributes of the list.
 *
 * A wildcard on both sides: `SEN:TO == '*<id>*' || SEN:TO:1 == '*<id>*' || ...`. The ids have a fixed length and no id
 * is the part of another, so a match is a match.
 */
inline std::string
ContainsPredicate(const std::string& base, const std::string& id)
{
    std::string predicate;
    for (size_t i = 0; i < kMaxChunks; i++) {
        if (i > 0)
            predicate += " || ";
        predicate += ChunkName(base, i) + " == '*" + id + "*'";
    }
    return predicate;
}

}   // namespace idlist
}   // namespace sen
