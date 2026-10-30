/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenKey.h
 * @brief The SEN key: the modifier key that switches a click or a drag to SEN (header only, Haiku only).
 *
 * The key is a setting of SEN (`SEN:modifierKey` in the settings of the server, chosen in its Settings window): by default the standard shortcut key of Haiku (B_COMMAND_KEY, which key of the
 * keyboard that is depends on the key map), or Option, Control or Shift. The programs that look at the key (the knot in the Deskbar, Akita) ask here instead of testing a key of their own.
 *
 *     if (sen::SenKeyIsDown()) ...      // in a click or a drag
 */

#include <Autolock.h>
#include <File.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Locker.h>
#include <Message.h>
#include <OS.h>
#include <Path.h>

#include "SenMessages.h"

namespace sen {

/** The modifier of the SEN key: B_COMMAND_KEY (the standard), B_OPTION_KEY, B_CONTROL_KEY or B_SHIFT_KEY. The file of the settings is read once a second, not at every mouse move. */
inline uint32
SenModifierKey()
{
	static BLocker lock("SEN key");
	static bigtime_t checked = 0;
	static uint32 key = B_COMMAND_KEY;

	BAutolock locker(lock);
	const bigtime_t now = system_time();
	if (checked != 0 && now - checked < 1000000)
		return key;
	checked = now;

	key = B_COMMAND_KEY;
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK && path.Append("sen") == B_OK && path.Append(config::kSettingsFile) == B_OK) {
		BFile file(path.Path(), B_READ_ONLY);
		BMessage settings;
		int32 stored;
		if (file.InitCheck() == B_OK && settings.Unflatten(&file) == B_OK && settings.FindInt32(key::kModifierKey, &stored) == B_OK
				&& (stored == B_COMMAND_KEY || stored == B_OPTION_KEY || stored == B_CONTROL_KEY || stored == B_SHIFT_KEY))
			key = (uint32) stored;
	}
	return key;
}

/** Whether the SEN key is pressed now. */
inline bool
SenKeyIsDown()
{
	return (modifiers() & SenModifierKey()) != 0;
}

}	// namespace sen
