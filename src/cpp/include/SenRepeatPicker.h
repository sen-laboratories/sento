/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenRepeatPicker.h
 * @brief The window that sets how a thing repeats: every N days, weeks, months or years, on which weekdays, and how long (Haiku only, header only).
 *
 * It is what `Custom...` at the end of the list of Repeats opens (FilterPopUpMenu::SetExtraItem()) in Shoji and in the lists of Akita. It writes what it makes into the file itself, the words
 * (`schema:repeatFrequency`, `every 2 weeks on Monday and Wednesday, 10 times`) and the rule beside them (`SEN:recurrence`, `FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE;COUNT=10`), so that they agree
 * (SenRecurrence.h). *Doesn't repeat* takes the words away and writes an empty rule. A rule that says more than the window can (the second Tuesday of the month) is shown as it is in words and
 * is replaced when OK is pressed. The window is a window like another: it has the keyboard, and it goes with Escape or Cancel.
 *
 *     new sen::RepeatPickerWindow(BPoint(x, y), fileRef, BMessenger(this), &changedMessage);    // shows itself; `changedMessage` is sent to the target after a change
 */

#include <Button.h>
#include <Control.h>
#include <ControlLook.h>
#include <CheckBox.h>
#include <DateFormat.h>
#include <Entry.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Node.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <Screen.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <string>

#include "SenRecurrence.h"

namespace sen {

/** The texts of the window (English unless the caller translates them). */
struct RepeatPickerLabels {
	std::string title = "Repeat";
	std::string every = "Every";
	std::string units[4] = {"days", "weeks", "months", "years"};
	std::string ends = "Ends";
	std::string never = "Never";
	std::string after = "After";
	std::string times = "times";
	std::string on = "On";
	std::string untilHint = "YYYY-MM-DD";
	std::string doesNotRepeat = "Doesn't repeat";
	std::string ok = "OK";
	std::string cancel = "Cancel";
	std::string more = "This repeat says more than can be set here; OK replaces it.";
};

/**
 * A whole number with the stacked arrows of the date and time fields of Shoji (filled triangles in the right part of the field: the upper raises, the lower lowers; the keys Up and Down and the wheel do
 * the same, digits can be typed). It sends its message when the value changes.
 */
class StepField : public BControl {
public:
	StepField(const char* name, BMessage* message)
		:
		BControl(name, "", message, B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE),
		fMin(0),
		fMax(999),
		fTyped(false)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		font_height height;
		be_plain_font->GetHeight(&height);
		const BSize size(ceilf(be_plain_font->StringWidth("0000")) + 4 + kArrowWidth + 8, ceilf(height.ascent + height.descent + height.leading) + 8);
		SetExplicitMinSize(size);
		SetExplicitMaxSize(size);
		SetExplicitPreferredSize(size);
	}

	void SetRange(int32 minimum, int32 maximum)
	{
		fMin = minimum;
		fMax = maximum;
	}

	virtual void SetValue(int32 value)
	{
		value = value < fMin ? fMin : (value > fMax ? fMax : value);
		const bool changed = value != Value();
		BControl::SetValue(value);
		fTyped = false;
		if (changed && Window() != NULL)
			Invoke();
		if (Window() != NULL)
			Invalidate();
	}

	virtual void SetEnabled(bool enabled)
	{
		BControl::SetEnabled(enabled);
		Invalidate();
	}

	virtual void Draw(BRect updateRect)
	{
		const rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		BRect rect = Bounds();
		uint32 flags = 0;
		if (!IsEnabled())
			flags |= BControlLook::B_DISABLED;
		if (IsFocus() && Window() != NULL && Window()->IsActive())
			flags |= BControlLook::B_FOCUSED;
		be_control_look->DrawTextControlBorder(this, rect, updateRect, base, flags);
		SetHighColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		FillRect(rect);

		// the number, in the left part
		char text[16];
		snprintf(text, sizeof(text), "%d", (int)(fTyped ? fTypedValue : Value()));
		font_height height;
		be_plain_font->GetHeight(&height);
		SetHighColor(ui_color(IsEnabled() ? B_DOCUMENT_TEXT_COLOR : B_CONTROL_TEXT_COLOR));
		if (!IsEnabled())
			SetHighColor(tint_color(ui_color(B_DOCUMENT_BACKGROUND_COLOR), B_DARKEN_2_TINT));
		SetFont(be_plain_font);
		DrawString(text, BPoint(rect.left + 3, floorf(rect.top + (rect.Height() + 1 - (height.ascent + height.descent)) / 2 + height.ascent)));

		// the arrows (as ShojiDateControl draws them): the filled triangles of BControlLook::DrawArrowShape(), each in its half of the area at the right
		const BRect area = _ArrowArea();
		const float half = floorf(area.Height() / 2);
		const float side = std::max(4.0f, std::min(std::min(half - 2, area.Width() - 2), floorf(be_plain_font->Size() * 0.7f)));
		PushState();
		SetLowUIColor(B_DOCUMENT_BACKGROUND_COLOR);
		FillRect(area, B_SOLID_LOW);
		float tint = IsEnabled() ? B_DARKEN_4_TINT : B_DARKEN_2_TINT;
		if (base.red + base.green + base.blue <= 128 * 3)
			tint = IsEnabled() ? B_LIGHTEN_2_TINT : B_DARKEN_1_TINT;
		const float x = floorf(area.left + (area.Width() + 1 - side) / 2);
		BRect up(x, floorf(area.top + (half - side) / 2), x + side - 1, 0);
		up.bottom = up.top + side - 1;
		BRect down(x, floorf(area.top + half + (area.Height() + 1 - half - side) / 2), x + side - 1, 0);
		down.bottom = down.top + side - 1;
		be_control_look->DrawArrowShape(this, up, updateRect, base, BControlLook::B_UP_ARROW, 0, tint);
		be_control_look->DrawArrowShape(this, down, updateRect, base, BControlLook::B_DOWN_ARROW, 0, tint);
		PopState();
	}

	virtual void MouseDown(BPoint where)
	{
		if (!IsEnabled())
			return;
		MakeFocus(true);
		const BRect area = _ArrowArea();
		if (area.Contains(where))
			_Step(where.y < area.top + area.Height() / 2 ? 1 : -1);
	}

	virtual void KeyDown(const char* bytes, int32 numBytes)
	{
		if (!IsEnabled() || numBytes != 1) {
			BControl::KeyDown(bytes, numBytes);
			return;
		}
		const char key = bytes[0];
		if (key == B_UP_ARROW)
			_Step(1);
		else if (key == B_DOWN_ARROW)
			_Step(-1);
		else if (key >= '0' && key <= '9') {
			int32 typed = (fTyped ? fTypedValue : 0) * 10 + (key - '0');
			if (typed > fMax)
				typed = key - '0';
			fTypedValue = typed;
			fTyped = true;
			Invalidate();
		} else if (key == B_BACKSPACE && fTyped) {
			fTypedValue /= 10;
			Invalidate();
		} else if (key == B_ENTER || key == B_TAB) {
			_Commit();
			BControl::KeyDown(bytes, numBytes);
		} else
			BControl::KeyDown(bytes, numBytes);
	}

	virtual void MakeFocus(bool focused = true)
	{
		if (!focused)
			_Commit();
		BControl::MakeFocus(focused);
		Invalidate();
	}

	virtual void MessageReceived(BMessage* message)
	{
		if (message->what == B_MOUSE_WHEEL_CHANGED && IsEnabled() && IsFocus()) {
			const float delta = message->GetFloat("be:wheel_delta_y", 0);
			if (delta != 0)
				_Step(delta < 0 ? 1 : -1);
			return;
		}
		BControl::MessageReceived(message);
	}

private:
	static const int32 kArrowWidth = 14;

	BRect _ArrowArea() const
	{
		const BRect inner = Bounds().InsetByCopy(2, 2);
		return BRect(inner.right - (kArrowWidth - 1), inner.top, inner.right, inner.bottom);
	}

	void _Step(int32 by)
	{
		_Commit();
		SetValue(Value() + by);
	}

	/** the digits that were typed become the value (at least the minimum) */
	void _Commit()
	{
		if (!fTyped)
			return;
		const int32 typed = fTypedValue;
		fTyped = false;
		SetValue(typed < fMin ? fMin : typed);
	}

	int32	fMin;
	int32	fMax;
	bool	fTyped;			///< digits are being typed
	int32	fTypedValue = 0;
};


/** The name of a weekday under its box: a click on the name sets the box, too. */
class DayLabel : public BStringView {
public:
	DayLabel(const char* label, BCheckBox* box) : BStringView("dayname", label), fBox(box)
	{
		SetAlignment(B_ALIGN_CENTER);
	}

	virtual void MouseDown(BPoint)
	{
		if (!fBox->IsEnabled())
			return;
		fBox->SetValue(fBox->Value() == B_CONTROL_ON ? B_CONTROL_OFF : B_CONTROL_ON);
		fBox->Invoke();
	}

	void SetEnabled(bool enabled)
	{
		SetHighUIColor(enabled ? B_PANEL_TEXT_COLOR : B_CONTROL_TEXT_COLOR, enabled ? B_NO_TINT : B_LIGHTEN_1_TINT);
		Invalidate();
	}

private:
	BCheckBox*	fBox;
};


class RepeatPickerWindow : public BWindow {
public:
	typedef RepeatPickerLabels Labels;

	/**
	 * @param where the left and the bottom of the field that asked for it, in screen coordinates
	 * @param file the file whose attributes are set
	 * @param target gets `done` (if there is one) after the attributes were written
	 */
	RepeatPickerWindow(BPoint where, const entry_ref& file, const BMessenger& target = BMessenger(), const BMessage* done = NULL, const Labels& labels = Labels())
		:
		BWindow(BRect(0, 0, 100, 100), labels.title.c_str(), B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_CLOSE_ON_ESCAPE),
		fFile(file),
		fTarget(target),
		fDone(done != NULL ? *done : BMessage((uint32)0)),
		fHasDone(done != NULL)
	{
		recurrence::Spec spec;
		const bool exact = _Initial(&spec);

		fInterval = new StepField("interval", new BMessage(kChanged));
		fInterval->SetRange(1, 999);
		fInterval->SetValue(spec.interval);
		BPopUpMenu* unitMenu = new BPopUpMenu("unit");
		for (int i = 0; i < 4; i++) {
			BMenuItem* item = new BMenuItem(labels.units[i].c_str(), new BMessage(kChanged));
			if (i == spec.unit)
				item->SetMarked(true);
			unitMenu->AddItem(item);
		}
		fUnit = new BMenuField("unit", "", unitMenu);

		// (the week starts on the day that the language and region of the user say: BDateFormat::GetStartOfWeek(), as the calendars of Haiku do)
		BDateFormat dates;
		BWeekday firstDay = B_WEEKDAY_MONDAY;
		dates.GetStartOfWeek(&firstDay);
		const int start = ((int)firstDay + 6) % 7;		// (0 = Monday ... 6 = Sunday)
		for (int i = 0; i < 7; i++) {
			BString name;
			dates.GetDayName(i + 1, name, B_MEDIUM_DATE_FORMAT);
			fDays[i] = new BCheckBox("day", "", new BMessage(kChanged));
			fDays[i]->SetValue((spec.weekdays & (1 << i)) != 0 ? B_CONTROL_ON : B_CONTROL_OFF);
			fDays[i]->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_TOP));
			fDayNames[i] = new DayLabel(name.String(), fDays[i]);
			fDayNames[i]->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_TOP));
		}
		// (the name of each weekday is under its box, the days close together)
		BGroupView* days = new BGroupView(B_HORIZONTAL, 0);
		for (int k = 0; k < 7; k++) {
			const int i = (start + k) % 7;
			BGroupView* cell = new BGroupView(B_VERTICAL, 0);
			cell->GroupLayout()->AddView(fDays[i]);
			cell->GroupLayout()->AddView(fDayNames[i]);
			days->GroupLayout()->AddView(cell);
		}

		fNever = new BRadioButton("never", labels.never.c_str(), new BMessage(kChanged));
		fCount = new BRadioButton("count", labels.after.c_str(), new BMessage(kChanged));
		fUntil = new BRadioButton("until", labels.on.c_str(), new BMessage(kChanged));
		(spec.end == recurrence::Spec::kCount ? fCount : spec.end == recurrence::Spec::kUntil ? fUntil : fNever)->SetValue(B_CONTROL_ON);
		fCountValue = new StepField("countvalue", new BMessage(kChanged));
		fCountValue->SetRange(1, 9999);
		fCountValue->SetValue(spec.count);
		fUntilValue = new BTextControl("untilvalue", "", spec.until.c_str(), new BMessage(kChanged));
		fUntilValue->SetModificationMessage(new BMessage(kChanged));
		fUntilValue->TextView()->SetExplicitMinSize(BSize(be_plain_font->StringWidth("0000-00-00 0"), B_SIZE_UNSET));
		fUntilValue->SetToolTip(labels.untilHint.c_str());

		fPreview = new BStringView("preview", "");
		fPreview->SetFont(be_bold_font);
		BStringView* note = new BStringView("note", exact ? "" : labels.more.c_str());

		fNoRepeat = new BButton("none", labels.doesNotRepeat.c_str(), new BMessage(kNoRepeat));
		BButton* cancel = new BButton("cancel", labels.cancel.c_str(), new BMessage(B_QUIT_REQUESTED));
		fOk = new BButton("ok", labels.ok.c_str(), new BMessage(kOk));
		fOk->MakeDefault(true);

		BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_WINDOW_INSETS)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(new BStringView("every", labels.every.c_str()))
				.Add(fInterval)
				.Add(fUnit)
				.AddGlue()
			.End()
			.Add(days)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(new BStringView("ends", labels.ends.c_str()))
				.AddGlue()
			.End()
			.Add(fNever)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(fCount)
				.Add(fCountValue)
				.Add(new BStringView("times", labels.times.c_str()))
				.AddGlue()
			.End()
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(fUntil)
				.Add(fUntilValue)
				.AddGlue()
			.End()
			.AddStrut(B_USE_SMALL_SPACING)
			.Add(fPreview)
			.Add(note)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.Add(fNoRepeat)
				.AddGlue()
				.Add(cancel)
				.Add(fOk)
			.End()
		.End();

		_Update();
		MoveTo(where);
		Show();
	}

	virtual void Show()
	{
		// (it opens where it is asked to, and does not go off the screen)
		const BRect screen(BScreen().Frame());
		const BSize size = GetLayout()->PreferredSize();
		BPoint where = Frame().LeftTop();
		if (where.x + size.width > screen.right)
			where.x = screen.right - size.width;
		if (where.y + size.height > screen.bottom)
			where.y = Frame().top - size.height - 30;
		MoveTo(where.x < 0 ? 0 : where.x, where.y < 24 ? 24 : where.y);
		BWindow::Show();
		if (Lock()) {
			fInterval->MakeFocus(true);
			Unlock();
		}
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kChanged:
				_Update();
				break;
			case kOk:
				if (_Write(true))
					PostMessage(B_QUIT_REQUESTED);
				break;
			case kNoRepeat:
				if (_Write(false))
					PostMessage(B_QUIT_REQUESTED);
				break;
			default:
				BWindow::MessageReceived(message);
		}
	}

private:
	enum { kChanged = 'RPch', kOk = 'RPok', kNoRepeat = 'RPno' };

	/** What the window starts with: the rule of the file if the window can say all of it, else the words; false if the rule says more. */
	bool _Initial(recurrence::Spec* spec)
	{
		BNode node(&fFile);
		std::string rule;
		if (node.InitCheck() == B_OK && recurrence::ReadRule(node, &rule) && !rule.empty()) {
			if (recurrence::SpecOf(rule, spec))
				return true;
			return false;
		}
		BString words;
		std::string fromWords;
		if (node.InitCheck() == B_OK && node.ReadAttrString(recurrence::kWords, &words) == B_OK && recurrence::RuleFromWords(words.String(), &fromWords)
				&& recurrence::SpecOf(fromWords, spec))
			return true;
		spec->unit = recurrence::Spec::kWeek;
		return words.IsEmpty();
	}

	recurrence::Spec _Spec() const
	{
		recurrence::Spec spec;
		BMenuItem* marked = fUnit->Menu()->FindMarked();
		spec.unit = marked != NULL ? fUnit->Menu()->IndexOf(marked) : recurrence::Spec::kWeek;
		spec.interval = fInterval->Value();
		if (spec.unit == recurrence::Spec::kWeek) {
			for (int i = 0; i < 7; i++) {
				if (fDays[i]->Value() == B_CONTROL_ON)
					spec.weekdays |= 1 << i;
			}
		}
		if (fCount->Value() == B_CONTROL_ON) {
			spec.end = recurrence::Spec::kCount;
			spec.count = fCountValue->Value();
		} else if (fUntil->Value() == B_CONTROL_ON) {
			spec.end = recurrence::Spec::kUntil;
			spec.until = fUntilValue->Text();
		}
		return spec;
	}

	static bool _IsDay(const std::string& text)
	{
		if (text.size() != 10 || text[4] != '-' || text[7] != '-')
			return false;
		for (size_t i = 0; i < text.size(); i++) {
			if (i != 4 && i != 7 && !isdigit((unsigned char)text[i]))
				return false;
		}
		const int month = atoi(text.substr(5, 2).c_str()), day = atoi(text.substr(8, 2).c_str());
		return month >= 1 && month <= 12 && day >= 1 && day <= 31;
	}

	void _Update()
	{
		const recurrence::Spec spec = _Spec();
		for (int i = 0; i < 7; i++) {
			fDays[i]->SetEnabled(spec.unit == recurrence::Spec::kWeek);
			fDayNames[i]->SetEnabled(spec.unit == recurrence::Spec::kWeek);
		}
		fCountValue->SetEnabled(spec.end == recurrence::Spec::kCount);
		fUntilValue->SetEnabled(spec.end == recurrence::Spec::kUntil);
		const bool valid = spec.end != recurrence::Spec::kUntil || _IsDay(spec.until);
		fOk->SetEnabled(valid);
		fPreview->SetText(valid ? recurrence::WordsOf(spec).c_str() : "");
	}

	/** Writes what the window says (or that the thing does not repeat) into the file. */
	bool _Write(bool repeats)
	{
		BNode node(&fFile);
		if (node.InitCheck() != B_OK)
			return false;
		status_t status;
		if (repeats) {
			const recurrence::Spec spec = _Spec();
			const std::string words = recurrence::WordsOf(spec), rule = recurrence::RuleOf(spec);
			const ssize_t written = node.WriteAttr(recurrence::kWords, B_STRING_TYPE, 0, words.c_str(), words.size() + 1);
			status = written == (ssize_t)words.size() + 1 ? recurrence::StoreRule(node, rule) : B_IO_ERROR;
		} else {
			status = node.RemoveAttr(recurrence::kWords);
			if (status == B_ENTRY_NOT_FOUND)
				status = B_OK;
			if (status == B_OK)
				status = recurrence::StoreRule(node, std::string());
		}
		if (status != B_OK)
			return false;
		if (fHasDone && fTarget.IsValid()) {
			BMessage done(fDone);
			fTarget.SendMessage(&done);
		}
		return true;
	}

	entry_ref		fFile;
	BMessenger		fTarget;
	BMessage		fDone;
	bool			fHasDone;
	StepField*		fInterval;
	BMenuField*		fUnit;
	BCheckBox*		fDays[7];
	DayLabel*		fDayNames[7];
	BRadioButton*	fNever;
	BRadioButton*	fCount;
	BRadioButton*	fUntil;
	StepField*		fCountValue;
	BTextControl*	fUntilValue;
	BStringView*	fPreview;
	BButton*		fNoRepeat;
	BButton*		fOk;
};

}	// namespace sen
