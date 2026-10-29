/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenAbout.h
 * @brief The About window of a SEN plugin (header only, Haiku only).
 *
 * A plugin is an application that does its work from the command line and has no window. The SEN menu asks for its About window with the usual
 * B_ABOUT_REQUESTED message (it starts the plugin with it), and the plugin shows what its application resource says: the name, the version and the
 * description, with its logo, in the dialog with the stripe of SenStripeDialog.h. The application quits when the window is closed.
 *
 * The logo is a picture that the package of the plugin brings: `<data>/sen/plugins/<signature>/logo.jpg` (the data folders of the system and the user,
 * packaged or not); a plugin without one has a window without a logo.
 *
 *     void App::AboutRequested() { sen::ShowAbout(); }
 */

#include <AppFileInfo.h>
#include <Application.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <string.h>

#include "SenStripeDialog.h"
#include "SenVersion.h"

namespace sen {

/** The height of the logo of a plugin in the About window, in pixels. */
static const float kPluginLogoHeight = 128;


/** The logo that the package of the plugin brings, scaled to the height it is shown in, or NULL. */
inline BBitmap*
LoadPluginLogo(const char* signature, float height = kPluginLogoHeight)
{
	const directory_which folders[] = {B_USER_DATA_DIRECTORY, B_USER_NONPACKAGED_DATA_DIRECTORY, B_SYSTEM_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY};
	for (directory_which folder : folders) {
		BPath path;
		if (find_directory(folder, &path) != B_OK || path.Append("sen/plugins") != B_OK || path.Append(signature) != B_OK
				|| path.Append("logo.jpg") != B_OK)
			continue;
		BBitmap* logo = LoadDialogImage(path.Path(), height);
		if (logo != NULL)
			return logo;
	}
	return NULL;
}


/** Show the About window of this application: its name, version and description are those of its version resource. It quits the application when it is closed. */
inline void
ShowAbout(int32 firstYear = 2024, const char* holder = "SEN Labs e.U.")
{
	app_info info;
	if (be_app->GetAppInfo(&info) != B_OK)
		return;

	version_info version;
	memset(&version, 0, sizeof(version));
	BFile file(&info.ref, B_READ_ONLY);
	BAppFileInfo appInfo(&file);
	bool hasVersion = file.InitCheck() == B_OK && appInfo.GetVersionInfo(&version, B_APP_VERSION_KIND) == B_OK;

	BString name(hasVersion && version.short_info[0] != '\0' ? version.short_info : info.ref.name);
	BString number;
	if (hasVersion)
		number = VersionText(version);	// 0.1.0-beta.3: the stage and its count are in the version resource, as for the server
	BString copyright("Copyright " B_UTF8_COPYRIGHT " ");
	copyright << (int) firstYear << "-2026 " << holder << "\nA plugin of SEN, the Semantic Extensions Native for Haiku (MIT license).";

	AboutDialog* window = new AboutDialog(name.String(), number.String(), hasVersion ? version.long_info : "", copyright.String(),
		LoadPluginLogo(info.signature), true);
	window->ShowCentered();
}

}	// namespace sen
