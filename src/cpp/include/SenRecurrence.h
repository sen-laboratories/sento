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

/**
 * What the picker of a repeat cycle (SenRepeatPicker.h) lets a person say: every N days, weeks, months or years, on which weekdays (for weeks), and how long (always, N times, until a day).
 * More than that (the second Tuesday of the month, 5 May) is written by the calendar extractor from a calendar file and stays as it is.
 */
struct Spec {
	enum Unit { kDay = 0, kWeek, kMonth, kYear };
	enum End { kNever = 0, kCount, kUntil };

	int			unit = kWeek;
	int			interval = 1;
	int			weekdays = 0;		///< for weeks: bit 0 Monday ... bit 6 Sunday; none: the weekday of the start
	int			end = kNever;
	int			count = 1;			///< for kCount
	std::string	until;				///< for kUntil: the last day, YYYY-MM-DD
};

namespace detail {
inline const char* const kUnitNames[] = {"day", "week", "month", "year"};
inline const char* const kUnitFreq[] = {"DAILY", "WEEKLY", "MONTHLY", "YEARLY"};
inline const char* const kWeekdayWords[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
inline const char* const kWeekdayCodes[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
}	// namespace detail

/** The RRULE of a spec. */
inline std::string
RuleOf(const Spec& spec)
{
	std::string rule = std::string("FREQ=") + detail::kUnitFreq[spec.unit < 0 || spec.unit > 3 ? 1 : spec.unit];
	if (spec.interval > 1)
		rule += ";INTERVAL=" + std::to_string(spec.interval);
	if (spec.unit == Spec::kWeek && spec.weekdays != 0) {
		std::string days;
		for (int i = 0; i < 7; i++) {
			if ((spec.weekdays & (1 << i)) != 0)
				days += std::string(days.empty() ? "" : ",") + detail::kWeekdayCodes[i];
		}
		rule += ";BYDAY=" + days;
	}
	if (spec.end == Spec::kCount && spec.count > 0)
		rule += ";COUNT=" + std::to_string(spec.count);
	else if (spec.end == Spec::kUntil && spec.until.size() == 10)
		rule += ";UNTIL=" + spec.until.substr(0, 4) + spec.until.substr(5, 2) + spec.until.substr(8, 2);
	return rule;
}

/** The words of a spec, as the calendar extractor says the same rule (`every 2 weeks on Monday and Wednesday, 10 times`; the usual ones are the values of the list: every day, every weekday...). */
inline std::string
WordsOf(const Spec& spec)
{
	const int unit = spec.unit < 0 || spec.unit > 3 ? 1 : spec.unit;
	if (unit == Spec::kWeek && spec.interval <= 1 && spec.weekdays == 0x1F && spec.end == Spec::kNever)
		return "every weekday";
	std::string text = spec.interval > 1 ? "every " + std::to_string(spec.interval) + " " + detail::kUnitNames[unit] + "s" : std::string("every ") + detail::kUnitNames[unit];
	if (unit == Spec::kWeek && spec.weekdays != 0) {
		std::string days;
		int left = 0;
		for (int i = 0; i < 7; i++)
			left += (spec.weekdays >> i) & 1;
		for (int i = 0; i < 7; i++) {
			if ((spec.weekdays & (1 << i)) == 0)
				continue;
			days += detail::kWeekdayWords[i];
			left--;
			if (left > 1)
				days += ", ";
			else if (left == 1)
				days += " and ";
		}
		text += " on " + days;
	}
	if (spec.end == Spec::kCount && spec.count > 0)
		text += spec.count == 1 ? std::string(", once") : ", " + std::to_string(spec.count) + " times";
	else if (spec.end == Spec::kUntil && spec.until.size() == 10)
		text += ", until " + spec.until;
	return text;
}

/**
 * The spec of an RRULE, if the picker can say all of it (FREQ, INTERVAL, BYDAY with plain weekdays for weeks, COUNT, UNTIL as a day). False for a rule with more in it: the second Tuesday of the month, a
 * day of the month, a month...
 */
inline bool
SpecOf(const std::string& rule, Spec* spec)
{
	Spec found;
	bool hasFrequency = false;
	std::istringstream in(rule);
	std::string part;
	while (std::getline(in, part, ';')) {
		const size_t equals = part.find('=');
		if (equals == std::string::npos)
			return false;
		std::string key, value;
		for (size_t i = 0; i < equals; i++)
			key += (char)toupper((unsigned char)part[i]);
		for (size_t i = equals + 1; i < part.size(); i++)
			value += (char)toupper((unsigned char)part[i]);
		if (key == "FREQ") {
			int unit = -1;
			for (int i = 0; i < 4; i++) {
				if (value == detail::kUnitFreq[i])
					unit = i;
			}
			if (unit < 0)
				return false;
			found.unit = unit;
			hasFrequency = true;
		} else if (key == "INTERVAL") {
			found.interval = atoi(value.c_str());
			if (found.interval < 1)
				return false;
		} else if (key == "BYDAY") {
			std::istringstream days(value);
			std::string day;
			while (std::getline(days, day, ',')) {
				int index = -1;
				for (int i = 0; i < 7; i++) {
					if (day == detail::kWeekdayCodes[i])
						index = i;
				}
				if (index < 0)
					return false;		// (an ordinal such as 2TU: more than the picker says)
				found.weekdays |= 1 << index;
			}
		} else if (key == "COUNT") {
			found.end = Spec::kCount;
			found.count = atoi(value.c_str());
			if (found.count < 1)
				return false;
		} else if (key == "UNTIL") {
			if (value.size() < 8)
				return false;
			found.end = Spec::kUntil;
			found.until = value.substr(0, 4) + "-" + value.substr(4, 2) + "-" + value.substr(6, 2);
		} else if (key != "WKST")
			return false;
	}
	if (!hasFrequency || (found.weekdays != 0 && found.unit != Spec::kWeek))
		return false;
	*spec = found;
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
