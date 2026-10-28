/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

#include <string>
#include <vector>

#include <Entry.h>
#include <FindDirectory.h>
#include <MimeType.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
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
 * stays with the server, which keeps both sides of a relation consistent. Also what a file is: its type, the type that a file of the MIME
 * database stands for, and the name of a type in words.
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

/**
 * @brief The type of a file: its semantic type (`META:TYPE`) if it has one, else its MIME type; empty if it has none.
 *
 * The semantic type says what a file is for SEN (a scientific paper, an entity of a book), the MIME type what its format is (a PDF).
 */
inline std::string
TypeOf(BNode& node)
{
	BString type;
	if (node.InitCheck() == B_OK && node.ReadAttrString(attr::kType, &type) == B_OK && !type.IsEmpty())
		return type.String();
	char mime[B_MIME_TYPE_LENGTH];
	if (node.InitCheck() == B_OK && BNodeInfo(&node).GetType(mime) == B_OK)
		return mime;
	return std::string();
}

/**
 * @brief The type that a file stands for when it is in the MIME database of the user, else empty.
 *
 * The file of a type (`~/config/settings/mime_db/entity/x-vnd.sen-labs.entity.book`) is no file of its own type: it has the attributes of the type, and
 * is known by where it is. Its type is its path in the database ("entity/x-vnd.sen-labs.entity.book").
 */
inline std::string
TypeOfDatabaseEntry(const entry_ref& ref)
{
	BPath settings, path(&ref);
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settings) != B_OK || settings.Append("mime_db") != B_OK || path.InitCheck() != B_OK)
		return std::string();

	const std::string database = std::string(settings.Path()) + "/";
	const std::string location = path.Path();
	if (location.compare(0, database.length(), database) != 0)
		return std::string();

	const std::string type = location.substr(database.length());
	return type.find('/') == std::string::npos ? std::string() : type;	// (a supertype is a folder, a type is in it)
}

/** @brief The name of a type for people (its short description: "Music album"); the type itself if it has none. */
inline std::string
NameOfType(const char* type)
{
	char shortName[B_MIME_TYPE_LENGTH];
	if (type != NULL && type[0] != '\0' && BMimeType(type).GetShortDescription(shortName) == B_OK && shortName[0] != '\0')
		return shortName;
	return type != NULL ? type : "";
}

}	// namespace lookup
}	// namespace sen
