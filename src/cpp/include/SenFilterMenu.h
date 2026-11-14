/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenFilterMenu.h
 * @brief A pop-up menu of values that follows what the user types into a text field (header only, Haiku only).
 *
 * FilterPopUpMenu is a BPopUpMenu: it looks and works as every other menu (mouse, selection, marked value) and is attached to a BTextView. It stays out of the way: the text field keeps the keyboard
 * and the caret and is edited as always; the menu opens BELOW the field while the user types and shows the values that contain the text (case does not matter), and follows every change. The keys
 * belong to the field except these: Down/Up move through the list (Down opens it again), Enter takes the selected value, Escape closes the list. Enter without a selection takes the value that
 * the text names (or the only one that is left); text that is none of the values is accepted if `allow_custom` says so, otherwise Enter beeps and the field keeps the text.
 *
 *     sen::FilterPopUpMenu* menu = new sen::FilterPopUpMenu("eventType", new BMessage(kChosen));
 *     menu->SetValues(values);                    // sen::attrvalues::Values (SenAttrValues.h: from the ontology, with allow_custom), or SetValues(texts, meanings)
 *     menu->SetTargetForChoice(this);             // the handler that gets the message
 *     menu->AttachTo(textControl->TextView());    // after the window of the field exists; DetachFromText() or deleting the menu ends it
 *
 * What the user picks from the list is put into the field and sent as a copy of the message given to the constructor with the fields `value` (the text), `meaning` (the Wikidata item, when the value
 * has one) and `custom` (false). A text that is not a value is not sent: Enter with it passes on to the field and the dialog as it does without the menu, the owner reads the text of the field (as
 * for any field), and `IsAcceptable(text)` tells whether it may be taken.
 *
 * Flags: `allow_custom` (other text may be entered) is what the ontology says per attribute; `allow_multiple` (several values at once) is kept for a later use of the control and has no effect yet.
 *
 * Threads: the menu is edited from the thread of the window of the field; the pick comes back to that thread by a message (the menu never locks the window of the field).
 */

#include <Beep.h>
#include <InterfaceDefs.h>
#include <Handler.h>
#include <Looper.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>

#include <string.h>
#include <string>
#include <vector>

#include "SenAttrValues.h"

namespace sen {

class FilterPopUpMenu : public BPopUpMenu {
public:
	/** `message` is the template of what a pick sends (owned by the menu). */
	FilterPopUpMenu(const char* name, BMessage* message)
		:
		BPopUpMenu(name, false, false),
		fMessage(message),
		fText(NULL),
		fFilter(NULL),
		fAllowCustom(false),
		fAllowMultiple(false),
		fNavigated(false)
	{
	}

	virtual ~FilterPopUpMenu()
	{
		DetachFromText();
		delete fMessage;
	}

	/** The values, and what each stands for (may be empty or shorter than the values). */
	void SetValues(const std::vector<std::string>& texts, const std::vector<std::string>& meanings = std::vector<std::string>())
	{
		fTexts = texts;
		fMeanings = meanings;
		fMeanings.resize(fTexts.size());
	}

	/** The values and the flags as the ontology gives them. */
	void SetValues(const attrvalues::Values& values)
	{
		fAllowCustom = values.allowCustom;
		fAllowMultiple = values.allowMultiple;
		SetValues(values.texts, values.meanings);
	}

	/** Other text than the values may be entered. */
	void SetAllowCustom(bool allow) { fAllowCustom = allow; }
	bool AllowCustom() const { return fAllowCustom; }

	/** Several values at once: only kept for now, the menu offers one. */
	void SetAllowMultiple(bool allow) { fAllowMultiple = allow; }
	bool AllowMultiple() const { return fAllowMultiple; }

	/** Where a pick goes. */
	void SetTargetForChoice(const BMessenger& target) { fTarget = target; }
	void SetTargetForChoice(const BHandler* handler, const BLooper* looper = NULL) { fTarget = BMessenger(handler, looper); }

	/** Whether the text may be taken: one of the values (any case), or anything if custom text is allowed (an empty text is always fine: nothing is set). */
	bool IsAcceptable(const char* text) const
	{
		if (text == NULL || text[0] == '\0' || fAllowCustom)
			return true;
		return _ExactMatch(text) >= 0;
	}

	/** Starts following the field: the window of the field must exist (be created), it may be shown or not. */
	void AttachTo(BTextView* textView)
	{
		DetachFromText();
		if (textView == NULL || textView->Window() == NULL)
			return;
		fText = textView;
		fWindow = BMessenger(textView->Window());
		fFilter = new Hook(this);
		BWindow* window = textView->Window();
		if (window->Lock()) {
			window->AddCommonFilter(fFilter);
			window->Unlock();
		}
	}

	void DetachFromText()
	{
		HideList();
		if (fFilter != NULL && fText != NULL) {
			BWindow* window = fText->Window();
			if (window != NULL && window->Lock()) {
				window->RemoveCommonFilter(fFilter);
				window->Unlock();
			}
		}
		delete fFilter;
		fFilter = NULL;
		fText = NULL;
	}

	/** Shows the list for the text of the field now (the keys do it by themselves; call it for a change that did not come from a key). */
	void Refresh(bool openIfClosed = true)
	{
		if (fText == NULL)
			return;
		std::vector<int32> matches;
		_Matches(fText->Text(), &matches);
		if (matches.empty()) {
			HideList();
			return;
		}
		if (!openIfClosed && !IsOpen())
			return;
		_Fill(matches);
		if (!IsOpen()) {
			const BRect bounds = _FieldFrame();
			Go(BPoint(bounds.left, bounds.bottom + 1), true, true, bounds, true);
		} else {
			BWindow* menuWindow = Window();
			if (menuWindow != NULL && menuWindow->Lock()) {
				InvalidateLayout();
				menuWindow->UpdateIfNeeded();
				menuWindow->Unlock();
			}
		}
	}

	/** Closes the list (as Escape closes a menu). */
	void HideList()
	{
		_ForwardKey("\x1b");
		fNavigated = false;
	}

	bool IsOpen() { return Window() != NULL; }

private:
	enum { kRefresh = 'FMrf', kPick = 'FMpk' };

	/** Looks at the keys that go to the field, before the field does. */
	class Hook : public BMessageFilter {
	public:
		Hook(FilterPopUpMenu* menu) : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE), fMenu(menu) {}

		virtual filter_result Filter(BMessage* message, BHandler** target)
		{
			switch (message->what) {
				case B_KEY_DOWN:
					return fMenu->_KeyDown(message);
				case B_WINDOW_ACTIVATED:
					// the user goes to another window: the list is closed (it would stay on the screen)
					if (!message->GetBool("active", true))
						fMenu->HideList();
					return B_DISPATCH_MESSAGE;
				case B_QUIT_REQUESTED:
					fMenu->HideList();
					return B_DISPATCH_MESSAGE;
				case kRefresh:
					fMenu->_Edited();
					return B_SKIP_MESSAGE;
				case kPick:
					fMenu->_Pick(message);
					return B_SKIP_MESSAGE;
				default:
					return B_DISPATCH_MESSAGE;
			}
		}

	private:
		FilterPopUpMenu*	fMenu;
	};

	/** The item of a value: invoked (in the thread of the menu) it only tells the window of the field. */
	class ValueItem : public BMenuItem {
	public:
		ValueItem(FilterPopUpMenu* menu, int32 index, const char* label) : BMenuItem(label, NULL), fMenu(menu), fIndex(index) {}

		virtual status_t Invoke(BMessage* message = NULL)
		{
			BMessage pick(kPick);
			pick.AddInt32("index", fIndex);
			return fMenu->fWindow.SendMessage(&pick);
		}

	private:
		FilterPopUpMenu*	fMenu;
		int32				fIndex;
	};

	/** Gives a key to the menu as if it had the keyboard. A key that ends the tracking (Escape, Enter) deletes the window of the menu while it is locked: it is not unlocked then. */
	bool _ForwardKey(const char* bytes)
	{
		BWindow* menuWindow = Window();
		if (menuWindow == NULL || !menuWindow->Lock())
			return false;
		const BMessenger alive(menuWindow);
		KeyDown(bytes, 1);
		if (alive.IsValid() && Window() == menuWindow)
			menuWindow->Unlock();
		return true;
	}

	/** The field as the user sees it, in screen coordinates: the text view with the border that its control draws around it (not the label of the control). */
	BRect _FieldFrame() const
	{
		BRect frame = fText->ConvertToScreen(fText->Bounds());
		frame.InsetBy(-2, -2);
		return frame;
	}

	static bool _SameText(const std::string& a, const char* b)
	{
		return BString(a.c_str()).ICompare(b) == 0;
	}

	int32 _ExactMatch(const char* text) const
	{
		for (size_t i = 0; i < fTexts.size(); i++)
			if (_SameText(fTexts[i], text))
				return (int32)i;
		return -1;
	}

	void _Matches(const char* text, std::vector<int32>* matches) const
	{
		const BString filter(text != NULL ? text : "");
		for (size_t i = 0; i < fTexts.size(); i++)
			if (filter.Length() == 0 || BString(fTexts[i].c_str()).IFindFirst(filter) >= 0)
				matches->push_back((int32)i);
	}

	void _Fill(const std::vector<int32>& matches)
	{
		BWindow* menuWindow = Window();
		const bool locked = menuWindow != NULL && menuWindow->Lock();
		BMenuItem* item;
		while ((item = RemoveItem((int32)0)) != NULL)
			delete item;
		const BString current(fText->Text());
		for (size_t i = 0; i < matches.size(); i++) {
			BMenuItem* value = new ValueItem(this, matches[i], fTexts[matches[i]].c_str());
			if (_SameText(fTexts[matches[i]], current.String()))
				value->SetMarked(true);
			AddItem(value);
		}
		fNavigated = false;
		if (locked)
			menuWindow->Unlock();
		fLastText = current;
	}

	/** A key for the field, seen before the field. */
	filter_result _KeyDown(BMessage* message)
	{
		if (fText == NULL || fText->Window() == NULL || fText->Window()->CurrentFocus() != fText)
			return B_DISPATCH_MESSAGE;
		const char* bytes = NULL;
		if (message->FindString("bytes", &bytes) != B_OK || bytes == NULL || bytes[0] == '\0')
			return B_DISPATCH_MESSAGE;
		const int32 modifiers = message->GetInt32("modifiers", 0);
		const bool plain = (modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY)) == 0;
		const unsigned char key = (unsigned char)bytes[0];
		const bool single = bytes[1] == '\0';

		if (plain && single && (key == B_DOWN_ARROW || key == B_UP_ARROW)) {
			if (!IsOpen()) {
				if (key == B_DOWN_ARROW)
					Refresh();
				else
					return B_DISPATCH_MESSAGE;
			} else {
				if (_ForwardKey(bytes))
					fNavigated = true;
			}
			return B_SKIP_MESSAGE;
		}
		if (plain && single && key == B_ESCAPE && IsOpen()) {
			HideList();
			return B_SKIP_MESSAGE;
		}
		if (plain && single && key == B_ENTER)
			return _Enter();

		if ((modifiers & B_COMMAND_KEY) != 0) {
			// a shortcut (quit, close, save): the list goes first, as a window with an open menu cannot be closed; only what changes the text (paste, cut, undo) opens it again
			HideList();
			if (!(single && strchr("vxzyVXZY", key) != NULL))
				return B_DISPATCH_MESSAGE;
		}

		// anything else is for the field; look at the text after the field has had it
		fWindow.SendMessage(kRefresh);
		return B_DISPATCH_MESSAGE;
	}

	filter_result _Enter()
	{
		const char* text = fText->Text();
		if (IsOpen() && fNavigated) {
			// the selected item: the menu takes it as for any menu
			_ForwardKey("\n");
			return B_SKIP_MESSAGE;
		}
		int32 index = _ExactMatch(text);
		if (index < 0 && IsOpen() && !fAllowCustom) {
			std::vector<int32> matches;
			_Matches(text, &matches);
			if (matches.size() == 1)
				index = matches[0];
		}
		if (index >= 0) {
			BMessage pick(kPick);
			pick.AddInt32("index", index);
			_Pick(&pick);
			return B_SKIP_MESSAGE;
		}
		if (text[0] == '\0' || fAllowCustom) {
			HideList();
			return B_DISPATCH_MESSAGE;		// the field and the dialog take the text as it is
		}
		system_beep(NULL);
		return B_SKIP_MESSAGE;
	}

	/** The text of the field has changed. */
	void _Edited()
	{
		if (fText == NULL)
			return;
		if (fLastText == fText->Text() && IsOpen())
			return;
		fLastText = "";
		Refresh();
	}

	/** A value has been picked (by Enter or the mouse): into the field, and to the target. */
	void _Pick(BMessage* pick)
	{
		const int32 index = pick->GetInt32("index", -1);
		if (index < 0 || (size_t)index >= fTexts.size() || fText == NULL)
			return;
		HideList();
		fText->SetText(fTexts[index].c_str());
		fText->Select(fText->TextLength(), fText->TextLength());
		fLastText = fText->Text();

		BMessage* message = fMessage != NULL ? new BMessage(*fMessage) : new BMessage();
		message->RemoveName("value");
		message->RemoveName("meaning");
		message->RemoveName("custom");
		message->AddString("value", fTexts[index].c_str());
		if (!fMeanings[index].empty())
			message->AddString("meaning", fMeanings[index].c_str());
		message->AddBool("custom", false);
		fTarget.SendMessage(message);
		delete message;
	}

	BMessage*					fMessage;
	std::vector<std::string>	fTexts;
	std::vector<std::string>	fMeanings;
	BMessenger					fTarget;
	BMessenger					fWindow;
	BTextView*					fText;
	Hook*						fFilter;
	BString						fLastText;
	bool						fAllowCustom;
	bool						fAllowMultiple;
	bool						fNavigated;		///< an arrow key has selected an item since the list was filled
};

}	// namespace sen
