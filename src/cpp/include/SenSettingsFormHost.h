/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenSettingsFormHost.h
 * @brief For a program that shows the settings form of Shoji in its own window (Haiku only, tested in tests/vm/SettingsFormHostTest.cpp).
 *
 * The form is the view `ShojiMessageView` of Shoji. The program does not link it and does not need Shoji at build time: Instantiate() makes the stub of an archive
 * (the class and the signature of the application that has it) and `instantiate_object()` loads the class from the executable of Shoji, found with
 * `BRoster::FindApp()`. From then on the program talks to the form with messages only (SenSettingsForm.h): the functions here only send them.
 *
 *     BView* form = sen::settings::form::Instantiate();       // NULL: no Shoji, or too old; say so in the pane and offer the settings file
 *     window->AddChild(form);                                 // the view has to be in a window before it can be talked to
 *     sen::settings::form::SetData(form, settings, &schema, &defaults, BMessenger(window));
 *     // ... kChanged and kRefused come to the window; the working copy is in the `settings` of kChanged
 *
 * The form is in the process of the host, in the thread of the window that has it: its messages are handled there, so nothing here waits for an answer, except
 * GetData(), which must not be called from that thread.
 */

#include <Archivable.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <View.h>

#include "SenSettingsForm.h"

namespace sen {
namespace settings {
namespace form {

/** Whether an application with the signature of Shoji is installed (it may still be too old to have the form). */
inline bool
IsAvailable(const char* signature = kShojiSignature)
{
	entry_ref ref;
	return be_roster != NULL && be_roster->FindApp(signature, &ref) == B_OK;
}


/**
 * The form, loaded from Shoji; NULL if it cannot be (not installed, a Shoji without the form, a class that does not load): `error` says what. The view is the
 * host's: it goes into a window (which deletes it), or is deleted.
 */
inline BView*
Instantiate(const char* name = "settings form", status_t* error = NULL, const char* signature = kShojiSignature)
{
	BMessage stub;
	stub.AddString("class", kViewClass);
	stub.AddString("add_on", signature);
	stub.AddString("_name", name);

	BArchivable* object = instantiate_object(&stub);
	BView* view = dynamic_cast<BView*>(object);
	if (view == NULL) {
		delete object;
		if (error != NULL)
			*error = IsAvailable(signature) ? B_NOT_SUPPORTED : B_ENTRY_NOT_FOUND;
	}
	return view;
}


/**
 * Gives the form its data, asynchronously. `target` gets kChanged and kRefused. `schema` and `defaults` are optional (without defaults there is no
 * *Defaults* for the form). B_NOT_INITIALIZED if the view is not in a window yet.
 */
inline status_t
SetData(BView* form, const BMessage& settings, const BMessage* schema, const BMessage* defaults, BMessenger target, bool editable = true,
	const char* rootLabel = NULL)
{
	BMessenger messenger(form);
	if (!messenger.IsValid())
		return B_NOT_INITIALIZED;

	BMessage set(kSetData);
	set.AddInt32(kFieldProtocol, kProtocol);
	set.AddMessage(kFieldSettings, &settings);
	if (schema != NULL)
		set.AddMessage(kFieldSchema, schema);
	if (defaults != NULL)
		set.AddMessage(kFieldDefaults, defaults);
	set.AddMessenger(kFieldTarget, target);
	set.AddBool(kFieldEditable, editable);
	if (rootLabel != NULL)
		set.AddString(kFieldRootLabel, rootLabel);
	return messenger.SendMessage(&set);
}


/** The working copy of the form. Not from the thread of the window that has the form. */
inline status_t
GetData(BView* form, BMessage* settings, bigtime_t timeout = 2000000)
{
	BMessenger messenger(form);
	if (!messenger.IsValid())
		return B_NOT_INITIALIZED;

	BMessage get(kGetData), reply;
	status_t result = messenger.SendMessage(&get, &reply, timeout, timeout);
	if (result == B_OK)
		result = reply.FindMessage(kFieldSettings, settings);
	return result;
}


/** The answer to a button (its @p path, as kAction gave it) did not change the data: the form ends the waiting state of that button (enables it, shows its line as it was) without being made again. */
inline status_t
ActionDone(BView* form, const char* path)
{
	BMessage done(kActionDone);
	if (path != NULL)
		done.AddString(kFieldPath, path);
	return BMessenger(form).SendMessage(&done);
}


/** Back to the data that was set / to the defaults; the form answers with kChanged for the whole message. */
inline status_t
Revert(BView* form)
{
	return BMessenger(form).SendMessage(kRevert);
}


inline status_t
Defaults(BView* form)
{
	return BMessenger(form).SendMessage(kDefaults);
}

}	// namespace form
}	// namespace settings
}	// namespace sen
