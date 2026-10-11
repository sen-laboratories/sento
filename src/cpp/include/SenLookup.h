/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

#include <string>
#include <vector>

#include <Entry.h>
#include <Node.h>
#include <Query.h>
#include <String.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "SenAttributes.h"
#include "SenIdList.h"

/**
 * @file SenLookup.h
 * @brief Read the targets of the relations of a file without the SEN server (header only, Haiku only).
 *
 * The relations are in the attributes of the files, so programs that only need to know where a relation leads (installers,
 * scripts, tools that run before the server does) can follow it themselves: the targets of a file are the ids in its `SEN:TO`
 * attributes (see SenIdList.h), and a file is found by its id with a query on `SEN:ID`. Adding, changing and removing relations
 * stays with the server, which keeps both sides of a relation consistent.
 */
namespace sen {
namespace lookup {

/**
 * @brief The `SEN:ID`s of the targets of the normal relations of a node.
 * @param base the list to read: attr::kTo (relations) or attr::kMeta (classification and context)
 * @return B_OK, also for a node without targets (the list is empty)
 */
inline status_t
TargetIds(BNode& node, std::vector<std::string>* ids, const char* base = attr::kTo)
{
	ids->clear();
	status_t result = node.InitCheck();
	if (result != B_OK)
		return result;

	for (size_t chunk = 0; chunk < idlist::kMaxChunks; chunk++) {
		BString value;
		if (node.ReadAttrString(idlist::ChunkName(base, chunk).c_str(), &value) != B_OK)
			continue;
		for (const std::string& id : idlist::Split(value.String()))
			ids->push_back(id);
	}
	return B_OK;
}

/** @brief The file that has a `SEN:ID`, searched on all volumes that can be queried. */
inline status_t
EntryForId(const std::string& id, entry_ref* ref)
{
	BString predicate;
	predicate << attr::kId << "==\"" << id.c_str() << "\"";

	BVolumeRoster roster;
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (!volume.KnowsQuery() || !volume.KnowsAttr())
			continue;
		BQuery query;
		if (query.SetVolume(&volume) != B_OK || query.SetPredicate(predicate.String()) != B_OK || query.Fetch() != B_OK)
			continue;
		if (query.GetNextRef(ref) == B_OK)
			return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}

/**
 * @brief The files that the normal relations of a file lead to (targets that are gone are left out).
 */
inline status_t
Targets(const entry_ref& source, std::vector<entry_ref>* refs)
{
	refs->clear();
	BNode node(&source);
	std::vector<std::string> ids;
	status_t result = TargetIds(node, &ids);
	if (result != B_OK)
		return result;

	for (const std::string& id : ids) {
		entry_ref ref;
		if (EntryForId(id, &ref) == B_OK)
			refs->push_back(ref);
	}
	return B_OK;
}

}	// namespace lookup
}	// namespace sen
