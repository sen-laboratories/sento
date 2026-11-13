/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenEdtf.h: exact and fuzzy times, their earliest and latest moments, and what is stored. Plain C++, no Haiku API. */

#include "TestHarness.h"
#include "../src/cpp/include/SenEdtf.h"

using namespace sen::edtf;

static Value P(const char* text)
{
	Value value;
	Parse(text, &value);
	return value;
}

TEST(ExactInstants)
{
	Value v = P("2026-11-07T11:00:00Z");
	CHECK(v.valid && v.IsExact() && v.precision == Precision::Second);
	CHECK(v.earliest == 1794049200LL && v.latest == 1794049200LL);
	CHECK(P("2026-11-07T11:00").earliest == 1794049200LL && P("2026-11-07T11:00").precision == Precision::Minute);
	CHECK(P("2026-11-07 11:00:00").earliest == 1794049200LL);			// (a space as people type it)
	CHECK(P("2026-11-07T12:00:00+01:00").earliest == 1794049200LL);		// (an offset is taken off)
	CHECK(P("2026-11-07T06:00:00-05:00").earliest == 1794049200LL);
	CHECK(P("2026-11-07T12:00:00+0100").earliest == 1794049200LL);
	CHECK(P("1970-01-01T00:00:00Z").earliest == 0);
}

TEST(PeriodsAreFuzzy)
{
	Value day = P("2026-11-07");
	CHECK(day.valid && !day.IsExact() && day.precision == Precision::Day);
	CHECK(day.earliest == 1794009600LL && day.latest == 1794095999LL);
	CHECK(P("1994").earliest == 757382400LL && P("1994").latest == 788918399LL);
	CHECK(P("1994-05").earliest == 767750400LL && P("1994-05").latest == 770428799LL);
	CHECK(P("2024-02").latest == 1709251199LL);		// (a leap year)
	CHECK(P("2023-02").latest == 1677628799LL);
}

TEST(QualifiersWidenByOneUnit)
{
	CHECK(P("1994-05~").earliest == 765158400LL && P("1994-05~").latest == 773020799LL);	// April to June
	CHECK(P("1994?").earliest == 725846400LL && P("1994?").latest == 820454399LL);			// 1993 to 1995
	CHECK(P("1994%").uncertain && P("1994%").approximate);
	CHECK(P("1994?").uncertain && !P("1994?").approximate && P("1994~").approximate && !P("1994~").uncertain);
	CHECK(P("2026-12-31~").earliest == 1798588800LL && P("2026-12-31~").latest == 1798847999LL);	// (over the end of the year)
	CHECK(!P("2026-11-07T11:00:00Z?").IsExact());
	CHECK(P("2026-11-07T11:00:00Z?").earliest == 1794049199LL && P("2026-11-07T11:00:00Z?").latest == 1794049201LL);
}

TEST(UnknownDigits)
{
	CHECK(P("202X").earliest == 1577836800LL && P("202X").latest == 1893455999LL);
	CHECK(P("19XX").earliest == -2208988800LL && P("19XX").latest == 946684799LL);
	CHECK(P("2020-XX").earliest == 1577836800LL && P("2020-XX").latest == 1609459199LL);
	CHECK(P("2020-XX").unspecified && !P("2020-XX").IsExact());
	Value december = P("2020-12-XX");
	CHECK(december.valid && december.earliest == 1606780800LL && december.latest == 1609459199LL);
	CHECK(P("2020-1X").earliest == 1601510400LL && P("2020-1X").latest == 1609459199LL);	// (October to December)
}

TEST(Invalid)
{
	const char* bad[] = {"", "  ", "x", "2026-13", "2026-00", "2026-02-30", "2023-02-29", "2026-11-07T25:00", "2026-11-07T11:60", "2026-11-07T11", "2026-11-07T11:00:00Q",
		"2026/11/07", "11-07-2026", "202", "20266", "2026-1", "2026-11-0", "20XX-1X-3X-1", "2026-XX-XXT11:00", "2026-11-07T11:00+25:00"};
	for (const char* text : bad)
		CHECK(!P(text).valid);
	CHECK(P("2024-02-29").valid);
}

TEST(WhatIsStored)
{
	Stored stored;
	CHECK(ForStart("2026-11-07T11:00:00Z", &stored) && stored.time == 1794049200LL && stored.edtf.empty());
	CHECK(ForStart("1994-05~", &stored) && stored.time == 765158400LL && stored.edtf == "1994-05~");
	CHECK(ForEnd("1994-05~", &stored) && stored.time == 773020799LL && stored.edtf == "1994-05~");
	CHECK(ForStart("  2026-11-07 ", &stored) && stored.edtf == "2026-11-07" && stored.time == 1794009600LL);		// (the day: its first moment; the text, trimmed)
	CHECK(ForEnd("2026-11-07", &stored) && stored.time == 1794095999LL);							// (the end of a day is its last second)
	CHECK(!ForStart("tomorrow", &stored));
}

TEST(FormatTime)
{
	CHECK_STR(FormatTime(1794049200LL).c_str(), "2026-11-07T11:00:00Z");
	CHECK_STR(FormatTime(0).c_str(), "1970-01-01T00:00:00Z");
	CHECK_STR(FormatTime(-2208988800LL).c_str(), "1900-01-01T00:00:00Z");
	CHECK_STR(FormatTime(1709251199LL).c_str(), "2024-02-29T23:59:59Z");
	// a time that is formatted is read again
	for (long long t : {0LL, 1794049200LL, 951782400LL, 1709251199LL, -86400LL})
		CHECK(P(FormatTime(t).c_str()).earliest == t);
}

TEST_MAIN()
