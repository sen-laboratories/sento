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
 * description. The application quits when the window is closed (it is a BAlert: the About window of Haiku is not in the public API).
 *
 *     void App::AboutRequested() { sen::ShowAbout(); }
 */

#include <Alert.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <File.h>
#include <Invoker.h>
#include <Message.h>
#include <Roster.h>
#include <String.h>

#include <string.h>

namespace sen {

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

	BString title(hasVersion && version.short_info[0] != '\0' ? version.short_info : info.ref.name);
	BString text(title);
	if (hasVersion)
		text << "\nVersion " << (int) version.major << "." << (int) version.middle << "." << (int) version.minor;
	if (hasVersion && version.long_info[0] != '\0')
		text << "\n\n" << version.long_info;
	text << "\n\nCopyright " B_UTF8_COPYRIGHT " " << (int) firstYear << "-2026 " << holder
		<< "\nA plugin of SEN, the Semantic Extensions Native for Haiku (MIT license).";

	BAlert* alert = new BAlert("about", text.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	// (asynchronous: the application quits when the window is closed)
	alert->Go(new BInvoker(new BMessage(B_QUIT_REQUESTED), be_app));
}

}	// namespace sen
