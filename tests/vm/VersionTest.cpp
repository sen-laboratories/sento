/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/**
 * Prints the version of programs as the About windows show it (SenVersion.h): the number of the version resource, and the same with the revision of the package that
 * brought the program (the attribute SYS:PACKAGE that the package file system puts on it; none for a program that was built by hand).
 *
 *     version-test /boot/system/apps/Shoji ./bin/shoji
 *       /boot/system/apps/Shoji: 1.0.0-beta.7 | 1.0.0-beta.7-9
 *       ./bin/shoji: 1.0.0-beta.7 | 1.0.0-beta.7
 *
 * Haiku only.
 *     g++ -std=c++17 -Wno-multichar -I<headers> -o version-test VersionTest.cpp -lbe
 */
#include <AppFileInfo.h>
#include <Entry.h>
#include <File.h>
#include <stdio.h>
#include <string.h>

#include <sen/SenVersion.h>

int
main(int argc, char** argv)
{
	int failures = 0;
	for (int i = 1; i < argc; i++) {
		entry_ref ref;
		version_info version;
		memset(&version, 0, sizeof(version));
		BFile file;
		if (get_ref_for_path(argv[i], &ref) != B_OK || file.SetTo(&ref, B_READ_ONLY) != B_OK || BAppFileInfo(&file).GetVersionInfo(&version, B_APP_VERSION_KIND) != B_OK) {
			printf("%s: no version resource\n", argv[i]);
			failures++;
			continue;
		}
		printf("%s: %s | %s\n", argv[i], sen::VersionText(version).String(), sen::VersionText(version, ref).String());
	}
	return failures;
}
