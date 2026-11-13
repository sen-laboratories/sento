/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenEdtf.h
 * @brief Times as people say them: exact, or fuzzy in the Extended Date/Time Format (EDTF, ISO 8601-2). Plain C++, no Haiku API, header only.
 *
 * What is understood (the subset that people type; levels 0 and 1 of EDTF, without intervals and negative years):
 *
 *     2026-11-07T11:00:00Z   an instant (a time of day: `T` or a space, hh:mm or hh:mm:ss, then `Z` or an offset +hh[:mm] / -hh[:mm]; no zone means UTC)
 *     2026-11-07             a day            1994-05   a month            1994   a year   (a period, not an instant)
 *     1994?  1994-05~  1994%  uncertain, approximate, both: the period WIDENED by one of its units on each side (1994-05~ is April to June 1994)
 *     202X  19XX  2020-XX  2020-12-XX   digits that are not known: the values that fit (202X is 2020 to 2029)
 *
 * A value has an earliest and a latest moment (seconds since 1970-01-01T00:00:00Z, the last second of the period for a period): an instant has the same for both. A value that is an
 * instant, with no qualifier and no unknown digit, is EXACT; every other is fuzzy. The start of an event is stored as its earliest moment, the end as its latest, so that both sort and are found
 * with a query; a fuzzy value is also kept as text (the attributes `SEN:startDateEdtf` and `SEN:endDateEdtf`), and an exact one has none (ForStart(), ForEnd()).
 */

#include <stdio.h>

#include <string>

namespace sen {
namespace edtf {

enum class Precision { Year, Month, Day, Minute, Second };

struct Value {
	bool		valid = false;
	Precision	precision = Precision::Year;
	bool		uncertain = false;		///< `?` or `%`
	bool		approximate = false;	///< `~` or `%`
	bool		unspecified = false;	///< digits that are `X`
	long long	earliest = 0;			///< the first moment that the value can mean, in seconds since 1970-01-01T00:00:00Z
	long long	latest = 0;				///< the last one (the last second of a period)

	/** an instant (a time of day), without a qualifier and without unknown digits: what a time attribute holds exactly */
	bool IsExact() const
	{
		return valid && !uncertain && !approximate && !unspecified && (precision == Precision::Minute || precision == Precision::Second);
	}
};

namespace detail {

inline bool IsLeap(long long year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

inline int DaysInMonth(long long year, int month)
{
	static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	return month == 2 && IsLeap(year) ? 29 : days[month - 1];
}

/** days since 1970-01-01 of a day of the proleptic Gregorian calendar (Howard Hinnant's algorithm) */
inline long long DaysFromCivil(long long y, unsigned m, unsigned d)
{
	y -= m <= 2;
	const long long era = (y >= 0 ? y : y - 399) / 400;
	const unsigned yoe = (unsigned)(y - era * 400);
	const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + (long long)doe - 719468;
}

inline void CivilFromDays(long long z, long long* y, int* m, int* d)
{
	z += 719468;
	const long long era = (z >= 0 ? z : z - 146096) / 146097;
	const unsigned doe = (unsigned)(z - era * 146097);
	const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	const long long year = (long long)yoe + era * 400;
	const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	const unsigned mp = (5 * doy + 2) / 153;
	*d = (int)(doy - (153 * mp + 2) / 5 + 1);
	*m = (int)(mp < 10 ? mp + 3 : mp - 9);
	*y = year + (*m <= 2);
}

/** the values of the range [low, high] that fit a pattern of digits and `X` (as many characters as it has); false if none does */
inline bool Fitting(const std::string& pattern, int low, int high, int* first, int* last)
{
	bool found = false;
	for (int v = low; v <= high; v++) {
		std::string text = std::to_string(v);
		if (text.size() > pattern.size())
			break;
		text.insert(0, pattern.size() - text.size(), '0');
		bool fits = true;
		for (size_t i = 0; i < pattern.size() && fits; i++)
			fits = pattern[i] == 'X' || pattern[i] == text[i];
		if (!fits)
			continue;
		if (!found)
			*first = v;
		*last = v;
		found = true;
	}
	return found;
}

inline bool IsDigitOrX(char c) { return (c >= '0' && c <= '9') || c == 'X'; }

inline bool TwoDigits(const std::string& s, size_t at, int* value)
{
	if (at + 2 > s.size() || s[at] < '0' || s[at] > '9' || s[at + 1] < '0' || s[at + 1] > '9')
		return false;
	*value = (s[at] - '0') * 10 + (s[at + 1] - '0');
	return true;
}

}	// namespace detail


/** Reads a time as it is typed or stored. @return false if the text is none (the value is not valid then). */
inline bool Parse(const std::string& input, Value* result)
{
	using namespace detail;
	Value value;
	*result = value;

	size_t from = input.find_first_not_of(" \t"), to = input.find_last_not_of(" \t");
	if (from == std::string::npos)
		return false;
	std::string s = input.substr(from, to - from + 1);
	while (!s.empty() && (s.back() == '?' || s.back() == '~' || s.back() == '%')) {
		if (s.back() == '?' || s.back() == '%')
			value.uncertain = true;
		if (s.back() == '~' || s.back() == '%')
			value.approximate = true;
		s.pop_back();
	}
	// (a space between the day and the time of day, as people type it)
	if (s.size() > 10 && s[10] == ' ')
		s[10] = 'T';

	const size_t t = s.find('T');
	const std::string dateText = s.substr(0, t), timeText = t == std::string::npos ? std::string() : s.substr(t + 1);

	// the date: YYYY, YYYY-MM or YYYY-MM-DD, digits or X
	if (dateText.size() != 4 && dateText.size() != 7 && dateText.size() != 10)
		return false;
	const std::string yearPattern = dateText.substr(0, 4);
	std::string monthPattern, dayPattern;
	for (size_t i = 0; i < dateText.size(); i++) {
		const bool dash = i == 4 || i == 7;
		if (dash ? dateText[i] != '-' : !IsDigitOrX(dateText[i]))
			return false;
		if (!dash && dateText[i] == 'X')
			value.unspecified = true;
	}
	value.precision = Precision::Year;
	if (dateText.size() >= 7) {
		monthPattern = dateText.substr(5, 2);
		value.precision = Precision::Month;
	}
	if (dateText.size() == 10) {
		dayPattern = dateText.substr(8, 2);
		value.precision = Precision::Day;
	}

	int year0, year1, month0 = 1, month1 = 12, day0 = 1, day1 = 31;
	if (!Fitting(yearPattern, 0, 9999, &year0, &year1))
		return false;
	if (!monthPattern.empty() && !Fitting(monthPattern, 1, 12, &month0, &month1))
		return false;
	if (monthPattern.empty())
		month0 = 1, month1 = 12;
	if (!dayPattern.empty()) {
		if (!Fitting(dayPattern, 1, DaysInMonth(year1, month1), &day0, &day1)) {
			// (a fixed day that the latest month does not have: it must fit the earliest one at least)
			if (!Fitting(dayPattern, 1, DaysInMonth(year0, month0), &day0, &day1))
				return false;
		}
		if (day0 > DaysInMonth(year0, month0))
			return false;
	} else {
		day0 = 1;
		day1 = DaysInMonth(year1, month1);
	}
	if (day1 > DaysInMonth(year1, month1))
		day1 = DaysInMonth(year1, month1);

	long long first = DaysFromCivil(year0, month0, day0) * 86400LL;
	long long last = DaysFromCivil(year1, month1, day1) * 86400LL + 86399;

	if (!timeText.empty()) {
		// a time of day: only after a whole day, without unknown digits
		if (value.precision != Precision::Day || value.unspecified)
			return false;
		int hours, minutes, seconds = 0;
		size_t at = 0;
		if (!TwoDigits(timeText, at, &hours) || hours > 23)
			return false;
		at += 2;
		if (at >= timeText.size() || timeText[at] != ':' || !TwoDigits(timeText, at + 1, &minutes) || minutes > 59)
			return false;
		at += 3;
		value.precision = Precision::Minute;
		if (at < timeText.size() && timeText[at] == ':') {
			if (!TwoDigits(timeText, at + 1, &seconds) || seconds > 59)
				return false;
			at += 3;
			value.precision = Precision::Second;
		}
		long long offset = 0;
		if (at < timeText.size()) {
			if (timeText[at] == 'Z' && at + 1 == timeText.size()) {
				// UTC
			} else if (timeText[at] == '+' || timeText[at] == '-') {
				int offsetHours, offsetMinutes = 0;
				if (!TwoDigits(timeText, at + 1, &offsetHours) || offsetHours > 14)
					return false;
				size_t end = at + 3;
				if (end < timeText.size()) {
					if (timeText[end] == ':')
						end++;
					if (!TwoDigits(timeText, end, &offsetMinutes) || offsetMinutes > 59)
						return false;
					end += 2;
				}
				if (end != timeText.size())
					return false;
				offset = (offsetHours * 3600LL + offsetMinutes * 60LL) * (timeText[at] == '-' ? -1 : 1);
			} else {
				return false;
			}
		}
		first = first + hours * 3600LL + minutes * 60LL + seconds - offset;
		last = first;
	}

	// uncertain or approximate: one unit of the precision more on each side
	if (value.uncertain || value.approximate) {
		switch (value.precision) {
			case Precision::Year:
				first = DaysFromCivil(year0 - 1, 1, 1) * 86400LL;
				last = DaysFromCivil(year1 + 1, 12, 31) * 86400LL + 86399;
				break;
			case Precision::Month:
			{
				long long y = year0;
				int m = month0 - 1;
				if (m < 1)
					m = 12, y--;
				first = DaysFromCivil(y, m, 1) * 86400LL;
				y = year1;
				m = month1 + 1;
				if (m > 12)
					m = 1, y++;
				last = DaysFromCivil(y, m, DaysInMonth(y, m)) * 86400LL + 86399;
				break;
			}
			case Precision::Day:
				first -= 86400;
				last += 86400;
				break;
			case Precision::Minute:
				first -= 60;
				last += 60;
				break;
			case Precision::Second:
				first -= 1;
				last += 1;
				break;
		}
	}

	value.earliest = first;
	value.latest = last;
	value.valid = true;
	*result = value;
	return true;
}


/** An instant as the exact text of a time: `2026-11-07T11:00:00Z`. */
inline std::string FormatTime(long long seconds)
{
	long long days = seconds >= 0 ? seconds / 86400 : -((-seconds + 86399) / 86400);
	long long rest = seconds - days * 86400;
	long long year;
	int month, day;
	detail::CivilFromDays(days, &year, &month, &day);
	char text[48];
	snprintf(text, sizeof(text), "%04lld-%02d-%02dT%02d:%02d:%02dZ", year, month, day, (int)(rest / 3600), (int)(rest % 3600 / 60), (int)(rest % 60));
	return text;
}


/** What is kept for a start or an end: the time attribute, and the text of a fuzzy value (empty for an exact one, which needs none). */
struct Stored {
	long long	time = 0;
	std::string	edtf;
};

/** The start of an event from the text of the user: its earliest moment, and the text if the value is fuzzy. */
inline bool ForStart(const std::string& text, Stored* stored)
{
	Value value;
	if (!Parse(text, &value))
		return false;
	stored->time = value.earliest;
	stored->edtf = value.IsExact() ? std::string() : std::string(text, text.find_first_not_of(" \t"), text.find_last_not_of(" \t") - text.find_first_not_of(" \t") + 1);
	return true;
}

/** The end of an event: its latest moment, and the text if the value is fuzzy. */
inline bool ForEnd(const std::string& text, Stored* stored)
{
	Value value;
	if (!Parse(text, &value))
		return false;
	stored->time = value.latest;
	stored->edtf = value.IsExact() ? std::string() : std::string(text, text.find_first_not_of(" \t"), text.find_last_not_of(" \t") - text.find_first_not_of(" \t") + 1);
	return true;
}

}	// namespace edtf
}	// namespace sen
