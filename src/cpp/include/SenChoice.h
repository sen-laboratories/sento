/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenChoice.h
 * @brief A window in which the user picks one of several candidates (header only, Haiku only).
 *
 * For plugins that look something up and get more than one answer (a book from OpenLibrary, a paper from arXiv or from ResearchGate): the plugin describes each
 * answer as a Candidate (a title, a line about who and when, a line of details, where it comes from, and a score for how good it is) and ChooseCandidate() shows them in
 * the dialog with the stripe of SenStripeDialog.h, the best first, and waits for the answer. The plugin knows what "good" is (how close the title is to what was asked,
 * how complete and how well known the entry is); the window only sorts and shows it as a bar.
 *
 *     std::vector<sen::Candidate> candidates;
 *     ... one for each answer: title, subtitle, detail, source, score (0 to 1) ...
 *     int32 chosen = sen::ChooseCandidate("Choose a book", "OpenLibrary found these:", candidates);   // the index in `candidates`, -1: the user cancelled
 *
 * It is made for the thread of a plugin (an application that does its work and quits): the caller waits, the window runs in its own thread. Alt+Q and the close button
 * cancel the choice, they do not quit the application.
 */

#include <Button.h>
#include <ControlLook.h>
#include <Font.h>
#include <ListItem.h>
#include <ListView.h>
#include <OS.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include <algorithm>
#include <math.h>
#include <vector>

#include "SenStripeDialog.h"

namespace sen {

/** One answer to choose from. */
struct Candidate {
	Candidate() : score(0) {}

	BString	title;		///< what it is called
	BString	subtitle;	///< who and when ("Andy Hunt, Dave Thomas - 1999")
	BString	detail;		///< a line more ("ISBN 0201616221 - 352 pages - English")
	BString	source;		///< where it comes from ("OpenLibrary", "arXiv"): several sources can be mixed in one list
	float	score;		///< how good it is, from 0 to 1: the list is sorted by it, the best first, and it is shown as a bar
};


/** The height of a row for a font: three lines and the margins. */
inline float
CandidateRowHeight(const BFont* font)
{
	font_height height;
	font->GetHeight(&height);
	return ceilf(height.ascent + height.descent + height.leading) * 3 + be_control_look->DefaultLabelSpacing() * 2;
}


/** A row of the list of candidates: the title in bold, two lines below it, and the source and the score on the right. */
class CandidateItem : public BListItem {
public:
	CandidateItem(const Candidate& candidate)
		:
		BListItem(),
		fCandidate(candidate)
	{
	}

	virtual void
	Update(BView* owner, const BFont* font)
	{
		BListItem::Update(owner, font);
		SetHeight(CandidateRowHeight(font));
	}

	virtual void
	DrawItem(BView* owner, BRect frame, bool complete = false)
	{
		const rgb_color background
			= IsSelected() ? ui_color(B_LIST_SELECTED_BACKGROUND_COLOR) : ui_color(B_LIST_BACKGROUND_COLOR);
		const rgb_color text = IsSelected() ? ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR) : ui_color(B_LIST_ITEM_TEXT_COLOR);
		const rgb_color dimmed = mix_color(text, background, 110);

		owner->SetHighColor(background);
		owner->FillRect(frame);

		BFont plain;
		owner->GetFont(&plain);
		BFont bold(plain);
		bold.SetFace(B_BOLD_FACE);
		font_height height;
		plain.GetHeight(&height);
		const float line = ceilf(height.ascent + height.descent + height.leading);
		const float margin = be_control_look->DefaultLabelSpacing();
		const float barWidth = plain.StringWidth("ResearchGate") + margin;	// (as wide as a long name of a source, which sits above the bar)
		const float textLeft = frame.left + margin * 2;
		const float textRight = frame.right - barWidth - margin * 3;
		float baseline = frame.top + margin + ceilf(height.ascent);

		// the three lines
		const BString* lines[] = {&fCandidate.title, &fCandidate.subtitle, &fCandidate.detail};
		for (int i = 0; i < 3; i++) {
			const BFont& font = i == 0 ? bold : plain;
			BString shown(*lines[i]);
			font.TruncateString(&shown, B_TRUNCATE_END, textRight - textLeft);
			owner->SetFont(&font);
			owner->SetHighColor(i == 2 ? dimmed : text);
			owner->DrawString(shown.String(), BPoint(textLeft, baseline));
			baseline += line;
		}

		// the source above, the score below it as a bar
		const float right = frame.right - margin * 2;
		BString source(fCandidate.source);
		plain.TruncateString(&source, B_TRUNCATE_END, barWidth);
		owner->SetFont(&plain);
		owner->SetHighColor(dimmed);
		owner->DrawString(source.String(), BPoint(right - plain.StringWidth(source.String()), frame.top + margin + ceilf(height.ascent)));

		const float score = std::max(0.0f, std::min(1.0f, fCandidate.score));
		BRect bar(right - barWidth, frame.top + margin + line + 3, right, frame.top + margin + line + 3 + 6);
		owner->SetHighColor(mix_color(text, background, 60));
		owner->StrokeRect(bar);
		BRect filled(bar.left + 1, bar.top + 1, bar.left + 1 + (bar.Width() - 2) * score, bar.bottom - 1);
		owner->SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
		if (score > 0)
			owner->FillRect(filled);

		// a line between the rows
		owner->SetHighColor(tint_color(background, B_DARKEN_1_TINT));
		owner->StrokeLine(BPoint(frame.left, frame.bottom), BPoint(frame.right, frame.bottom));
		owner->SetFont(&plain);
	}

private:
	Candidate	fCandidate;
};


/** The window of ChooseCandidate(). */
class ChoiceDialog : public StripeDialog {
public:
	/**
	 * @param order the indices of the candidates, in the order they are shown
	 * @param chosen where the index of the choice (of the candidates, not of the list) is written, -1 for none, before @p done is released
	 * @param done released once when the user has chosen or cancelled
	 */
	ChoiceDialog(const char* title, const char* prompt, const std::vector<Candidate>& candidates, const std::vector<int32>& order,
			BBitmap* picture, int32* chosen, sem_id done)
		:
		StripeDialog(title, picture, false, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
		fOrder(order),
		fChosen(chosen),
		fDone(done),
		fFinished(false)
	{
		// Alt+Q would quit the application that waits for the answer: here it closes the window, which is a cancel
		RemoveShortcut('Q', B_COMMAND_KEY);
		AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

		BStringView* heading = new BStringView("prompt", prompt);
		BFont bold(be_bold_font);
		heading->SetFont(&bold);

		fList = new BListView("candidates", B_SINGLE_SELECTION_LIST);
		for (int32 index : order)
			fList->AddItem(new CandidateItem(candidates[index]));
		fList->SetInvocationMessage(new BMessage(kChoose));
		fList->SetSelectionMessage(new BMessage(kSelected));

		BScrollView* scroll = new BScrollView("scroll", fList, 0, false, true, B_FANCY_BORDER);
		const float row = CandidateRowHeight(be_plain_font);
		scroll->SetExplicitMinSize(BSize(be_plain_font->StringWidth("M") * 50, row * 3.5f));
		scroll->SetExplicitPreferredSize(BSize(be_plain_font->StringWidth("M") * 60, row * 6.5f));

		BButton* cancel = new BButton("cancel", "Cancel", new BMessage(B_QUIT_REQUESTED));
		fOk = new BButton("ok", "Use this one", new BMessage(kChoose));
		fOk->MakeDefault(true);

		ContentLayout()->AddView(heading);
		ContentLayout()->AddView(scroll);
		BLayoutBuilder::Group<>(ContentLayout()).AddGroup(B_HORIZONTAL).AddGlue().Add(cancel).Add(fOk).End();

		// the best one is chosen unless the user says otherwise: Return takes it
		if (fList->CountItems() > 0)
			fList->Select(0);
		fOk->SetEnabled(fList->CurrentSelection() >= 0);
		fList->MakeFocus(true);
	}

	virtual void
	MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kSelected:
				fOk->SetEnabled(fList->CurrentSelection() >= 0);
				break;
			case kChoose:
			{
				const int32 row = fList->CurrentSelection();
				if (row >= 0 && row < (int32) fOrder.size()) {
					_Finish(fOrder[row]);
					PostMessage(B_QUIT_REQUESTED);
				}
				break;
			}
			default:
				StripeDialog::MessageReceived(message);
		}
	}

	virtual bool
	QuitRequested()
	{
		_Finish(-1);	// (no effect if the user has chosen already)
		return true;
	}

private:
	enum {
		kChoose = 'CHch',
		kSelected = 'CHsl'
	};

	/** Tells the caller, once. */
	void
	_Finish(int32 index)
	{
		if (fFinished)
			return;
		fFinished = true;
		*fChosen = index;
		release_sem(fDone);
	}

	std::vector<int32>	fOrder;
	int32*				fChosen;
	sem_id				fDone;
	bool				fFinished;
	BListView*			fList;
	BButton*			fOk;
};


/**
 * Shows the candidates, the best first, and waits until the user has chosen one or cancelled.
 * @param picture a picture for the stripe (taken over), NULL for none
 * @return the index of the chosen candidate in @p candidates, or -1 if the user cancelled (or the window could not be shown)
 */
inline int32
ChooseCandidate(const char* title, const char* prompt, const std::vector<Candidate>& candidates, BBitmap* picture = NULL)
{
	if (candidates.empty()) {
		delete picture;
		return -1;
	}

	std::vector<int32> order(candidates.size());
	for (size_t i = 0; i < order.size(); i++)
		order[i] = (int32) i;
	std::stable_sort(order.begin(), order.end(), [&candidates](int32 a, int32 b) { return candidates[a].score > candidates[b].score; });

	sem_id done = create_sem(0, "sen choice");
	if (done < B_OK) {
		delete picture;
		return -1;
	}

	int32 chosen = -1;
	ChoiceDialog* dialog = new ChoiceDialog(title, prompt, candidates, order, picture, &chosen, done);
	dialog->ShowCentered();

	// the dialog writes the choice and then releases the semaphore, once
	while (acquire_sem(done) == B_INTERRUPTED) {
	}
	delete_sem(done);
	return chosen;
}

}	// namespace sen
