/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 *
 * Haiku only: SenRecurrence.h on a real file (the rule follows the words, an empty rule says "does not repeat", free words leave the rule).
 *   g++ -std=c++17 -isystem ~/config/non-packaged/develop/headers -o recurtest SenRecurrenceAttributesTest.cpp -lbe && ./recurtest <a file on a BFS volume>
 */
#include <Node.h>
#include <stdio.h>
#include <sen/SenRecurrence.h>
using namespace sen;
static int failures = 0;
static void Expect(const char* label, bool present, const std::string& wanted, BNode& node)
{
	std::string rule;
	const bool has = recurrence::ReadRule(node, &rule);
	const bool ok = has == present && rule == wanted;
	printf("%s  %-34s present=%d rule='%s'%s\n", ok ? "ok  " : "FAIL", label, has, rule.c_str(), ok ? "" : " (wanted different)");
	if (!ok)
		failures++;
}
int main(int, char** argv)
{
	BNode node(argv[1]);
	node.RemoveAttr(recurrence::kRule);
	Expect("no rule at first", false, "", node);
	recurrence::SyncFromText(node, "every 2 weeks");	Expect("every 2 weeks", true, "FREQ=WEEKLY;INTERVAL=2", node);
	recurrence::SyncFromText(node, "Every Weekday");	Expect("Every Weekday", true, "FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR", node);
	recurrence::SyncFromText(node, "every second Friday after payday");	Expect("free words keep the rule", true, "FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR", node);
	recurrence::SyncFromText(node, "every year");		Expect("every year", true, "FREQ=YEARLY", node);
	recurrence::SyncFromText(node, "");				Expect("empty words: does not repeat", true, "", node);
	recurrence::SyncFromText(node, "   ");			Expect("blank words: does not repeat", true, "", node);
	recurrence::SyncFromText(node, "every month");		Expect("every month again", true, "FREQ=MONTHLY", node);
	node.RemoveAttr(recurrence::kRule);			Expect("removed: no rule (the content decides)", false, "", node);
	printf("%d failure(s)\n", failures);
	return failures != 0;
}
