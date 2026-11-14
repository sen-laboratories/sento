/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenTimeAttributes.h
 * @brief The start and the end of anything that has a time (`schema:startDate`, `schema:endDate`), kept as a time attribute with the text of a fuzzy value beside it (Haiku only, header only).
 *
 * The time attribute (B_TIME_TYPE) holds the earliest moment of a start and the latest moment of an end, so that they sort and are found with a query: the attributes are indexed as 64-bit integers, so a time is written as a `time_t` (8 bytes). A value that is not exact (a day, a
 * month, `1994-05~`, `202X`: see SenEdtf.h) is also kept as text in the attribute beside it (`SEN:startDateEdtf`, `SEN:endDateEdtf`); an exact one has none. Shoji, Akita and the
 * temporal navigator write and read them through this header, so they all do it the same way:
 *
 *     sen::timeattr::Store(node, "schema:startDate", "1994-05~");      // the time, and the text beside it
 *     sen::timeattr::FuzzyText(node, "schema:startDate", seconds, &text);   // what to show instead of the time, if the text beside it still belongs to it
 */

#include <Node.h>
#include <SupportDefs.h>
#include <TypeConstants.h>
#include <fs_attr.h>

#include <string.h>
#include <time.h>

#include <string>

#include "SenEdtf.h"

namespace sen {
namespace timeattr {

struct Companion {
	const char*	attribute;		///< the time
	const char*	edtf;			///< the text of a fuzzy value
	bool		end;			///< the latest moment is kept (an end), not the earliest (a start)
};

inline const Companion*
Find(const char* attribute)
{
	static const Companion companions[] = {
		{"schema:startDate", "SEN:startDateEdtf", false},
		{"schema:endDate", "SEN:endDateEdtf", true},
	};
	for (const Companion& companion : companions) {
		if (strcmp(attribute, companion.attribute) == 0)
			return &companion;
	}
	return NULL;
}

/** the time attribute that a text attribute belongs to, NULL if it is none of them */
inline const char*
PrimaryOf(const char* edtfAttribute)
{
	for (const char* time : {"schema:startDate", "schema:endDate"}) {
		if (strcmp(Find(time)->edtf, edtfAttribute) == 0)
			return time;
	}
	return NULL;
}

/** The offset of the local time zone at a moment, in seconds: what a person means who types a time of day. */
inline long long
LocalOffsetAt(long long utcSeconds)
{
	time_t moment = (time_t)utcSeconds;
	struct tm parts;
	if (localtime_r(&moment, &parts) == NULL)
		return 0;
	// the local clock read as if it were UTC, minus the moment (Haiku has no timegm: the days are counted)
	const long long days = edtf::detail::DaysFromCivil(parts.tm_year + 1900, (unsigned)parts.tm_mon + 1, (unsigned)parts.tm_mday);
	return days * 86400LL + parts.tm_hour * 3600LL + parts.tm_min * 60LL + parts.tm_sec - utcSeconds;
}

/**
 * The text that stands beside the time, if there is one and it still belongs to the time (it gives that very moment: someone who sets the time without it makes it stale, and it is
 * then ignored). @return false if there is none to show.
 */
inline bool
FuzzyText(BNode& node, const char* attribute, long long seconds, std::string* text)
{
	const Companion* companion = Find(attribute);
	if (companion == NULL)
		return false;
	char buffer[256];
	const ssize_t length = node.ReadAttr(companion->edtf, B_STRING_TYPE, 0, buffer, sizeof(buffer) - 1);
	if (length <= 0)
		return false;
	buffer[length] = '\0';
	edtf::Stored stored;
	const bool known = companion->end ? edtf::ForEnd(buffer, &stored, LocalOffsetAt) : edtf::ForStart(buffer, &stored, LocalOffsetAt);
	if (!known || stored.time != seconds)
		return false;
	*text = buffer;
	return true;
}

/**
 * Stores what was typed for a start or an end: the time, and the text of a fuzzy value beside it (an exact value has none, which is taken away). An empty text takes both away.
 * @return B_BAD_VALUE if the text is no time, B_NOT_SUPPORTED if the attribute is none of the start and the end
 */
inline status_t
Store(BNode& node, const char* attribute, const std::string& input)
{
	const Companion* companion = Find(attribute);
	if (companion == NULL)
		return B_NOT_SUPPORTED;

	const size_t first = input.find_first_not_of(" \t"), last = input.find_last_not_of(" \t");
	if (first == std::string::npos) {
		status_t result = node.RemoveAttr(attribute);
		if (result != B_OK && result != B_ENTRY_NOT_FOUND)
			return result;
		result = node.RemoveAttr(companion->edtf);
		return result == B_ENTRY_NOT_FOUND ? B_OK : result;
	}

	edtf::Stored stored;
	if (!(companion->end ? edtf::ForEnd(input, &stored, LocalOffsetAt) : edtf::ForStart(input, &stored, LocalOffsetAt)))
		return B_BAD_VALUE;

	const time_t seconds = (time_t)stored.time;
	ssize_t written = node.WriteAttr(attribute, B_TIME_TYPE, 0, &seconds, sizeof(seconds));
	if (written != (ssize_t)sizeof(seconds))
		return written < 0 ? (status_t)written : B_IO_ERROR;

	if (stored.edtf.empty()) {
		const status_t result = node.RemoveAttr(companion->edtf);
		return result == B_ENTRY_NOT_FOUND ? B_OK : result;
	}
	written = node.WriteAttr(companion->edtf, B_STRING_TYPE, 0, stored.edtf.c_str(), stored.edtf.size() + 1);
	return written == (ssize_t)stored.edtf.size() + 1 ? B_OK : (written < 0 ? (status_t)written : B_IO_ERROR);
}

}	// namespace timeattr
}	// namespace sen
