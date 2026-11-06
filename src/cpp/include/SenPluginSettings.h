/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenPluginSettings.h
 * @brief The settings that all plugins share (Haiku only).
 *
 * Some choices are not a plugin's own, and are decided once, in the settings of SEN (the node *Plugins* of the settings window), not for each plugin and type:
 *
 *  - where the entities that a plugin finds in a file go (a person for each author of a book, for the director of a movie): files of their own, or only names in the attributes,
 *  - whether the user is asked which one is meant if a lookup finds several answers (several books for a title),
 *  - the most entities of one type that a plugin makes of a file (the first 10 authors).
 *
 * The SEN server keeps these settings and answers:
 *
 *     const sen::plugins::SharedSettings shared = sen::plugins::LoadSharedSettings();
 *     if (shared.extractIntoFiles)
 *         ... // make the entity files (at most shared.maxEntities of a type) and relate them
 *     else
 *         ... // write the names into the attributes of the file only
 *
 * Without a server the answer is the defaults. A command line option of a plugin (`--auto`) wins over the setting for one run.
 */

#include <Message.h>
#include <Messenger.h>

#include <string.h>

#include "SenMessages.h"
#include "SenSettingsForm.h"
#include "SenMimeTypes.h"

namespace sen {
namespace plugins {

/** What all plugins share. */
struct SharedSettings {
	/** entities that a plugin finds are made as files of their own (and not only written as names into the attributes of the file) */
	bool	extractIntoFiles = true;
	/** ask which one is meant if there are several answers (off: the best one is taken) */
	bool	askIfSeveral = true;
	/** the most entities of one type that are made of one file: 0 for none, sen::key::kMaxEntitiesInternal (255) if the setting is All */
	int32	maxEntities = sen::key::kMaxEntitiesDefault;
};

/** Asks the server for the settings that all plugins share; the defaults if there is no server or it does not answer. */
inline SharedSettings
LoadSharedSettings()
{
	SharedSettings result;
	BMessenger server(sen::kServerSignature);
	if (!server.IsValid())
		return result;

	BMessage request(sen::cmd::kCoreSettings);
	request.AddString(sen::key::kScope, sen::key::kScopePlugins);
	BMessage reply;
	if (server.SendMessage(&request, &reply, 3000000, 3000000) != B_OK)
		return result;

	BMessage settings;
	if (reply.FindMessage(sen::settings::form::kFieldSettings, &settings) != B_OK)
		return result;
	result.extractIntoFiles = strcmp(settings.GetString(sen::key::kExtractInto, sen::key::kExtractIntoFiles), sen::key::kExtractIntoAttributes) != 0;
	result.askIfSeveral = settings.GetBool(sen::key::kAskIfSeveral, true);
	const int32 most = settings.GetInt32(sen::key::kMaxEntities, sen::key::kMaxEntitiesDefault);
	if (most >= sen::key::kMaxEntitiesLimit)
		result.maxEntities = sen::key::kMaxEntitiesInternal;		// (All)
	else if (most >= 0)
		result.maxEntities = most;
	return result;
}

/** Whether entities that a plugin finds are made as files of their own (the default) and not only written as names into the attributes of the file. */
inline bool
ExtractEntitiesIntoFiles()
{
	return LoadSharedSettings().extractIntoFiles;
}

}	// namespace plugins
}	// namespace sen
