/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenList.h: the values of a list in one string, with quotes as in CSV. Plain C++, no Haiku API. */

#include "TestHarness.h"
#include "../src/cpp/include/SenList.h"

using namespace sen::list;
using Values = std::vector<std::string>;

static void
CheckValues(const std::string& text, char separator, const Values& expected, int line)
{
	const Values got = Split(text, separator);
	if (got != expected) {
		printf("  FAIL line %d: Split(\"%s\") has %zu values, expected %zu\n", line, text.c_str(), got.size(), expected.size());
		for (size_t i = 0; i < got.size(); i++)
			printf("       got[%zu] = '%s'\n", i, got[i].c_str());
		testing::Failures()++;
	}
}
#define CHECK_VALUES(text, separator, ...) CheckValues(text, separator, Values{__VA_ARGS__}, __LINE__)

TEST(PlainValues)
{
	CHECK_VALUES("Andy Hunt;Dave Thomas", ';', "Andy Hunt", "Dave Thomas");
	CHECK_VALUES("a,b,c", ',', "a", "b", "c");
	CHECK_VALUES("single", ';', "single");
	CHECK_VALUES("", ';');
	CHECK_VALUES(";", ';');
	CHECK_VALUES("   ", ';');
}

TEST(BlanksAroundAValueAreNotPartOfIt)
{
	CHECK_VALUES("a ; b ;c", ';', "a", "b", "c");
	CHECK_VALUES("  Dave Thomas  ", ';', "Dave Thomas");
	CHECK_VALUES("a\t;\tb", ';', "a", "b");
}

TEST(EmptyValuesAreNoValues)
{
	CHECK_VALUES("a;;b", ';', "a", "b");
	CHECK_VALUES(";a;", ';', "a");
	CHECK_VALUES("a; ;b", ';', "a", "b");
	CHECK_VALUES("\"\";a", ';', "a");
}

TEST(QuotesTakeTheWholeValue)
{
	CHECK_VALUES("\"Hunt, Andy\",\"Thomas, Dave\"", ',', "Hunt, Andy", "Thomas, Dave");
	CHECK_VALUES("'Hunt; Andy';Dave", ';', "Hunt; Andy", "Dave");
	CHECK_VALUES("\"  padded  \";x", ';', "  padded  ", "x");
	CHECK_VALUES("  \"after blanks\" ; next", ';', "after blanks", "next");
}

TEST(TheOtherQuoteIsAnOrdinaryCharacter)
{
	CHECK_VALUES("\"it's\";b", ';', "it's", "b");
	CHECK_VALUES("'say \"hi\"';b", ';', "say \"hi\"", "b");
	// quotes inside a value that does not start with one are what they are
	CHECK_VALUES("O'Brien;5\" pipe", ';', "O'Brien", "5\" pipe");
}

TEST(BackslashEscapesLikeInCode)
{
	CHECK_VALUES("\"a \\\"quoted\\\" word\";b", ';', "a \"quoted\" word", "b");
	CHECK_VALUES("'it\\'s';b", ';', "it's", "b");
	CHECK_VALUES("\"back\\\\slash\";b", ';', "back\\slash", "b");
	// a backslash before anything else is a backslash
	CHECK_VALUES("\"C:\\temp\";b", ';', "C:\\temp", "b");
	// and outside of quotes it is always one
	CHECK_VALUES("C:\\temp;b", ';', "C:\\temp", "b");
}

TEST(LenientAboutWhatPeopleType)
{
	CHECK_VALUES("\"unterminated, value", ',', "unterminated, value");
	CHECK_VALUES("\"abc\" def;x", ';', "abcdef", "x");
	CHECK_VALUES("\"abc\"   ;x", ';', "abc", "x");
}

TEST(ABlankCanBeTheSeparator)
{
	CHECK_VALUES("one two  three", ' ', "one", "two", "three");
	CHECK_VALUES("one \"two words\" three", ' ', "one", "two words", "three");
}

TEST(QuoteOnlyWhenNeeded)
{
	CHECK_STR(Quote("Andy Hunt", ';'), "Andy Hunt");
	CHECK_STR(Quote("O'Brien", ';'), "O'Brien");
	CHECK_STR(Quote("Hunt; Andy", ';'), "\"Hunt; Andy\"");
	CHECK_STR(Quote("Hunt, Andy", ';'), "Hunt, Andy");		// (a comma is no separator of this list)
	CHECK_STR(Quote("Hunt, Andy", ','), "\"Hunt, Andy\"");
	CHECK_STR(Quote(" padded", ';'), "\" padded\"");
	CHECK_STR(Quote("padded ", ';'), "\"padded \"");
	CHECK_STR(Quote("'Twas", ';'), "\"'Twas\"");			// (it would be read as a quote)
	CHECK_STR(Quote("\"quoted\" start", ';'), "'\"quoted\" start'");
	CHECK_STR(Quote("both \" and ' ;", ';'), "\"both \\\" and ' ;\"");
	CHECK_STR(Quote("a\\b;c", ';'), "\"a\\\\b;c\"");		// (a backslash in quotes is escaped)
	CHECK_STR(Quote("two\nlines", ';'), "\"two\nlines\"");
	CHECK_STR(Quote("", ';'), "");
}

TEST(JoinWritesWhatSplitReads)
{
	CHECK_STR(Join({"Andy Hunt", "Dave Thomas"}, ';'), "Andy Hunt;Dave Thomas");
	CHECK_STR(Join({}, ';'), "");
	CHECK_STR(Join({"a", "", "b"}, ';'), "a;b");		// (empty values are left out)
	CHECK_STR(Join({"Hunt; Andy", "Dave"}, ';'), "\"Hunt; Andy\";Dave");
}

TEST(RoundTrip)
{
	const Values nasty = {
		"plain", "with, comma", "with; semicolon", "O'Brien", "'Twas", "\"quoted\"", "5\" pipe", "both \" and '", " leading", "trailing ",
		"back\\slash", "ends with backslash\\", "\\", "\"", "'", "line\nbreak", "tab\tinside", "\xc3\xa9\xc3\xa8 unicode \xe6\xa1\x90", "a b c",
	};
	for (char separator : {';', ',', '|', ' ', '/'}) {
		// every value alone, and all of them in one list
		for (const std::string& value : nasty) {
			const Values got = Split(Join({value}, separator), separator);
			if (got != Values{value}) {
				printf("  FAIL round trip of '%s' with '%c': got %zu values\n", value.c_str(), separator, got.size());
				testing::Failures()++;
			}
		}
		const Values got = Split(Join(nasty, separator), separator);
		if (got != nasty) {
			printf("  FAIL round trip of the whole list with '%c': %zu of %zu values\n", separator, got.size(), nasty.size());
			testing::Failures()++;
		}
	}
}

TEST(Separators)
{
	CHECK(IsValidSeparator(';'));
	CHECK(IsValidSeparator(','));
	CHECK(IsValidSeparator('|'));
	CHECK(IsValidSeparator(' '));
	CHECK(!IsValidSeparator('"'));
	CHECK(!IsValidSeparator('\''));
	CHECK(!IsValidSeparator('\\'));
	CHECK(!IsValidSeparator('\n'));
	CHECK(!IsValidSeparator('\0'));
}

TEST_MAIN()
