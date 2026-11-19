/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenRecurrence.h: the rule that simple words say (the part without attributes). Plain C++, no Haiku API. */

#include "TestHarness.h"
#include "../src/cpp/include/SenRecurrence.h"

using namespace sen::recurrence;

static std::string R(const char* words)
{
	std::string rule;
	return RuleFromWords(words, &rule) ? rule : std::string("-");
}

TEST(UsualValues)
{
	CHECK(R("every day") == "FREQ=DAILY");
	CHECK(R("every week") == "FREQ=WEEKLY");
	CHECK(R("every month") == "FREQ=MONTHLY");
	CHECK(R("every year") == "FREQ=YEARLY");
	CHECK(R("every weekday") == "FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR");
}

TEST(WithANumber)
{
	CHECK(R("every 2 weeks") == "FREQ=WEEKLY;INTERVAL=2");
	CHECK(R("every 3 days") == "FREQ=DAILY;INTERVAL=3");
	CHECK(R("every 1 month") == "FREQ=MONTHLY");
	CHECK(R("every 10 years") == "FREQ=YEARLY;INTERVAL=10");
}

TEST(CaseAndBlanksDoNotMatter)
{
	CHECK(R("  Every Week ") == "FREQ=WEEKLY");
	CHECK(R("EVERY 2 WEEKS") == "FREQ=WEEKLY;INTERVAL=2");
}

TEST(WordsThatAreNoSimpleRule)
{
	CHECK(R("") == "-");
	CHECK(R("   ") == "-");
	CHECK(R("weekly") == "-");
	CHECK(R("every") == "-");
	CHECK(R("every fortnight") == "-");
	CHECK(R("every 0 weeks") == "-");
	CHECK(R("every week on Monday") == "-");
	CHECK(R("every month, 10 times") == "-");
	CHECK(R("every year until 2030-12-31") == "-");
}

TEST(AttributeName)
{
	CHECK(IsWordsAttribute("schema:repeatFrequency"));
	CHECK(!IsWordsAttribute("SEN:recurrence"));
	CHECK(!IsWordsAttribute(NULL));
}

TEST_MAIN()
