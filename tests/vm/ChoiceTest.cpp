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
 * The candidates are not in the order of their scores: the best one is the second, and the window shows it first. Haiku only.
 *     g++ -std=c++17 -Wno-multichar -I<headers> -o choice-test ChoiceTest.cpp -lbe
 */
#include <Application.h>
#include <Looper.h>
#include <OS.h>
#include <stdio.h>

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

		int32 chosen = sen::ChooseCandidate("Choose a paper", "Several sources have this paper. Which one do you want?", candidates);
		printf("chosen: %d\n", (int) chosen);
		fflush(stdout);
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
};

int
main()
{
	BApplication app("application/x-vnd.sen-labs.choice-test");
	Chooser* chooser = new Chooser();
	chooser->Run();
	chooser->PostMessage(kChoose);
	app.Run();
	return 0;
}
