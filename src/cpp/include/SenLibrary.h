/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenLibrary.h
 * @brief Where a new entity goes: the Library of the current context (Haiku only).
 *
 * A plugin that makes an entity (a person for the author of a book, a movie, a label) does not put it into the folder of the file that it found it for. Every
 * context has a Library with a folder for each type of entity (`People`, `Books`, `Movies`), and the SEN server tells where it is:
 *
 *     entry_ref folder;
 *     if (sen::library::FolderFor(sen::onto::people::mime::kPerson, &folder) != B_OK)
 *         ... // no server: do what was done before (next to the file)
 *
 * The server knows the current context and its Library (the default one is `~/Library`, the name of the folder can be set in the settings of SEN; that of another context `~/Library/Contexts/<name>`, a context can name another folder in the attribute
 * `SEN:libraryPath`), makes the folders that are missing and names the folder of a type after its plural (the slot `plural` of the root class Entity of the ontology, `SEN:PLURAL` of the type). The entity is found
 * again by a query for its key, wherever it is: the Library only decides where NEW ones are made. See sen::cmd::kContextLibrary.
 */

#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>

#include "SenMessages.h"
#include "SenMimeTypes.h"

namespace sen {
namespace library {

/**
 * The folder for entities of a type in the Library of a context.
 * @param type the entity MIME type, NULL for the Library itself
 * @param folder gets the folder
 * @param context the name of a context, NULL: the current one
 * @param create make the folders that are missing
 * @return B_OK, B_NAME_NOT_FOUND if the server is not running, B_ENTRY_NOT_FOUND for a context that does not exist, B_BAD_VALUE for a type that is not an entity type, or the error of the server
 */
inline status_t
FolderFor(const char* type, entry_ref* folder, const char* context = NULL, bool create = true, BString* contextUsed = NULL)
{
	BMessenger server(sen::kServerSignature);
	if (!server.IsValid())
		return B_NAME_NOT_FOUND;

	BMessage request(sen::cmd::kContextLibrary);
	if (type != NULL)
		request.AddString(sen::key::kType, type);
	if (context != NULL)
		request.AddString(sen::key::kContext, context);
	request.AddBool(sen::key::kCreate, create);

	BMessage reply;
	status_t result = server.SendMessage(&request, &reply, 5000000, 5000000);
	if (result != B_OK)
		return result;
	result = reply.GetInt32(sen::key::kResult, B_ERROR);
	if (result != B_OK)
		return result;
	if (contextUsed != NULL)
		contextUsed->SetTo(reply.GetString(sen::key::kContext, ""));
	return reply.FindRef("refs", folder);
}

/**
 * The path of the Library, or of the folder of a type in it, whether the folder is there or not (nothing is made unless @p create).
 * @return B_OK, or what FolderFor() returns
 */
inline status_t
PathFor(const char* type, BString* path, const char* context = NULL, bool create = false, BString* contextUsed = NULL)
{
	BMessenger server(sen::kServerSignature);
	if (!server.IsValid())
		return B_NAME_NOT_FOUND;

	BMessage request(sen::cmd::kContextLibrary);
	if (type != NULL)
		request.AddString(sen::key::kType, type);
	if (context != NULL)
		request.AddString(sen::key::kContext, context);
	request.AddBool(sen::key::kCreate, create);

	BMessage reply;
	status_t result = server.SendMessage(&request, &reply, 5000000, 5000000);
	if (result == B_OK)
		result = reply.GetInt32(sen::key::kResult, B_ERROR);
	if (result != B_OK)
		return result;
	path->SetTo(reply.GetString(sen::key::kLibraryPath, ""));
	if (contextUsed != NULL)
		contextUsed->SetTo(reply.GetString(sen::key::kContext, ""));
	return path->IsEmpty() ? B_ERROR : B_OK;
}

}	// namespace library
}	// namespace sen
