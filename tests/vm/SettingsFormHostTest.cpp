/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/**
 * The settings form of Shoji as a replicant, seen from a host that does not link it (SenSettingsFormHost.h): the class is loaded from the executable of Shoji,
 * the form is given its data with messages, and what the user does in it comes back as messages. Needs a Shoji (with ShojiMessageView) whose MIME entry is
 * known (`mimeset -f bin/shoji` after the build), and a desktop:
 *     g++ -std=c++20 -Wall -Wextra -Wno-multichar -o host-test SettingsFormHostTest.cpp -lbe && ./host-test [signature]
 * The class comes from the application that BRoster::FindApp() finds for the signature, which is the installed Shoji unless the argument names another: to test a
 * build, copy it to a place on a volume with the BEOS:APP_SIG index (the home folder), give the copy another signature (`addattr -t string BEOS:APP_SIG ...`) and
 * pass that signature.
 */

#include <Application.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <OutlineListView.h>
#include <TextControl.h>
#include <Window.h>

#include <stdio.h>
#include <string.h>

#include "../../src/cpp/include/SenMessagePath.h"
#include "../../src/cpp/include/SenSettingsFormHost.h"

namespace form = sen::settings::form;
using namespace sen::settings;

static int sFailures = 0;
static int sChecks = 0;

#define CHECK(condition) \
	do { \
		sChecks++; \
		if (!(condition)) { \
			sFailures++; \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
		} \
	} while (0)


static BMessage
MakeSettings()
{
	BMessage deep, nested, settings;
	deep.AddString("name", "one");
	nested.AddInt32("depth", 2);
	nested.AddString("label", "deep");
	nested.AddMessage("items", &deep);
	settings.AddString("email", "a@b.c");
	settings.AddBool("unpaywall", true);
	settings.AddInt32("max_authors", 10);
	settings.AddString("tags", "alpha");
	settings.AddString("tags", "beta");
	settings.AddMessage("nested", &nested);
	return settings;
}


/** the host: its window gets what the form tells */
class HostWindow : public BWindow {
public:
	HostWindow()
		:
		BWindow(BRect(100, 100, 600, 450), "settings form host", B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
		changed(0),
		whole(0),
		refused(0),
		refusedStatus(B_OK)
	{
		SetLayout(new BGroupLayout(B_VERTICAL));
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case form::kChanged:
				changed++;
				if (message->HasString(form::kFieldPath)) {
					lastPath = message->GetString(form::kFieldPath, "");
					lastValue = message->GetString(form::kFieldValue, "");
				} else {
					whole++;
				}
				message->FindMessage(form::kFieldSettings, &lastSettings);
				break;
			case form::kRefused:
				refused++;
				lastRefusedPath = message->GetString(form::kFieldPath, "");
				refusedStatus = message->GetInt32(form::kFieldStatus, B_OK);
				break;
			default:
				BWindow::MessageReceived(message);
		}
	}

	void Settle()
	{
		Unlock();
		snooze(250000);
		Lock();
	}

	void Reset()
	{
		changed = whole = refused = 0;
		lastPath.clear();
		lastValue.clear();
		lastRefusedPath.clear();
		refusedStatus = B_OK;
	}

	int32		changed;
	int32		whole;
	int32		refused;
	status_t	refusedStatus;
	std::string	lastPath;
	std::string	lastValue;
	std::string	lastRefusedPath;
	BMessage	lastSettings;
};


static std::string
Text(const BMessage& message, const char* path)
{
	std::string text = "<none>";
	GetText(message, path, &text);
	return text;
}


/** the user types into a text field and presses Enter */
static void
Type(HostWindow* window, BView* view, const char* name, const char* text)
{
	BTextControl* control = dynamic_cast<BTextControl*>(view->FindView(name));
	CHECK(control != NULL);
	if (control == NULL)
		return;
	control->SetText(text);
	control->Invoke();
	window->Settle();
}


int main(int argc, char** argv)
{
	BApplication app("application/x-vnd.sen-labs.settings-form-host-test");

	const char* signature = argc > 1 ? argv[1] : form::kShojiSignature;
	CHECK(form::IsAvailable(signature));
	CHECK(!form::IsAvailable("application/x-vnd.sen-labs.no-such-app"));

	// a class that cannot be loaded: no view, and the reason
	status_t error = B_OK;
	BView* none = form::Instantiate("x", &error, "application/x-vnd.sen-labs.no-such-app");
	CHECK(none == NULL);
	CHECK(error == B_ENTRY_NOT_FOUND);

	// the class is loaded from the executable of Shoji (this program does not link it)
	BView* loose = form::Instantiate("loose", &error, signature);
	CHECK(loose != NULL);
	// a form that is in no window cannot be talked to
	BMessage settings = MakeSettings();
	CHECK(form::SetData(loose, settings, NULL, NULL, BMessenger()) == B_NOT_INITIALIZED);
	delete loose;

	HostWindow* window = new HostWindow();
	BView* view = form::Instantiate("settings form", &error, signature);
	CHECK(view != NULL);
	if (view == NULL) {
		printf("(%s: %s)\n", signature, strerror(error));
		printf("%d checks, %d failure(s)\n", sChecks, sFailures);
		return 1;
	}
	CHECK(strcmp(view->Name(), "settings form") == 0);
	window->AddChild(view);
	window->Show();
	window->Lock();

	// the data: what the form shows
	BMessage defaults;
	defaults.AddString("email", "");
	defaults.AddBool("unpaywall", false);
	defaults.AddInt32("max_authors", 20);
	CHECK(form::SetData(view, settings, NULL, &defaults, BMessenger(window), true, "Host test") == B_OK);
	window->Settle();
	BTextControl* email = dynamic_cast<BTextControl*>(view->FindView("email"));
	CHECK(email != NULL && strcmp(email->Text(), "a@b.c") == 0);
	BCheckBox* unpaywall = dynamic_cast<BCheckBox*>(view->FindView("unpaywall"));
	CHECK(unpaywall != NULL && unpaywall->Value() == B_CONTROL_ON);
	BOutlineListView* tree = dynamic_cast<BOutlineListView*>(view->FindView("sen:tree"));
	CHECK(tree != NULL && tree->FullListCountItems() == 3);		// the message itself, nested, and its one item
	CHECK(window->changed == 0);								// the data of the owner is no change

	// changes come back as messages with the path of the value
	window->Reset();
	Type(window, view, "email", "me@example.org");
	CHECK(window->changed == 1 && window->lastPath == "email" && window->lastValue == "me@example.org");
	CHECK(Text(window->lastSettings, "email") == "me@example.org");
	unpaywall->SetValue(B_CONTROL_OFF);
	unpaywall->Invoke();
	window->Settle();
	CHECK(window->lastPath == "unpaywall" && window->lastValue == "false");
	CHECK(Text(window->lastSettings, "unpaywall") == "false");

	// a value that does not fit: refused, the field goes back
	window->Reset();
	Type(window, view, "max_authors", "many");
	CHECK(window->refused == 1 && window->lastRefusedPath == "max_authors" && window->refusedStatus == B_BAD_VALUE);
	CHECK(window->changed == 0);
	CHECK(strcmp(dynamic_cast<BTextControl*>(view->FindView("max_authors"))->Text(), "10") == 0);

	// the data can be read back (not from the thread of the window: this is the main thread)
	BMessage back;
	window->Unlock();
	CHECK(form::GetData(view, &back) == B_OK);
	window->Lock();
	CHECK(Text(back, "email") == "me@example.org");
	CHECK(Text(back, "unpaywall") == "false");
	CHECK(Text(back, "max_authors") == "10");
	CHECK(Text(back, "nested.depth") == "2");

	// revert: the data as it was set, told as a change of the whole message
	window->Reset();
	CHECK(form::Revert(view) == B_OK);
	window->Settle();
	CHECK(window->whole == 1 && window->changed == 1);
	CHECK(window->lastSettings.HasSameData(settings, true, true));
	email = dynamic_cast<BTextControl*>(view->FindView("email"));
	CHECK(email != NULL && strcmp(email->Text(), "a@b.c") == 0);

	// defaults
	window->Reset();
	CHECK(form::Defaults(view) == B_OK);
	window->Settle();
	CHECK(window->whole == 1);
	CHECK(window->lastSettings.HasSameData(defaults, true, true));
	CHECK(strcmp(dynamic_cast<BTextControl*>(view->FindView("max_authors"))->Text(), "20") == 0);

	// a protocol that is newer than the form: not guessed at, the host is told, nothing changes
	window->Reset();
	BMessage newer(form::kSetData);
	newer.AddInt32(form::kFieldProtocol, form::kProtocol + 1);
	newer.AddMessage(form::kFieldSettings, &settings);
	newer.AddMessenger(form::kFieldTarget, BMessenger(window));
	BMessenger(view).SendMessage(&newer);
	window->Settle();
	CHECK(window->refused == 1 && window->refusedStatus == B_NOT_SUPPORTED);
	CHECK(strcmp(dynamic_cast<BTextControl*>(view->FindView("max_authors"))->Text(), "20") == 0);

	// a message without settings
	window->Reset();
	BMessage empty(form::kSetData);
	empty.AddMessenger(form::kFieldTarget, BMessenger(window));
	BMessenger(view).SendMessage(&empty);
	window->Settle();
	CHECK(window->refused == 1 && window->refusedStatus == B_BAD_VALUE);

	// new data replaces the old, and a form that is not editable shows the values only
	CHECK(form::SetData(view, settings, NULL, NULL, BMessenger(window), false) == B_OK);
	window->Settle();
	email = dynamic_cast<BTextControl*>(view->FindView("email"));
	CHECK(email != NULL && strcmp(email->Text(), "a@b.c") == 0 && !email->IsEnabled());
	window->Reset();
	CHECK(form::Defaults(view) == B_OK);			// (there are no defaults now)
	window->Settle();
	CHECK(window->changed == 0);

	window->Quit();
	printf("%d checks, %d failure(s)\n", sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
