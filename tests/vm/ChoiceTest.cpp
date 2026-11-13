/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/**
 * Shows the window of SenChoice.h with a few candidates and prints which one was chosen ("chosen: 2", or "chosen: -1" for a cancel). The window is shown by a thread
 * of its own, so that the application can be scripted while it is up:
 *
 *     hey application/x-vnd.sen-labs.choice-test QUIT Window 0          cancels  (chosen: -1)
 *     hey application/x-vnd.sen-labs.choice-test CHch Window 0           takes the selected row, the best at first  (chosen: 1)
 *
 * The candidates are not in the order of their scores: the best one is the second, and the window shows it first.
 *
 *     choice-test multi         check boxes (several can be taken, the best is checked): prints "chosen: 1 2"
 *     choice-test reorder       the same, and the rows can be moved (Ctrl or Alt and the arrow keys, or dragged): the list is in the order of the rows
 *     choice-test auto          drives the window of "reorder" through its list (the best row moved down twice, the upload checked, the button pressed): the answer is "chosen: 2 1" (the rows as they were left; without the move it would be "1 2")
 *
 * Haiku only.
 *     g++ -std=c++17 -Wno-multichar -I<headers> -o choice-test ChoiceTest.cpp -lbe
 */
#include <Application.h>
#include <Button.h>
#include <Window.h>
#include <Looper.h>
#include <OS.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include <sen/SenChoice.h>

static sen::Candidate
Make(const char* title, const char* subtitle, const char* detail, const char* source, float score)
{
	sen::Candidate candidate;
	candidate.title = title;
	candidate.subtitle = subtitle;
	candidate.detail = detail;
	candidate.source = source;
	candidate.score = score;
	return candidate;
}

static const uint32 kChoose = 'Tchs';

/** the chooser blocks until the user has chosen: it is run by a looper of its own, not by the thread of the application */
class Chooser : public BLooper {
public:
	Chooser(bool multiple, bool reorderable)
		:
		BLooper("chooser"),
		fMultiple(multiple),
		fReorderable(reorderable)
	{
	}

	virtual void
	MessageReceived(BMessage* message)
	{
		if (message->what != kChoose) {
			BLooper::MessageReceived(message);
			return;
		}

		std::vector<sen::Candidate> candidates;
		candidates.push_back(Make("A preprint of the same paper", "Ann Author, Ben Writer \xc2\xb7 2024", "arXiv:2401.00001 \xc2\xb7 12 pages", "arXiv", 0.45f));
		candidates.push_back(Make("The paper, as published", "Ann Author, Ben Writer \xc2\xb7 2025", "DOI 10.1000/xyz123 \xc2\xb7 PDF \xc2\xb7 14 pages", "Unpaywall", 0.92f));
		candidates.push_back(Make("A copy that somebody uploaded", "A. Author", "PDF \xc2\xb7 scanned", "ResearchGate", 0.2f));

		if (!fMultiple) {
			int32 chosen = sen::ChooseCandidate("Choose a paper", "Several sources have this paper. Which one do you want?", candidates);
			printf("chosen: %d\n", (int) chosen);
		} else {
			sen::ChoiceOptions options;
			options.multiple = true;
			options.reorderable = fReorderable;
			options.explanation = "Select matches you want to keep to merge results.";
			const std::vector<int32> chosen = sen::ChooseCandidates("Choose a paper", "Found 3 results", candidates, options);
			printf("chosen:");
			for (int32 index : chosen)
				printf(" %d", (int) index);
			printf("\n");
		}
		fflush(stdout);
		be_app->PostMessage(B_QUIT_REQUESTED);
	}

private:
	bool	fMultiple;
	bool	fReorderable;
};


/** the window is driven through its list, which the keys call (a window does not take key messages that were made up): the best row moves to the bottom, the row that is then second is checked,
 * and the button is pressed; the answer is the checked rows in the order of the rows */
static int32
Drive(void*)
{
	snooze(2500000);
	BWindow* window = NULL;
	sen::CandidateList* list = NULL;
	BButton* ok = NULL;
	if (be_app->Lock()) {
		for (int32 i = 0; i < be_app->CountWindows(); i++) {
			if (strcmp(be_app->WindowAt(i)->Title(), "Choose a paper") == 0)
				window = be_app->WindowAt(i);
		}
		be_app->Unlock();
	}
	if (window != NULL && window->Lock()) {
		list = dynamic_cast<sen::CandidateList*>(window->FindView("candidates"));
		ok = dynamic_cast<BButton*>(window->FindView("ok"));
		window->Unlock();
	}
	if (list == NULL || ok == NULL) {
		printf("FAILED: no window\n");
		fflush(stdout);
		be_app->PostMessage(B_QUIT_REQUESTED);
		return 1;
	}
	// rows: best (checked), preprint, upload
	if (window->Lock()) {
		list->MoveSelected(1);			// preprint, best, upload
		list->MoveSelected(1);			// preprint, upload, best (the best is selected and checked)
		list->Select(1);				// the upload
		const char space = B_SPACE;
		list->KeyDown(&space, 1);		// ... is checked
		window->Unlock();
	}
	snooze(500000);
	if (window->Lock()) {
		ok->Invoke();
		window->Unlock();
	}
	return 0;
}


int
main(int argc, char** argv)
{
	const bool automatic = argc > 1 && strcmp(argv[1], "auto") == 0;
	const bool reorder = automatic || (argc > 1 && strcmp(argv[1], "reorder") == 0);
	const bool multi = reorder || (argc > 1 && strcmp(argv[1], "multi") == 0);
	BApplication app("application/x-vnd.sen-labs.choice-test");
	Chooser* chooser = new Chooser(multi, reorder);
	chooser->Run();
	chooser->PostMessage(kChoose);
	if (automatic)
		resume_thread(spawn_thread(Drive, "driver", B_NORMAL_PRIORITY, NULL));
	app.Run();
	return 0;
}
