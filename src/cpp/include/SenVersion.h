/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenVersion.h
 * @brief The version of a program as the releases of SEN are named (header only, Haiku only).
 *
 * The version is kept in the version resource of the program (`app_version` of its rdef) and nowhere else: the numbers, the stage of the release (`variety`)
 * and, for each stage before the final one, its count (`internal`). The About windows, the Info window and `sen --version` show it with VersionText(), and the
 * scripts that make the packages read the same resource, so that everything says the same:
 *
 *     rdef: major 1, middle 0, minor 0, variety B_APPV_BETA, internal 4    ->  1.0.0-beta.4   (package 1.0.0~beta4)
 *     rdef: major 2, middle 0, minor 0, variety B_APPV_DEVELOPMENT, internal 1  ->  2.0.0-develop.1  (package 2.0.0~develop.1)
 *     rdef: major 1, middle 0, minor 0, variety B_APPV_FINAL                   ->  1.0.0
 */

#include <AppFileInfo.h>
#include <String.h>

namespace sen {

/** @brief The name of a stage of the release (the values of B_APPV_DEVELOPMENT... of the resource compiler, they are not in a header); empty for the final version. */
inline const char*
VersionStageName(uint32 variety)
{
	switch (variety) {
		case 0:	return "develop";
		case 1:	return "alpha";
		case 2:	return "beta";
		case 3:	return "gamma";
		case 4:	return "gm";	// golden master
		default:	return "";	// final
	}
}


/** @brief The full version: 1.0.0-beta.4. The stage follows a dash and is counted by `internal` if that is not 0; the final version has neither. */
inline BString
VersionText(const version_info& version)
{
	BString text;
	text << (int) version.major << "." << (int) version.middle << "." << (int) version.minor;
	const char* stage = VersionStageName(version.variety);
	if (stage[0] != '\0') {
		text << "-" << stage;
		if (version.internal > 0)
			text << "." << (int) version.internal;
	}
	return text;
}

}	// namespace sen
