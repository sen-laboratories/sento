/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenRecurrence.h
 * @brief How something repeats: the rule for programs (`SEN:recurrence`, hidden) and the words for people (`schema:repeatFrequency`, Repeats), kept in step. Header only; the part with attributes is Haiku only.
 *
 * Like the time and the text of a fuzzy time beside it (SenTimeAttributes.h), a repeat has two attributes:
 *
 * - **`SEN:recurrence`** is the technical value, the RRULE of RFC 5545 (`FREQ=WEEKLY;BYDAY=MO,WE`). It is what programs read and it can say more than the words. It is not shown.
 *   An empty one says that the thing does NOT repeat (set by hand), which a missing one does not say (the content of the file may still have a rule).
 * - **`schema:repeatFrequency`** is the text for people (`every week`), shown and edited in Shoji and in the lists of Akita, with a popup of the usual values.
 *
 * Whoever changes the text (Shoji, Akita) calls SyncFromText() after it, so that the rule follows; whoever works with the rule (the calendar extractor) writes the words with it. The one who reads
 * (SeriesEventOf() of the calendar extractor) trusts the text when it differs from the words of the rule and says something that can be read (the text was changed alone, for example with
 * `addattr`), and the rule when the text is free words that cannot be read. A program that changes one of the two alone should change both, or take the text away.
 *
 *     sen::recurrence::SyncFromText(node, "every 2 weeks");    // SEN:recurrence = FREQ=WEEKLY;INTERVAL=2
 *     sen::recurrence::RuleFromWords("every weekday", &rule);  // "FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR"
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>
#include <string>

#ifdef __HAIKU__
#	include <Node.h>
#	include <TypeConstants.h>
#	include <fs_attr.h>
#endif

namespace sen {
namespace recurrence {

inline constexpr char kRule[] = "SEN:recurrence";			///< the rule (RRULE), technical, not shown
inline constexpr char kWords[] = "schema:repeatFrequency";	///< the words, for people (Repeats)

/** whether this is the attribute of the words */
inline bool
IsWordsAttribute(const char* name)
{
	return name != NULL && strcmp(name, kWords) == 0;
}

/**
 * The rule that simple words say: every day, every weekday, every week, every month, every year, and with a number (every 2 weeks); case and blanks do not matter. False for words with more
 * in them (on Monday, until 2030): they are not a rule that can be worked out from the words.
 * @param rule gets the RRULE (`FREQ=DAILY`, `FREQ=WEEKLY;INTERVAL=2`, `FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR`)
 */
inline bool
RuleFromWords(const std::string& text, std::string* rule)
{
	std::string t;
	for (char ch : text)
		t += (char)tolower((unsigned char)ch);
	while (!t.empty() && isspace((unsigned char)t.back()))
		t.pop_back();
	size_t start = 0;
	while (start < t.size() && isspace((unsigned char)t[start]))
		start++;
	t = t.substr(start);

	if (t == "every weekday") {
		*rule = "FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR";
		return true;
	}
	if (t.compare(0, 6, "every ") != 0)
		return false;
	std::istringstream in(t.substr(6));
	std::string first, unit, rest;
	int interval = 1;
	in >> first;
	if (first.empty())
		return false;
	if (isdigit((unsigned char)first[0])) {
		interval = atoi(first.c_str());
		in >> unit;
	} else
		unit = first;
	in >> rest;
	if (!rest.empty() || interval < 1 || unit.empty())
		return false;
	if (unit.size() > 1 && unit.back() == 's')
		unit.pop_back();
	const char* frequency = NULL;
	if (unit == "day")
		frequency = "DAILY";
	else if (unit == "week")
		frequency = "WEEKLY";
	else if (unit == "month")
		frequency = "MONTHLY";
	else if (unit == "year")
		frequency = "YEARLY";
	else
		return false;
	*rule = std::string("FREQ=") + frequency;
	if (interval > 1)
		*rule += ";INTERVAL=" + std::to_string(interval);
	return true;
}

#ifdef __HAIKU__

/** Writes the technical rule (an empty one says: does not repeat). */
inline status_t
StoreRule(BNode& node, const std::string& rule)
{
	const ssize_t written = node.WriteAttr(kRule, B_STRING_TYPE, 0, rule.c_str(), rule.size() + 1);
	return written == (ssize_t)rule.size() + 1 ? B_OK : (written < 0 ? (status_t)written : B_IO_ERROR);
}

/**
 * The technical rule of a file. @return false if the file has none (the content of the file may still say how it repeats); true if it has one, which is empty when the thing does not repeat.
 */
inline bool
ReadRule(BNode& node, std::string* rule)
{
	attr_info info;
	rule->clear();
	if (node.GetAttrInfo(kRule, &info) != B_OK)
		return false;
	if (info.size > 1) {
		std::string buffer((size_t)info.size, '\0');
		const ssize_t length = node.ReadAttr(kRule, B_STRING_TYPE, 0, &buffer[0], (size_t)info.size);
		if (length > 0)
			*rule = buffer.c_str();
	}
	return true;
}

/**
 * To be called after the words (Repeats) of a file were set: the rule follows them. Empty words say that it does not repeat (an empty rule); simple words (every 2 weeks) are the rule;
 * other words (every second Friday after payday) leave the rule as it is, because they cannot be worked out.
 */
inline status_t
SyncFromText(BNode& node, const char* text)
{
	const std::string words(text != NULL ? text : "");
	if (words.find_first_not_of(" \t\r\n") == std::string::npos)
		return StoreRule(node, std::string());
	std::string rule;
	if (RuleFromWords(words, &rule))
		return StoreRule(node, rule);
	return B_OK;
}

#endif	// __HAIKU__

}	// namespace recurrence
}	// namespace sen
