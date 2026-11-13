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
 *
 * A program that is installed from a package also shows the revision of the package after it (1.0.0-beta.7-9): the package system puts the attribute `SYS:PACKAGE`
 * (`shoji-1.0.0~beta7-9`) on every file that a package brings, so nothing has to be written into the program for it (a program that was built by hand has none).
 */

#include <AppFileInfo.h>
#include <Entry.h>
#include <Node.h>
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


/** @brief The revision of the package that brought @p file (`9` for `shoji-1.0.0~beta7-9`); empty if the file is not from a package. */
inline BString
PackageRevision(const entry_ref& file)
{
	BNode node(&file);
	BString package;
	if (node.InitCheck() != B_OK || node.ReadAttrString("SYS:PACKAGE", &package) != B_OK)
		return BString();
	const int32 dash = package.FindLast('-');
	if (dash < 0 || dash + 1 >= package.Length())
		return BString();
	BString revision;
	package.CopyInto(revision, dash + 1, package.Length() - dash - 1);
	for (int32 i = 0; i < revision.Length(); i++) {
		if (revision[i] < '0' || revision[i] > '9')
			return BString();
	}
	return revision;
}


/** @brief The version as the user installed it: 1.0.0-beta.7-9, the version of the program and the revision of its package. */
inline BString
VersionText(const version_info& version, const entry_ref& file)
{
	BString text = VersionText(version);
	const BString revision = PackageRevision(file);
	if (!revision.IsEmpty())
		text << "-" << revision;
	return text;
}

}	// namespace sen
