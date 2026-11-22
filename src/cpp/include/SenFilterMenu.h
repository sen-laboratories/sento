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
 * and the caret and is edited as always; the menu opens BELOW the field while the user types and shows the values that BEGIN with the text (case does not matter), or, if there are none, those that have it anywhere; the part that matched is drawn in bold. It follows every change. The keys
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
 * A small arrow at the right end of the field (the field is a combo box) tells that there are values and opens the whole list when it is clicked; `SetShowArrow(false)` leaves it out.
 *
 * Flags: `allow_custom` (other text may be entered) is what the ontology says per attribute; `allow_multiple` (several values at once) is kept for a later use of the control and has no effect yet.
 *
 * Keys: while the list is open Haiku gives the keyboard to the window of the menu, so the menu passes every key that is not its own (arrows, Enter, Escape) on to the field, through the filters of the
 * field as a key from the keyboard would go (the file manager's editor ends its edit with Enter and Tab by a filter).
 *
 * Threads: the menu is edited from the thread of the window of the field; the pick comes back to that thread by a message (the menu never locks the window of the field).
 */

#include <Beep.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <Handler.h>
#include <List.h>
#include <Looper.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <OS.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <SeparatorItem.h>
#include <String.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

#include <math.h>
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
		fArrow(NULL),
		fSavedRightInset(0),
		fShowArrow(true),
		fShowAll(false),
		fAllowCustom(false),
		fAllowMultiple(false),
		fEnterAfterPick(false),
		fPassEnter(false),
		fNavigated(false),
		fHand(false),
		fArrowPressed(false)
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

	/** Where the list is for a field that ends its edit with Enter (the file manager): after a pick the menu sends that Enter itself, so that one key stores the value. */
	void SetEnterAfterPick(bool send) { fEnterAfterPick = send; }

	/** The arrow in the field (on by default; set before AttachTo). */
	void SetShowArrow(bool show) { fShowArrow = show; }

	/**
	 * What the arrow does when the menu has no values: it sends this message to the target (a field that is not a list but has a control of its own, such as a calendar for a day). With values the arrow
	 * opens the list as always; without values and without this message there is no arrow. Set before AttachTo.
	 */
	void SetArrowMessage(const BMessenger& target, const BMessage& message)
	{
		fArrowTarget = target;
		fArrowMessage = message;
		fHasArrowMessage = true;
	}

	/**
	 * An item below the values that is always in the list (not a value: it does not fill the field): a click or Enter on it sends this message to the target (a list of repeat cycles ends with
	 * `Custom...`, which opens a window of its own). Set before the list is shown.
	 */
	void SetExtraItem(const char* label, const BMessenger& target, const BMessage& message)
	{
		fExtraLabel = label != NULL ? label : "";
		fExtraTarget = target;
		fExtraMessage = message;
		fHasExtra = !fExtraLabel.empty();
	}

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
			if (fShowArrow && (!fTexts.empty() || fHasArrowMessage) && textView->Parent() != NULL) {
				// the arrow is at the right end of the field, the text stops before it
				const float width = ceilf(textView->LineHeight() * 1.1f);
				BRect frame = textView->Frame();
				frame.left = frame.right - width + 1;
				frame.InsetBy(0, 1);
				float left, top, right, bottom;
				textView->GetInsets(&left, &top, &right, &bottom);
				fSavedRightInset = right;
				textView->SetInsets(left, top, right + width, bottom);
				fArrow = new Arrow(this, frame);
				textView->Parent()->AddChild(fArrow);
			}
			window->Unlock();
		}
	}

	void DetachFromText()
	{
		HideList();
		Hook* hook = fFilter;
		BLooper* looper = NULL;
		fWindow.Target(&looper);
		BTextView* text = fText;
		Arrow* arrow = fArrow;
		fFilter = NULL;
		fText = NULL;
		fArrow = NULL;
		if (hook != NULL) {
			BWindow* window = dynamic_cast<BWindow*>(looper);
			if (window != NULL && window->Lock()) {
				window->RemoveCommonFilter(hook);
				if (arrow != NULL) {
					arrow->fMenu = NULL;
					arrow->RemoveSelf();
					delete arrow;
					if (text != NULL) {
						float left, top, right, bottom;
						text->GetInsets(&left, &top, &right, &bottom);
						text->SetInsets(left, top, fSavedRightInset, bottom);
					}
				}
				window->Unlock();
			}
			delete hook;
		}
		fWindow = BMessenger();
	}

	/** Shows the list for the text of the field now (the keys do it by themselves; call it for a change that did not come from a key). */
	void Refresh(bool openIfClosed = true)
	{
		if (fText == NULL)
			return;
		std::vector<int32> matches, offsets;
		const char* filter = fShowAll ? "" : fText->Text();
		_Matches(filter, &matches, &offsets);
		if (matches.empty()) {
			HideList();
			return;
		}
		if (!openIfClosed && !IsOpen())
			return;
		_Fill(matches, offsets, (int32)strlen(filter));
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

	/** Opens the list with all values (what the arrow does); typing filters it from then on. */
	void ShowAll()
	{
		if (fTexts.empty() && fHasArrowMessage) {
			BMessage message(fArrowMessage);
			fArrowTarget.SendMessage(&message);
			return;
		}
		fShowAll = true;
		fLastText = "";
		Refresh();
	}

	/** Closes the list (as Escape closes a menu). */
	void HideList()
	{
		_ForwardKey("\x1b");
		fNavigated = false;
	}

	bool IsOpen() { return Window() != NULL; }

	/** Called by the window of the menu while the list is open (it has the keyboard then): the menu keeps the keys of a list and gives the others to the field. */
	virtual void KeyDown(const char* bytes, int32 numBytes)
	{
		if (fText == NULL || !fWindow.IsValid() || bytes == NULL || numBytes <= 0) {
			BPopUpMenu::KeyDown(bytes, numBytes);
			return;
		}
		const unsigned char key = (unsigned char)bytes[0];
		if (numBytes == 1 && (key == B_UP_ARROW || key == B_DOWN_ARROW)) {
			fNavigated = true;
			BPopUpMenu::KeyDown(bytes, numBytes);
			return;
		}
		if (numBytes == 1 && key == B_ESCAPE) {
			BPopUpMenu::KeyDown(bytes, numBytes);
			return;
		}
		if (numBytes == 1 && key == B_ENTER) {
			if (fNavigated)
				BPopUpMenu::KeyDown(bytes, numBytes);		// the selected item
			else
				_Send(kEnterKey);			// the field decides (exact value, custom text)
			return;
		}
		// the rest (letters, Backspace, Delete, Left/Right, Tab...) is for the field
		BMessage through(kKeyThrough);
		through.AddString("bytes", std::string(bytes, (size_t)numBytes).c_str());
		through.AddInt32("modifiers", ::modifiers());
		_Send(&through);
	}

private:
	/** Sends a message of this list to its window; it carries the list, so that the filters of the other lists of the window leave it alone. */
	status_t _Send(uint32 what)
	{
		BMessage message(what);
		return _Send(&message);
	}

	status_t _Send(BMessage* message)
	{
		message->RemoveName("sen:menu");
		message->AddPointer("sen:menu", this);
		return fWindow.SendMessage(message);
	}

	enum { kRefresh = 'FMrf', kPick = 'FMpk', kKeyThrough = 'FMky', kEnterKey = 'FMen', kShowAllValues = 'FMsa', kPlaceArrow = 'FMpa' };

	/**
	 * The arrow at the right end of the field: a view over the text field that only draws. The mouse is handled by the filter of the window (`_MouseDown()` and the others), by the frame that
	 * the arrow has: the text view lies under the arrow and is the target of the mouse there.
	 */
	class Arrow : public BView {
	public:
		Arrow(FilterPopUpMenu* menu, BRect frame)
			:
			BView(frame, "values arrow", B_FOLLOW_NONE, B_WILL_DRAW),
			fMenu(menu)
		{
			SetViewColor(B_TRANSPARENT_COLOR);
		}

		virtual ~Arrow()
		{
			if (fMenu != NULL && fMenu->fArrow == this)
				fMenu->fArrow = NULL;
		}

		virtual void Draw(BRect updateRect)
		{
			if (fMenu == NULL || fMenu->fText == NULL)
				return;
			// (the field is laid out after the menu was attached: the arrow moves to the right end of the field, by a message, as a view is not moved while it is drawn)
			if (fMenu->_ArrowFrame() != Frame())
				fMenu->_Send(kPlaceArrow);
			const rgb_color base = fMenu->fText->ViewColor();
			SetHighColor(base);
			FillRect(Bounds());
			if (!fMenu->fText->IsEditable())
				return;
			// the filled triangle pointing down that the lists of Tracker and the date fields have (BControlLook::DrawArrowShape()), in the middle
			const BRect bounds(Bounds());
			const float side = ceilf(be_plain_font->Size() * 0.7f);
			const float x = floorf(bounds.left + (bounds.Width() + 1 - side) / 2);
			const float y = floorf(bounds.top + (bounds.Height() + 1 - side) / 2);
			const rgb_color panel = ui_color(B_PANEL_BACKGROUND_COLOR);
			float tint = B_DARKEN_4_TINT;
			if (panel.red + panel.green + panel.blue <= 128 * 3)
				tint = B_LIGHTEN_2_TINT;
			PushState();
			BRect arrow(x, y, x + side - 1, y + side - 1);
			be_control_look->DrawArrowShape(this, arrow, updateRect, base, BControlLook::B_DOWN_ARROW, 0, tint);
			PopState();
		}

	private:
		friend class FilterPopUpMenu;
		FilterPopUpMenu*	fMenu;
	};

	/** Looks at the keys that go to the field, before the field does. */
	class Hook : public BMessageFilter {
	public:
		Hook(FilterPopUpMenu* menu) : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE), fMenu(menu) {}

		// (a window deletes its filters when it goes: the menu must not look at the window or the field after that)
		virtual ~Hook()
		{
			if (fMenu != NULL && fMenu->fFilter == this) {
				fMenu->fFilter = NULL;
				fMenu->fText = NULL;
				if (fMenu->fArrow != NULL)
					fMenu->fArrow->fMenu = NULL;
				fMenu->fArrow = NULL;
				fMenu->fWindow = BMessenger();
			}
		}

		virtual filter_result Filter(BMessage* message, BHandler** target)
		{
			// (all the lists of a window have their filters in the same chain: a message of the list of another field is left to that one)
			switch (message->what) {
				case kRefresh: case kPlaceArrow: case kShowAllValues: case kPick: case kKeyThrough: case kEnterKey:
					if (message->GetPointer("sen:menu", (void*)NULL) != (void*)fMenu)
						return B_DISPATCH_MESSAGE;
					break;
				default:
					break;
			}
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
				case B_WINDOW_RESIZED:
					fMenu->_PlaceArrow();
					return B_DISPATCH_MESSAGE;
				case B_MOUSE_DOWN:
					return fMenu->_MouseDown();
				case B_MOUSE_UP:
					return fMenu->_MouseUp();
				case B_MOUSE_MOVED:
					return fMenu->_MouseMoved(message);
				case kRefresh:
					fMenu->_Edited();
					return B_SKIP_MESSAGE;
				case kPlaceArrow:
					fMenu->_PlaceArrow();
					return B_SKIP_MESSAGE;
				case kShowAllValues:
					fMenu->ShowAll();
					return B_SKIP_MESSAGE;
				case kPick:
					fMenu->_Pick(message);
					return B_SKIP_MESSAGE;
				case kKeyThrough:
				{
					// (the menu and this filter can be gone when the key has been handled: nothing is touched afterwards)
					FilterPopUpMenu* menu = fMenu;
					menu->_PassKey(message->GetString("bytes", ""), message->GetInt32("modifiers", 0));
					return B_SKIP_MESSAGE;
				}
				case kEnterKey:
				{
					FilterPopUpMenu* menu = fMenu;
					if (menu->_Enter() == B_DISPATCH_MESSAGE)
						menu->_PassKey("\n", 0);
					return B_SKIP_MESSAGE;
				}
				default:
					return B_DISPATCH_MESSAGE;
			}
		}

	private:
		FilterPopUpMenu*	fMenu;
	};

	/**
	 * A disabled item without size. BMenu cannot select the only item of a menu with the arrow keys (its search for the next item loops over the OTHER items, of which there are none),
	 * so a list of one value gets this one as a second item to make the arrow find the first.
	 */
	class Companion : public BMenuItem {
	public:
		Companion() : BMenuItem("", NULL) { SetEnabled(false); }

		virtual void GetContentSize(float* width, float* height)
		{
			*width = 0;
			*height = 0;
		}

		virtual void DrawContent() {}
	};

	/** The item below the values (SetExtraItem()): invoked, it tells the window of the field, which sends its message. */
	class ExtraItem : public BMenuItem {
	public:
		ExtraItem(FilterPopUpMenu* menu, const char* label) : BMenuItem(label, NULL), fMenu(menu) {}

		virtual status_t Invoke(BMessage* message = NULL)
		{
			BMessage pick(kPick);
			pick.AddBool("extra", true);
			return fMenu->_Send(&pick);
		}

	private:
		FilterPopUpMenu*	fMenu;
	};

	/**
	 * The item of a value: invoked (in the thread of the menu) it only tells the window of the field. The part of the label that the text of the field matched is drawn in bold, so that
	 * it can be seen why the value is in the list (the label is drawn in pieces, with the font of the menu and its bold face).
	 */
	class ValueItem : public BMenuItem {
	public:
		ValueItem(FilterPopUpMenu* menu, int32 index, const char* label, int32 matchOffset, int32 matchLength)
			:
			BMenuItem(label, NULL),
			fMenu(menu),
			fIndex(index),
			fMatchOffset(matchOffset),
			fMatchLength(matchLength)
		{
		}

		virtual void GetContentSize(float* width, float* height)
		{
			BMenuItem::GetContentSize(width, height);
			BMenu* menu = Menu();
			if (menu == NULL || fMatchOffset < 0 || fMatchLength <= 0)
				return;
			BFont font, bold;
			_Fonts(menu, &font, &bold);
			const BString label(Label());
			BString before, match, after;
			_Pieces(label, &before, &match, &after);
			*width = font.StringWidth(before.String()) + bold.StringWidth(match.String()) + font.StringWidth(after.String());
		}

		virtual void DrawContent()
		{
			BMenu* menu = Menu();
			if (menu == NULL || fMatchOffset < 0 || fMatchLength <= 0) {
				BMenuItem::DrawContent();
				return;
			}
			BFont font, bold;
			_Fonts(menu, &font, &bold);
			const BString label(Label());
			BString before, match, after;
			_Pieces(label, &before, &match, &after);

			menu->PushState();
			menu->SetHighColor(ui_color(IsSelected() ? B_MENU_SELECTED_ITEM_TEXT_COLOR : B_MENU_ITEM_TEXT_COLOR));
			// (the pen is on the baseline: the label is in the middle of the height of the item)
			font_height height;
			font.GetHeight(&height);
			const BRect frame = Frame();
			menu->MovePenTo(BPoint(ContentLocation().x, frame.top + (frame.Height() + 1 - (height.ascent + height.descent)) / 2 + height.ascent));
			menu->SetFont(&font);
			menu->DrawString(before.String());
			menu->SetFont(&bold);
			menu->DrawString(match.String());
			menu->SetFont(&font);
			menu->DrawString(after.String());
			menu->PopState();
		}

		virtual status_t Invoke(BMessage* message = NULL)
		{
			BMessage pick(kPick);
			pick.AddInt32("index", fIndex);
			return fMenu->_Send(&pick);
		}

	private:
		static void _Fonts(BMenu* menu, BFont* font, BFont* bold)
		{
			menu->GetFont(font);
			*bold = *font;
			bold->SetFace(B_BOLD_FACE);
		}

		void _Pieces(const BString& label, BString* before, BString* match, BString* after) const
		{
			label.CopyInto(*before, 0, fMatchOffset);
			label.CopyInto(*match, fMatchOffset, fMatchLength);
			label.CopyInto(*after, fMatchOffset + fMatchLength, label.Length() - fMatchOffset - fMatchLength);
		}

		FilterPopUpMenu*	fMenu;
		int32				fIndex;
		int32				fMatchOffset;	///< where the text of the field is in the label (-1: nowhere, the whole label is plain)
		int32				fMatchLength;
	};

	/** The frame that the arrow has at the right end of the text view, in the coordinates of the parent of the view. */
	BRect _ArrowFrame() const
	{
		const float width = ceilf(fText->LineHeight() * 1.1f);
		BRect frame = fText->Frame();
		frame.left = frame.right - width + 1;
		frame.InsetBy(0, 1);
		return frame;
	}

	/** Puts the arrow where it belongs. */
	void _PlaceArrow()
	{
		if (fText == NULL || fArrow == NULL || fText->Window() == NULL)
			return;
		const BRect frame = _ArrowFrame();
		if (frame == fArrow->Frame())
			return;
		fArrow->MoveTo(frame.LeftTop());
		fArrow->ResizeTo(frame.Width(), frame.Height());
		fArrow->Invalidate();
	}

	/**
	 * Whether the pointer is over the arrow, by the frame that the arrow has now. The position is asked from the window: the position in a mouse message is relative to the window, to the
	 * view or to the screen, depending on the message and the field.
	 */
	bool _OverArrow()
	{
		if (fText == NULL || fArrow == NULL || fArrow->IsHidden() || fArrow->Parent() == NULL || fText->Window() == NULL || !fText->IsEditable())
			return false;
		_PlaceArrow();
		BPoint where;
		uint32 buttons;
		fArrow->Parent()->GetMouse(&where, &buttons, false);
		return fArrow->Frame().Contains(where);
	}

	void _ShowHand(bool hand)
	{
		if (fText == NULL || fHand == hand)
			return;
		fHand = hand;
		// (the text view sets the I beam again when the pointer is over its text)
		const BCursor cursor(hand ? B_CURSOR_ID_FOLLOW_LINK : B_CURSOR_ID_I_BEAM);
		fText->SetViewCursor(&cursor);
	}

	/**
	 * A press and a release on the arrow open the list, as a button does (on the release). They always go on to the window: the file manager waits for the release of every press that it got.
	 */
	filter_result _MouseDown()
	{
		fArrowPressed = _OverArrow();
		return B_DISPATCH_MESSAGE;
	}

	filter_result _MouseUp()
	{
		if (fArrowPressed) {
			fArrowPressed = false;
			// (opened by a message of its own: not in the middle of the mouse event, whatever state the window or the file manager is in then)
			if (_OverArrow())
				_Send(kShowAllValues);
		}
		return B_DISPATCH_MESSAGE;
	}

	/** The hand is the field's to keep while the pointer is over the arrow (the text view would set the I beam); a drag that was started elsewhere goes on. */
	filter_result _MouseMoved(BMessage* message)
	{
		const bool over = _OverArrow();
		_ShowHand(over);
		return over && message->GetInt32("buttons", 0) == 0 ? B_SKIP_MESSAGE : B_DISPATCH_MESSAGE;
	}

	/** Gives a key to the menu as if it had the keyboard. A key that ends the tracking (Escape, Enter) deletes the window of the menu while it is locked: it is not unlocked then. */
	bool _ForwardKey(const char* bytes)
	{
		BWindow* menuWindow = Window();
		if (menuWindow == NULL || !menuWindow->Lock())
			return false;
		const BMessenger alive(menuWindow);
		KeyDown(bytes, 1);
		// (a window that is still there is unlocked, also when the menu has left it by now: the thread that tracks the menu waits for this lock, and the one that deletes the menu waits for that thread)
		if (alive.IsValid())
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

	/**
	 * The values for the text: those that BEGIN with it; if there are none, those that have it anywhere (case does not matter). `offsets` (if given) has where the text is in each of them.
	 */
	void _Matches(const char* text, std::vector<int32>* matches, std::vector<int32>* offsets = NULL) const
	{
		const BString filter(text != NULL ? text : "");
		for (int pass = 0; pass < 2 && matches->empty(); pass++) {
			for (size_t i = 0; i < fTexts.size(); i++) {
				const int32 at = filter.Length() == 0 ? 0 : BString(fTexts[i].c_str()).IFindFirst(filter);
				if (at < 0 || (pass == 0 && at != 0))
					continue;
				matches->push_back((int32)i);
				if (offsets != NULL)
					offsets->push_back(filter.Length() == 0 ? -1 : at);
			}
			if (filter.Length() == 0)
				break;
		}
	}

	void _Fill(const std::vector<int32>& matches, const std::vector<int32>& offsets, int32 filterLength)
	{
		BWindow* menuWindow = Window();
		const bool locked = menuWindow != NULL && menuWindow->Lock();
		BMenuItem* item;
		while ((item = RemoveItem((int32)0)) != NULL)
			delete item;
		const BString current(fText->Text());
		for (size_t i = 0; i < matches.size(); i++) {
			BMenuItem* value = new ValueItem(this, matches[i], fTexts[matches[i]].c_str(), i < offsets.size() ? offsets[i] : -1, filterLength);
			if (_SameText(fTexts[matches[i]], current.String()))
				value->SetMarked(true);
			AddItem(value);
		}
		if (fHasExtra) {
			AddItem(new BSeparatorItem());
			ExtraItem* extra = new ExtraItem(this, fExtraLabel.c_str());
			// (the list of all values, and the field has a value that is none of them (every 2 weeks on Monday): that is what the extra item, `Custom...`, stands for, and it is checked)
			if (fShowAll && !current.IsEmpty() && _ExactMatch(current.String()) < 0)
				extra->SetMarked(true);
			AddItem(extra);
		} else if (matches.size() == 1)
			AddItem(new Companion());
		fNavigated = false;
		if (locked)
			menuWindow->Unlock();
		fLastText = current;
	}

	/**
	 * A key that the menu got instead of the field: it goes to the field as a key from the keyboard would, through the filters of the field (they may end the edit: the menu may be deleted by
	 * then, so nothing is touched after a filter has skipped the key), and the list follows the new text.
	 */
	void _PassKey(const char* bytes, int32 modifiers)
	{
		BTextView* text = fText;
		if (text == NULL || bytes == NULL || bytes[0] == '\0')
			return;
		BMessage key(B_KEY_DOWN);
		key.AddInt64("when", system_time());
		key.AddInt32("key", 0);
		key.AddInt32("modifiers", modifiers);
		key.AddInt32("raw_char", (unsigned char)bytes[0]);
		key.AddString("bytes", bytes);
		for (const char* b = bytes; *b != '\0'; b++)
			key.AddInt8("byte", *b);
		if ((unsigned char)bytes[0] >= 0x20 && bytes[0] != 0x7F)
			_Send(kRefresh);		// (queued behind this key)
		BHandler* target = text;
		BList* filters = text->FilterList();
		for (int32 i = 0; filters != NULL && i < filters->CountItems(); i++) {
			BMessageFilter* filter = static_cast<BMessageFilter*>(filters->ItemAt(i));
			if (filter->FiltersAnyCommand() || filter->Command() == B_KEY_DOWN) {
				if (filter->Filter(&key, &target) == B_SKIP_MESSAGE)
					return;
			}
		}
		text->KeyDown(bytes, (int32)strlen(bytes));
		_Send(kRefresh);
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
		_Send(kRefresh);
		return B_DISPATCH_MESSAGE;
	}

	filter_result _Enter()
	{
		if (fPassEnter) {
			fPassEnter = false;
			return B_DISPATCH_MESSAGE;
		}
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
		fShowAll = false;
		Refresh();
	}

	/** A value has been picked (by Enter or the mouse): into the field, and to the target. */
	void _Pick(BMessage* pick)
	{
		if (pick->GetBool("extra", false)) {
			HideList();
			BMessage message(fExtraMessage);
			fExtraTarget.SendMessage(&message);
			return;
		}
		const int32 index = pick->GetInt32("index", -1);
		if (index < 0 || (size_t)index >= fTexts.size() || fText == NULL)
			return;
		HideList();
		fText->SetText(fTexts[index].c_str());
		fText->Select(fText->TextLength(), fText->TextLength());
		fLastText = fText->Text();

		if (fEnterAfterPick) {
			BMessage enter(B_KEY_DOWN);
			enter.AddInt64("when", system_time());
			enter.AddInt32("key", 0x47);
			enter.AddInt32("modifiers", 0);
			enter.AddInt32("raw_char", B_ENTER);
			enter.AddString("bytes", "\n");
			enter.AddInt8("byte", B_ENTER);
			fPassEnter = true;
			fWindow.SendMessage(&enter);
		}

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
	BMessenger					fArrowTarget;
	std::string					fExtraLabel;	///< SetExtraItem()
	BMessenger					fExtraTarget;
	BMessage					fExtraMessage;
	bool						fHasExtra = false;
	BMessage					fArrowMessage;
	bool						fHasArrowMessage = false;
	BMessenger					fWindow;
	BTextView*					fText;
	Hook*						fFilter;
	BString						fLastText;
	Arrow*						fArrow;			///< the arrow in the field (a child of the parent of the text view)
	float						fSavedRightInset;
	bool						fShowArrow;
	bool						fShowAll;		///< the list shows all values, as the arrow was clicked, until the text is edited
	bool						fAllowCustom;
	bool						fAllowMultiple;
	bool						fEnterAfterPick;
	bool						fPassEnter;		///< the Enter that was sent after a pick goes to the field
	bool						fNavigated;		///< an arrow key has selected an item since the list was filled
	bool						fHand;			///< the pointer is over the arrow and the field shows the hand
	bool						fArrowPressed;	///< the press was on the arrow
};

}	// namespace sen
