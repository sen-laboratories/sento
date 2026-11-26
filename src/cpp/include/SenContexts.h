/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenContexts.h
 * @brief The contexts that are set up and the current one (Haiku only).
 *
 * A context is a folder in the contexts folder of the settings of SEN (`~/config/settings/sen/contexts/<name>`); the current one is a setting of the SEN server
 * (`SEN:currentContext` in `sen.settings`), the default one when none was chosen. The Deskbar menu shows the same list. The Library of a context, where the entities
 * of that context are, is asked of the server: see SenLibrary.h.
 */

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <strings.h>

#include <algorithm>
#include <string>
#include <vector>

#include "SenMessages.h"

namespace sen {
namespace contexts {

/**
 * The names of the contexts, the default one first, the others in alphabetical order, and the name of the current one (the default one if none was chosen, or if the one that was
 * chosen is gone).
 * @return B_OK; B_ERROR if the settings folder cannot be found (the default context alone is still returned)
 */
inline status_t
List(std::vector<std::string>* names, std::string* current)
{
	names->clear();
	*current = sen::config::kContextDefault;

	BPath settings;
	status_t result = find_directory(B_USER_SETTINGS_DIRECTORY, &settings);
	if (result == B_OK)
		result = settings.Append("sen");
	if (result == B_OK) {
		BPath folder(settings);
		if (folder.Append(sen::config::kContextsDir) == B_OK) {
			BDirectory directory(folder.Path());
			entry_ref ref;
			while (directory.GetNextRef(&ref) == B_OK) {
				BEntry entry(&ref, true);
				if (entry.IsDirectory())
					names->push_back(ref.name);
			}
		}
		BPath file(settings);
		if (file.Append(sen::config::kSettingsFile) == B_OK) {
			BFile settingsFile(file.Path(), B_READ_ONLY);
			BMessage message;
			if (settingsFile.InitCheck() == B_OK && message.Unflatten(&settingsFile) == B_OK)
				*current = message.GetString(sen::key::kCurrentContext, sen::config::kContextDefault);
		}
	}

	std::sort(names->begin(), names->end(), [](const std::string& a, const std::string& b) { return strcasecmp(a.c_str(), b.c_str()) < 0; });
	auto defaultContext = std::find(names->begin(), names->end(), std::string(sen::config::kContextDefault));
	if (defaultContext != names->end())
		names->erase(defaultContext);
	names->insert(names->begin(), sen::config::kContextDefault);

	if (std::find(names->begin(), names->end(), *current) == names->end())
		*current = sen::config::kContextDefault;
	return result;
}

}	// namespace contexts
}	// namespace sen
