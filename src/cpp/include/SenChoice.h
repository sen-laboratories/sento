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

#include <Bitmap.h>
#include <Button.h>
#include <ControlLook.h>
#include <Font.h>
#include <ListItem.h>
#include <ListView.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include <algorithm>
#include <atomic>
#include <functional>
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
	BString	imageUrl;	///< a picture of it (a photo of the person, a cover): not empty gives the row room for one; it is loaded after the window is shown (see ImageLoader)
};

/**
 * Fetches the picture of a candidate, from its `imageUrl`: called in a thread of its own, one candidate after the other, the best first, while the window is already shown (a picture can take
 * a while, and the user can choose before it is there). It returns a bitmap that the window takes over, or NULL if there is none (the row keeps its empty frame).
 */
typedef std::function<BBitmap*(const BString& imageUrl)> ImageLoader;


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
	CandidateItem(const Candidate& candidate, int32 index = -1)
		:
		BListItem(),
		fCandidate(candidate),
		fIndex(index),
		fImage(NULL),
		fCheckable(false),
		fChecked(false),
		fImageSpace(false)
	{
	}

	virtual ~CandidateItem() { delete fImage; }

	/** the index of the candidate in the list that was given to the window (the rows can be moved: it stays with the row) */
	int32 Index() const { return fIndex; }

	/** room for a picture at the left even if this candidate has none, so that the texts of the rows line up */
	void SetImageSpace(bool space) { fImageSpace = space; }

	/** with a check box at the left of the row (to take several of the rows) */
	void SetCheckable(bool checkable) { fCheckable = checkable; }
	bool IsChecked() const { return fChecked; }
	void SetChecked(bool checked) { fChecked = checked; }

	/** where the check box is in a row of this frame, for the drawing and for the click */
	static BRect
	CheckBoxFrame(BRect frame, const BFont& font)
	{
		font_height height;
		font.GetHeight(&height);
		const float side = ceilf(height.ascent + height.descent) + 2;
		const float left = frame.left + be_control_look->DefaultLabelSpacing() * 2;
		const float top = frame.top + floorf((frame.Height() - side) / 2);
		return BRect(left, top, left + side - 1, top + side - 1);
	}

	/** the picture of the candidate, taken over (it is drawn in the room that the row has for it) */
	void
	SetImage(BBitmap* image)
	{
		delete fImage;
		fImage = image;
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
		float textLeft = frame.left + margin * 2;

		// the check box of a list that takes several rows
		if (fCheckable) {
			BRect box = CheckBoxFrame(frame, plain);
			be_control_look->DrawCheckBox(owner, box, frame, background, fChecked ? BControlLook::B_ACTIVATED : 0);
			textLeft = CheckBoxFrame(frame, plain).right + margin * 2;
		}

		// the picture: a square at the left, there from the start (an empty frame) so that the text does not move when the picture comes
		if (!fCandidate.imageUrl.IsEmpty() || fImageSpace) {
			const float side = frame.Height() - margin * 2;
			const BRect box(textLeft, frame.top + margin, textLeft + side - 1, frame.top + margin + side - 1);
			if (fCandidate.imageUrl.IsEmpty()) {
				// (nothing to show for this one)
			} else if (fImage != NULL) {
				owner->SetDrawingMode(B_OP_ALPHA);
				owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
				// (a rectangular picture is cut to the square, not stretched)
				const BRect source = fImage->Bounds();
				const float edge = std::min(source.Width(), source.Height());
				const BRect part(source.left + (source.Width() - edge) / 2, source.top + (source.Height() - edge) / 2,
					source.left + (source.Width() + edge) / 2, source.top + (source.Height() + edge) / 2);
				owner->DrawBitmap(fImage, part, box, B_FILTER_BITMAP_BILINEAR);
				owner->SetDrawingMode(B_OP_COPY);
			} else {
				owner->SetHighColor(mix_color(text, background, 40));
				owner->StrokeRect(box);
			}
			textLeft += side + margin * 2;
		}
		const float textRight = frame.right - barWidth - margin * 3;
		float baseline = frame.top + margin + ceilf(height.ascent);

		// the three lines (the source and the bar sit at the right of the first two: the third has the whole width)
		const BString* lines[] = {&fCandidate.title, &fCandidate.subtitle, &fCandidate.detail};
		for (int i = 0; i < 3; i++) {
			const BFont& font = i == 0 ? bold : plain;
			BString shown(*lines[i]);
			font.TruncateString(&shown, B_TRUNCATE_END, (i < 2 ? textRight : frame.right - margin * 2) - textLeft);
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
	int32		fIndex;
	BBitmap*	fImage;
	bool		fCheckable;
	bool		fChecked;
	bool		fImageSpace;
};


/**
 * The list of the candidates. With @p multiple every row has a check box (a click on it or on the strip at the left, or the space key, toggles it): the rows that are checked are the choice.
 * With @p reorderable the rows can be moved: dragged by the mouse (a line shows where it goes) or with Ctrl or Alt and the arrow keys; the choice is then in the order that the user made.
 */
class CandidateList : public BListView {
public:
	enum {
		kRowDrag = 'CHdr'	///< the message of a row that is dragged: `row`
	};

	CandidateList(const char* name, bool multiple, bool reorderable, BMessage* toggled)
		:
		BListView(name, B_SINGLE_SELECTION_LIST),
		fMultiple(multiple),
		fReorderable(reorderable),
		fToggled(toggled),
		fDragRow(-1),
		fDropRow(-1)
	{
	}

	virtual ~CandidateList() { delete fToggled; }

	virtual void
	MouseDown(BPoint where)
	{
		const int32 index = IndexOf(where);
		if (fMultiple && index >= 0) {
			BFont font;
			GetFont(&font);
			if (where.x <= CandidateItem::CheckBoxFrame(ItemFrame(index), font).right + be_control_look->DefaultLabelSpacing() * 2) {
				Select(index);
				_Toggle(index);
				return;
			}
		}
		BListView::MouseDown(where);
		// (a row that is held and moved is dragged: see MouseMoved)
		fDragRow = fReorderable ? index : -1;
		fDragStart = where;
	}

	virtual void
	MouseUp(BPoint where)
	{
		fDragRow = -1;
		BListView::MouseUp(where);
	}

	virtual void
	MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
	{
		const BPoint moved = where - fDragStart;
		if (fReorderable && dragMessage == NULL && fDragRow >= 0 && moved.x * moved.x + moved.y * moved.y > 25) {
			// (after a few pixels with the button down: the row is picked up)
			BMessage drag(kRowDrag);
			drag.AddInt32("row", fDragRow);
			DragMessage(&drag, ItemFrame(fDragRow), this);
			fDragRow = -1;
			return;
		}
		if (fReorderable && dragMessage != NULL && dragMessage->what == kRowDrag && transit != B_EXITED_VIEW)
			_ShowDrop(_RowAt(where));
		else if (fDropRow >= 0)
			_ShowDrop(-1);
		BListView::MouseMoved(where, transit, dragMessage);
	}

	virtual void
	MessageReceived(BMessage* message)
	{
		if (fReorderable && message->what == kRowDrag && message->WasDropped()) {
			BPoint where = message->DropPoint();
			ConvertFromScreen(&where);
			const int32 from = message->GetInt32("row", -1);
			_ShowDrop(-1);
			MoveRow(from, _RowAt(where));
			return;
		}
		BListView::MessageReceived(message);
	}

	/** moves the selected row up (-1) or down (+1) by one place */
	void
	MoveSelected(int32 direction)
	{
		const int32 row = CurrentSelection();
		if (row >= 0)
			MoveRow(row, direction < 0 ? row - 1 : row + 2);
	}

	/** moves the row @p from to the place before the row @p before (CountItems() for the end): the same row is selected afterwards */
	void
	MoveRow(int32 from, int32 before)
	{
		if (!fReorderable || from < 0 || from >= CountItems() || before < 0)
			return;
		const int32 to = std::min(before > from ? before - 1 : before, CountItems() - 1);
		if (to == from)
			return;
		MoveItem(from, to);
		Select(to);
		ScrollToSelection();
	}

	virtual void
	Draw(BRect updateRect)
	{
		BListView::Draw(updateRect);
		if (fDropRow >= 0 && CountItems() > 0) {
			// (where the row that is dragged goes: a line between the rows)
			const float y = fDropRow < CountItems() ? ItemFrame(fDropRow).top : ItemFrame(CountItems() - 1).bottom;
			SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
			FillRect(BRect(Bounds().left, y - 1, Bounds().right, y + 1));
		}
	}

	virtual void
	KeyDown(const char* bytes, int32 numBytes)
	{
		if (fMultiple && numBytes == 1 && bytes[0] == B_SPACE && CurrentSelection() >= 0) {
			_Toggle(CurrentSelection());
			return;
		}
		// (the modifiers of the key press itself, not of the keyboard now)
		const BMessage* press = Window() != NULL ? Window()->CurrentMessage() : NULL;
		const int32 held = press != NULL ? press->GetInt32("modifiers", 0) : (int32)modifiers();
		if (fReorderable && numBytes == 1 && (bytes[0] == B_UP_ARROW || bytes[0] == B_DOWN_ARROW) && CurrentSelection() >= 0
			&& (held & (B_COMMAND_KEY | B_CONTROL_KEY)) != 0) {
			MoveSelected(bytes[0] == B_UP_ARROW ? -1 : 1);
			return;
		}
		BListView::KeyDown(bytes, numBytes);
	}

	/** the candidates that are checked (the rows, in the order of the list) */
	std::vector<int32>
	CheckedRows() const
	{
		std::vector<int32> rows;
		for (int32 i = 0; i < CountItems(); i++) {
			const CandidateItem* item = dynamic_cast<const CandidateItem*>(ItemAt(i));
			if (item != NULL && item->IsChecked())
				rows.push_back(i);
		}
		return rows;
	}

private:
	/** the place that a point is at: before a row, or the end of the list below the last */
	int32
	_RowAt(BPoint where)
	{
		const int32 row = IndexOf(where);
		if (row < 0)
			return CountItems();
		// (the lower half of a row is the place after it)
		const BRect frame = ItemFrame(row);
		return where.y > frame.top + frame.Height() / 2 ? row + 1 : row;
	}

	void
	_ShowDrop(int32 row)
	{
		if (row == fDropRow)
			return;
		fDropRow = row;
		Invalidate();
	}

	void
	_Toggle(int32 index)
	{
		CandidateItem* item = dynamic_cast<CandidateItem*>(ItemAt(index));
		if (item == NULL)
			return;
		item->SetChecked(!item->IsChecked());
		InvalidateItem(index);
		if (Window() != NULL && fToggled != NULL)
			Window()->PostMessage(fToggled);
	}

	bool		fMultiple;
	bool		fReorderable;
	BMessage*	fToggled;
	int32		fDragRow;
	BPoint		fDragStart;
	int32		fDropRow;
};


/** The window of ChooseCandidate(). */
class ChoiceDialog : public StripeDialog {
public:
	/**
	 * @param order the indices of the candidates, in the order they are first shown (the rows keep their index when the user moves them)
	 * @param chosen where the index of the choice (of the candidates, not of the list) is written, -1 for none, before @p done is released
	 * @param done released once when the user has chosen or cancelled
	 */
	ChoiceDialog(const char* title, const char* prompt, const std::vector<Candidate>& candidates, const std::vector<int32>& order,
			BBitmap* picture, std::vector<int32>* chosen, sem_id done, bool withImages = false, const char* explanation = NULL, bool multiple = false,
			bool reorderable = false)
		:
		StripeDialog(title, picture, false, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
		fChosen(chosen),
		fDone(done),
		fFinished(false),
		fCandidates(candidates)
	{
		// Alt+Q would quit the application that waits for the answer: here it closes the window, which is a cancel
		RemoveShortcut('Q', B_COMMAND_KEY);
		AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

		BStringView* heading = new BStringView("prompt", prompt);
		BFont bold(be_bold_font);
		heading->SetFont(&bold);

		fMultiple = multiple;
		fList = new CandidateList("candidates", multiple, reorderable && multiple, new BMessage(kSelected));
		for (int32 index : order) {
			CandidateItem* item = new CandidateItem(candidates[index], index);
			item->SetCheckable(multiple);
			item->SetImageSpace(withImages);
			fList->AddItem(item);
		}
		// (with check boxes a double click is two clicks: the button takes the choice)
		if (!multiple)
			fList->SetInvocationMessage(new BMessage(kChoose));
		fList->SetSelectionMessage(new BMessage(kSelected));

		BScrollView* scroll = new BScrollView("scroll", fList, 0, false, true, B_FANCY_BORDER);
		float row = CandidateRowHeight(be_plain_font);
		if (withImages)
			row = std::max(row, be_plain_font->Size() * 6.0f);		// (room for a picture of a size that can be recognized)
		scroll->SetExplicitMinSize(BSize(be_plain_font->StringWidth("M") * 50, row * 3.5f));
		scroll->SetExplicitPreferredSize(BSize(be_plain_font->StringWidth("M") * 60, row * 6.5f));

		BButton* cancel = new BButton("cancel", "Cancel", new BMessage(B_QUIT_REQUESTED));
		fOk = new BButton("ok", multiple ? "Use selected" : "Use this one", new BMessage(kChoose));
		fOk->MakeDefault(true);

		ContentLayout()->AddView(heading);
		// (what the heading does not say, in plain text under it)
		if (explanation != NULL && explanation[0] != '\0')
			ContentLayout()->AddView(new BStringView("explanation", explanation));
		ContentLayout()->AddView(scroll);
		BLayoutBuilder::Group<>(ContentLayout()).AddGroup(B_HORIZONTAL).AddGlue().Add(cancel).Add(fOk).End();

		// the best one is chosen unless the user says otherwise: Return takes it
		if (fList->CountItems() > 0) {
			fList->Select(0);
			if (multiple) {
				if (CandidateItem* first = dynamic_cast<CandidateItem*>(fList->ItemAt(0)))
					first->SetChecked(true);
			}
		}
		_UpdateButton();
		fList->MakeFocus(true);
	}

	virtual void
	MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kSelected:
				_UpdateButton();
				break;
			case kImage:
			{
				// a picture came in: it is for the row of the candidate (the loader works in the order of the list)
				const int32 index = message->GetInt32("index", -1);
				void* pointer = NULL;
				BBitmap* image = message->FindPointer("image", &pointer) == B_OK ? (BBitmap*)pointer : NULL;
				bool taken = false;
				for (int32 row = 0; row < fList->CountItems(); row++) {
					CandidateItem* item = dynamic_cast<CandidateItem*>(fList->ItemAt(row));
					if (item != NULL && item->Index() == index) {
						item->SetImage(image);
						fList->InvalidateItem(row);
						taken = true;
						break;
					}
				}
				if (!taken)
					delete image;
				break;
			}
			case kChoose:
			{
				// the rows that are selected (one, or several that are merged), as the indices of the candidates, in the order of the list
				std::vector<int32> rows;
				if (fMultiple)
					rows = fList->CheckedRows();
				else if (fList->CurrentSelection() >= 0)
					rows.push_back(fList->CurrentSelection());
				std::vector<int32> picked;
				for (int32 row : rows) {
					if (const CandidateItem* item = dynamic_cast<const CandidateItem*>(fList->ItemAt(row)))
						picked.push_back(item->Index());
				}
				if (!picked.empty()) {
					_Finish(picked);
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
		_Finish(std::vector<int32>());	// (no effect if the user has chosen already)
		return true;
	}

private:
public:
	enum {
		kImage = 'CHim'		///< from the loader of the pictures: `index` (of the candidate) and `image` (a BBitmap pointer that the window takes over)
	};

private:
	enum {
		kChoose = 'CHch',
		kSelected = 'CHsl'
	};

	/** the button is there when something is chosen: a row that is checked, or the one that is selected */
	void
	_UpdateButton()
	{
		fOk->SetEnabled(fMultiple ? !fList->CheckedRows().empty() : fList->CurrentSelection() >= 0);
	}

	/** Tells the caller, once. */
	void
	_Finish(const std::vector<int32>& indices)
	{
		if (fFinished)
			return;
		fFinished = true;
		*fChosen = indices;
		release_sem(fDone);
	}

	std::vector<int32>*	fChosen;
	sem_id				fDone;
	bool				fFinished;
	std::vector<Candidate>	fCandidates;
	CandidateList*		fList;
	BButton*			fOk;
	bool				fMultiple;
};


/** What the window of ChooseCandidates() can do besides showing one choice. */
struct ChoiceOptions {
	bool			multiple = false;		///< every candidate has a check box (the best one is checked): the checked ones are the choice, for a caller that merges them
	bool			reorderable = false;	///< with @c multiple: the rows can be moved (dragged, or Ctrl or Alt with the arrow keys), and the choice comes in the order of the rows as the user left them
	BBitmap*		picture = NULL;			///< a picture for the stripe (taken over), NULL for none
	ImageLoader		loader;					///< fetches the pictures of the candidates that have an `imageUrl`, after the window is shown
	const char*		explanation = NULL;		///< a line of plain text under the bold prompt, NULL for none
};


/**
 * Shows the candidates, the best first, and waits until the user has chosen or cancelled. With @c multiple several can be chosen (they are merged by the caller: a person that is known on
 * several services, an ORCID and a GitHub account of the same person), and with @c reorderable the user can say in which order (the first wins where they disagree).
 * @return the indices of the chosen candidates in @p candidates, in the order of the rows (the order of the score, or the order that the user made); none if the user cancelled (or the window could not be shown)
 */
inline std::vector<int32>
ChooseCandidates(const char* title, const char* prompt, const std::vector<Candidate>& candidates, const ChoiceOptions& options)
{
	BBitmap* picture = options.picture;
	const bool multiple = options.multiple;
	const bool reorderable = options.reorderable;
	const ImageLoader& loader = options.loader;
	const char* explanation = options.explanation;
	if (candidates.empty()) {
		delete picture;
		return std::vector<int32>();
	}
	std::vector<int32> order(candidates.size());
	for (size_t i = 0; i < order.size(); i++)
		order[i] = (int32) i;
	std::stable_sort(order.begin(), order.end(), [&candidates](int32 a, int32 b) { return candidates[a].score > candidates[b].score; });

	sem_id done = create_sem(0, "sen choice");
	if (done < B_OK) {
		delete picture;
		return std::vector<int32>();
	}

	bool withImages = false;
	if (loader) {
		for (const Candidate& candidate : candidates)
			withImages = withImages || !candidate.imageUrl.IsEmpty();
	}

	std::vector<int32> chosen;
	ChoiceDialog* dialog = new ChoiceDialog(title, prompt, candidates, order, picture, &chosen, done, withImages, explanation, multiple, reorderable);
	dialog->ShowCentered();

	// The pictures come after the window: one thread fetches them, the best candidate first, and posts each to the window, which is not held up by it. The window may be gone by then
	// (the user chose): the picture that nobody takes is deleted here.
	struct Worker {
		std::vector<Candidate>	candidates;
		std::vector<int32>		order;
		ImageLoader				loader;
		BMessenger				window;
		std::atomic<bool>		stop;
		Worker() : stop(false) {}
	};
	Worker* worker = NULL;
	thread_id thread = -1;
	if (withImages) {
		worker = new Worker();
		worker->candidates = candidates;
		worker->order = order;
		worker->loader = loader;
		worker->window = BMessenger(dialog);
		thread = spawn_thread([](void* data) -> int32 {
			Worker* worker = (Worker*)data;
			for (int32 index : worker->order) {
				if (worker->stop.load())
					break;
				const Candidate& candidate = worker->candidates[index];
				if (candidate.imageUrl.IsEmpty())
					continue;
				BBitmap* image = worker->loader(candidate.imageUrl);
				if (image == NULL)
					continue;
				BMessage message(ChoiceDialog::kImage);
				message.AddInt32("index", index);
				message.AddPointer("image", image);
				if (worker->stop.load() || worker->window.SendMessage(&message) != B_OK)
					delete image;
			}
			return 0;
		}, "sen choice images", B_LOW_PRIORITY, worker);
		if (thread >= B_OK)
			resume_thread(thread);
		else
			delete worker;
	}

	// the dialog writes the choice and then releases the semaphore, once
	while (acquire_sem(done) == B_INTERRUPTED) {
	}
	delete_sem(done);

	if (thread >= B_OK) {
		// (the thread ends after the picture that it is fetching; the caller goes on with the same network client afterwards, so it is not left running beside it)
		worker->stop.store(true);
		status_t ignored;
		wait_for_thread(thread, &ignored);
		delete worker;
	}
	return chosen;
}

/** The same with the positional arguments of before: @p multiple only (see ChoiceOptions). */
inline std::vector<int32>
ChooseCandidates(const char* title, const char* prompt, const std::vector<Candidate>& candidates, bool multiple, BBitmap* picture = NULL,
	const ImageLoader& loader = ImageLoader(), const char* explanation = NULL)
{
	ChoiceOptions options;
	options.multiple = multiple;
	options.picture = picture;
	options.loader = loader;
	options.explanation = explanation;
	return ChooseCandidates(title, prompt, candidates, options);
}

/**
 * Shows the candidates, the best first, and waits until the user has chosen one or cancelled.
 * @return the index of the chosen candidate in @p candidates, or -1 if the user cancelled (or the window could not be shown)
 */
inline int32
ChooseCandidate(const char* title, const char* prompt, const std::vector<Candidate>& candidates, BBitmap* picture = NULL, const ImageLoader& loader = ImageLoader(),
	const char* explanation = NULL)
{
	const std::vector<int32> chosen = ChooseCandidates(title, prompt, candidates, false, picture, loader, explanation);
	return chosen.empty() ? -1 : chosen[0];
}

}	// namespace sen
