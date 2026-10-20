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
 * description, with its logo. The look is that of the About window of Toji: a stripe, the logo scaled to the room it has, the text, OK. The
 * application quits when the window is closed.
 *
 * The logo is a picture that the package of the plugin brings: `<data>/sen/plugins/<signature>/logo.jpg` (the data folders of the system and the user,
 * packaged or not); a plugin without one has a window without a logo.
 *
 *     void App::AboutRequested() { sen::ShowAbout(); }
 */

#include <Application.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <Button.h>
#include <ControlLook.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <View.h>
#include <Window.h>

#include <math.h>
#include <string.h>

namespace sen {

/** the grey stripe on the left of the window, as in the About window of Toji */
class AboutStripeView : public BView {
public:
	AboutStripeView()
		:
		BView("stripe", B_WILL_DRAW)
	{
		float width = floorf(2 * be_control_look->DefaultLabelSpacing());
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
		SetExplicitMaxSize(BSize(width, B_SIZE_UNSET));
	}

	virtual void
	Draw(BRect)
	{
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		FillRect(Bounds());
	}
};


/** the logo, scaled to the room it has (a picture, shown as a card) */
class AboutLogoView : public BView {
public:
	AboutLogoView(BBitmap* logo, float height)
		:
		BView("logo", B_WILL_DRAW),
		fLogo(logo)
	{
		float width = height;
		if (fLogo != NULL && fLogo->Bounds().Height() > 0)
			width = floorf(height * (fLogo->Bounds().Width() + 1) / (fLogo->Bounds().Height() + 1));
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		SetExplicitMinSize(BSize(width, height));
		SetExplicitMaxSize(BSize(width, height));
		SetExplicitPreferredSize(BSize(width, height));
	}

	virtual ~AboutLogoView() { delete fLogo; }

	virtual void
	Draw(BRect)
	{
		if (fLogo == NULL)
			return;
		BRect bounds = Bounds();
		SetDrawingMode(B_OP_COPY);
		DrawBitmap(fLogo, fLogo->Bounds(), bounds, B_FILTER_BITMAP_BILINEAR);
		SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
		StrokeRect(bounds);
	}

private:
	BBitmap*	fLogo;
};


/** the window closes the application that was started to show it */
class PluginAboutWindow : public BWindow {
public:
	PluginAboutWindow(const char* title)
		:
		BWindow(BRect(0, 0, 100, 100), title, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
	{
	}

	virtual bool
	QuitRequested()
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
};


/** The logo that the package of the plugin brings, or NULL. */
inline BBitmap*
LoadPluginLogo(const char* signature)
{
	const directory_which folders[] = {B_USER_DATA_DIRECTORY, B_USER_NONPACKAGED_DATA_DIRECTORY, B_SYSTEM_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY};
	for (directory_which folder : folders) {
		BPath path;
		if (find_directory(folder, &path) != B_OK || path.Append("sen/plugins") != B_OK || path.Append(signature) != B_OK
				|| path.Append("logo.jpg") != B_OK)
			continue;
		BBitmap* logo = BTranslationUtils::GetBitmap(path.Path());
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

	BString title(hasVersion && version.short_info[0] != '\0' ? version.short_info : info.ref.name);
	BString text(title);
	text << "\n";
	int32 titleEnd = text.Length();
	text << "\n";
	if (hasVersion)
		text << "Version " << (int) version.major << "." << (int) version.middle << "." << (int) version.minor << "\n";
	if (hasVersion && version.long_info[0] != '\0')
		text << "\n" << version.long_info << "\n";
	text << "\nCopyright " B_UTF8_COPYRIGHT " " << (int) firstYear << "-2026 " << holder
		<< "\nA plugin of SEN, the Semantic Extensions Native for Haiku (MIT license).";

	float spacing = be_control_look->DefaultLabelSpacing();
	float textWidth = be_plain_font->StringWidth("M") * 42;

	BTextView* view = new BTextView(BRect(0, 0, textWidth, 100), "text", BRect(0, 0, textWidth, 100), B_FOLLOW_NONE, B_WILL_DRAW);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	view->SetWordWrap(true);
	view->MakeEditable(false);
	view->MakeSelectable(false);
	view->SetStylable(true);
	view->SetText(text.String());

	// the name: large and bold; the empty line after it is just a small gap
	BFont titleFont(be_bold_font);
	titleFont.SetSize(be_plain_font->Size() * 2);
	view->SetFontAndColor(0, titleEnd, &titleFont, B_FONT_ALL);
	BFont gapFont(be_plain_font);
	gapFont.SetSize(be_plain_font->Size() * 0.6);
	view->SetFontAndColor(titleEnd, titleEnd + 1, &gapFont, B_FONT_ALL);

	float textHeight = view->TextHeight(0, view->CountLines() - 1);
	view->SetExplicitMinSize(BSize(textWidth, textHeight));
	view->SetExplicitMaxSize(BSize(textWidth, textHeight));

	BBitmap* logo = LoadPluginLogo(info.signature);

	PluginAboutWindow* window = new PluginAboutWindow((BString("About ") << title).String());
	BButton* ok = new BButton("ok", "OK", new BMessage(B_QUIT_REQUESTED));
	ok->MakeDefault(true);

	BLayoutBuilder::Group<>(window, B_HORIZONTAL, 0)
		.Add(new AboutStripeView())
		.AddGroup(B_VERTICAL, spacing)
			.SetInsets(spacing * 2)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(new AboutLogoView(logo, logo != NULL ? 200 : 1))
				.AddGlue()
			.End()
			.Add(view)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(ok)
			.End()
		.End();

	window->CenterOnScreen();
	window->Show();
}

}	// namespace sen
