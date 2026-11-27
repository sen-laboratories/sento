/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

#include <string.h>

#include <Entry.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>

/**
 * @file SenTrash.h
 * @brief Whether a file is in the Trash (header only, Haiku only).
 *
 * A file that is moved to the Trash keeps its relations: they stay with it, and come back with it when it is restored, just like the attributes of any file.
 * They are removed when the Trash is emptied (Akita tells the server; see sen::cmd::kRelationsRemoveReferences). Meanwhile a file in the Trash is not part of
 * what SEN shows: lookups of an object by its `SEN:ID` and queries over the volumes skip it, so menus, graph and enrichers see the relations to it as if its target
 * were absent. The one rule for "in the Trash" is here, so that every program agrees.
 */
namespace sen {
namespace trash {

/**
 * @brief Whether a file is in the Trash of its volume (at any depth: a file in a folder that was thrown away is in the Trash, too).
 *
 * Every volume has a Trash folder of its own (find_directory with the volume); a file is in it when its path starts with the path of that folder.
 * A file that does not exist (any more), or a volume without a Trash folder, is not "in the Trash".
 */
inline bool
IsInTrash(const entry_ref& ref)
{
	char trash[B_PATH_NAME_LENGTH];
	if (find_directory(B_TRASH_DIRECTORY, ref.device, false, trash, sizeof(trash)) != B_OK)
		return false;

	BPath path(&ref);
	if (path.InitCheck() != B_OK || path.Path() == NULL)
		return false;

	const size_t length = strlen(trash);
	return length > 0 && strncmp(path.Path(), trash, length) == 0 && path.Path()[length] == '/';
}

}	// namespace trash
}	// namespace sen
