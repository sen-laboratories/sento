/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenPluginSettings.h
 * @brief The settings that all plugins share (Haiku only).
 *
 * Some choices are not a plugin's own: where the entities that a plugin finds in a file go (a person for each author of a book, for the director of a movie) is decided
 * once, in the settings of SEN (the node *Plugins* of the settings window), not for each plugin and type. The SEN server keeps these settings and answers:
 *
 *     if (sen::plugins::ExtractEntitiesIntoFiles())
 *         ... // make the entity files and relate them
 *     else
 *         ... // write the names into the attributes of the file only
 *
 * Without a server the answer is the default (files).
 */

#include <Message.h>
#include <Messenger.h>

#include <string.h>

#include "SenMessages.h"
#include "SenSettingsForm.h"
#include "SenMimeTypes.h"

namespace sen {
namespace plugins {

/** Whether entities that a plugin finds are made as files of their own (the default) and not only written as names into the attributes of the file. */
inline bool
ExtractEntitiesIntoFiles()
{
	BMessenger server(sen::kServerSignature);
	if (!server.IsValid())
		return true;

	BMessage request(sen::cmd::kCoreSettings);
	request.AddString(sen::key::kScope, sen::key::kScopePlugins);
	BMessage reply;
	if (server.SendMessage(&request, &reply, 3000000, 3000000) != B_OK)
		return true;

	BMessage settings;
	if (reply.FindMessage(sen::settings::form::kFieldSettings, &settings) != B_OK)
		return true;
	return strcmp(settings.GetString(sen::key::kExtractInto, sen::key::kExtractIntoFiles), sen::key::kExtractIntoAttributes) != 0;
}

}	// namespace plugins
}	// namespace sen
