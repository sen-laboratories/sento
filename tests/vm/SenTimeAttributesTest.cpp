/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 *
 * Haiku only: SenTimeAttributes.h on a real file (types, the text beside a fuzzy time, a stale text, bad input).
 *   g++ -std=c++17 -isystem ~/config/non-packaged/develop/headers -o timetest SenTimeAttributesTest.cpp -lbe && ./timetest <a file on a BFS volume>
 */
// timetest <file>: Store and FuzzyText of SenTimeAttributes.h on a real file
#include <Node.h>
#include <String.h>
#include <fs_attr.h>
#include <stdio.h>
#include <sen/SenTimeAttributes.h>
using namespace sen;
static void Show(BNode& node, const char* label)
{
	const char* names[] = {"schema:startDate", "SEN:startDateEdtf", "schema:endDate", "SEN:endDateEdtf"};
	printf("  %-26s", label);
	for (const char* name : names) {
		attr_info info;
		if (node.GetAttrInfo(name, &info) != B_OK) { printf(" %s=-", name + 7); continue; }
		if (info.type == B_TIME_TYPE) { time_t t = 0; node.ReadAttr(name, B_TIME_TYPE, 0, &t, sizeof(t)); printf(" %s=%lld(%s,%d bytes)", name + 7, (long long)t, edtf::FormatTime(t).c_str(), (int)info.size); }
		else { char b[128] = {0}; node.ReadAttr(name, B_STRING_TYPE, 0, b, sizeof(b) - 1); printf(" %s='%s'", name + 4, b); }
	}
	printf("\n");
}
int main(int, char** argv)
{
	BNode node(argv[1]);
	printf("local offset now: %lld s\n", timeattr::LocalOffsetAt(1794049200));
	status_t r;
	r = timeattr::Store(node, "schema:startDate", "2026-11-14 10:00"); printf("store exact start -> %s\n", strerror(r)); Show(node, "exact start");
	r = timeattr::Store(node, "schema:startDate", "1994-05~"); printf("store fuzzy start -> %s\n", strerror(r)); Show(node, "fuzzy start 1994-05~");
	std::string text; time_t t = 0; node.ReadAttr("schema:startDate", B_TIME_TYPE, 0, &t, sizeof(t));
	printf("FuzzyText consistent: %d '%s' (must be 1 '1994-05~')\n", timeattr::FuzzyText(node, "schema:startDate", t, &text), text.c_str());
	r = timeattr::Store(node, "schema:endDate", "2026-11-14"); printf("store day end -> %s\n", strerror(r)); Show(node, "end 2026-11-14 (a day)");
	t = 12345; node.WriteAttr("schema:startDate", B_TIME_TYPE, 0, &t, sizeof(t));
	printf("FuzzyText after someone else set the time: %d (must be 0)\n", timeattr::FuzzyText(node, "schema:startDate", t, &text));
	r = timeattr::Store(node, "schema:startDate", "tomorrow-ish"); printf("store garbage -> %s (B_BAD_VALUE is '%s')\n", strerror(r), strerror(B_BAD_VALUE)); Show(node, "unchanged by garbage");
	r = timeattr::Store(node, "dc:title", "x"); printf("store other attribute -> %s (B_NOT_SUPPORTED is '%s')\n", strerror(r), strerror(B_NOT_SUPPORTED));
	r = timeattr::Store(node, "schema:startDate", "  "); printf("store empty start -> %s\n", strerror(r)); Show(node, "start removed");
	r = timeattr::Store(node, "schema:endDate", ""); Show(node, "end removed too");
	timeattr::Store(node, "schema:startDate", "1994-05~"); timeattr::Store(node, "schema:endDate", "2026-11-14 18:30");
	BString duration("PT2H"), title("Test event");
	node.WriteAttrString("schema:duration", &duration); node.WriteAttrString("dc:title", &title);
	Show(node, "state shown in the form");
	// EditText and the order of start and end (an Akita edit)
	timeattr::Store(node, "schema:startDate", "2026-11-14 10:00"); timeattr::Store(node, "schema:endDate", "2026-11-14 12:00");
	t = 0; node.ReadAttr("schema:startDate", B_TIME_TYPE, 0, &t, sizeof(t));
	printf("EditText exact: '%s' (must be 2026-11-14 10:00)\n", timeattr::EditText(node, "schema:startDate", t).c_str());
	timeattr::Store(node, "schema:startDate", "1994-05~"); t = 0; node.ReadAttr("schema:startDate", B_TIME_TYPE, 0, &t, sizeof(t));
	printf("EditText fuzzy: '%s' (must be 1994-05~)\n", timeattr::EditText(node, "schema:startDate", t).c_str());
	timeattr::Store(node, "schema:startDate", "2026-11-14 10:00"); timeattr::Store(node, "schema:endDate", "2026-11-14 12:00");
	r = timeattr::StoreKeepingOrder(node, "schema:endDate", "2026-11-14 09:00"); printf("end before start -> %s\n", strerror(r)); Show(node, "end becomes the start (10:00)");
	timeattr::Store(node, "schema:endDate", "2026-11-14 12:00");
	r = timeattr::StoreKeepingOrder(node, "schema:startDate", "2026-11-14 13:00"); printf("start past end -> %s\n", strerror(r)); Show(node, "end moves with it (15:00)");
	r = timeattr::StoreKeepingOrder(node, "schema:startDate", "nonsense"); printf("garbage -> %s (B_BAD_VALUE)\n", strerror(r));
	return 0;
}
