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

TEST(PickerSpecToRuleAndWords)
{
	Spec spec;
	spec.unit = Spec::kWeek; spec.interval = 2; spec.weekdays = 0x05;	// Monday, Wednesday
	CHECK(RuleOf(spec) == "FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE");
	CHECK(WordsOf(spec) == "every 2 weeks on Monday and Wednesday");
	spec.weekdays = 0x15; spec.end = Spec::kCount; spec.count = 10;	// Monday, Wednesday, Friday
	CHECK(WordsOf(spec) == "every 2 weeks on Monday, Wednesday and Friday, 10 times");
	CHECK(RuleOf(spec) == "FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE,FR;COUNT=10");
	spec.interval = 1; spec.weekdays = 0x1F; spec.end = Spec::kNever;
	CHECK(WordsOf(spec) == "every weekday");
	spec.weekdays = 0; spec.unit = Spec::kMonth; spec.end = Spec::kUntil; spec.until = "2030-12-31";
	CHECK(RuleOf(spec) == "FREQ=MONTHLY;UNTIL=20301231");
	CHECK(WordsOf(spec) == "every month, until 2030-12-31");
	spec.end = Spec::kCount; spec.count = 1;
	CHECK(WordsOf(spec) == "every month, once");
	spec.unit = Spec::kDay; spec.interval = 3; spec.end = Spec::kNever;
	CHECK(RuleOf(spec) == "FREQ=DAILY;INTERVAL=3" && WordsOf(spec) == "every 3 days");
}

TEST(PickerSpecOfRule)
{
	Spec spec;
	CHECK(SpecOf("FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE;COUNT=10", &spec));
	CHECK(spec.unit == Spec::kWeek && spec.interval == 2 && spec.weekdays == 0x05 && spec.end == Spec::kCount && spec.count == 10);
	CHECK(SpecOf("freq=yearly;until=20301231T000000Z", &spec) && spec.unit == Spec::kYear && spec.end == Spec::kUntil && spec.until == "2030-12-31");
	CHECK(SpecOf("FREQ=DAILY;WKST=MO", &spec) && spec.unit == Spec::kDay && spec.interval == 1);
	CHECK(!SpecOf("FREQ=MONTHLY;BYDAY=2TU", &spec));			// (the second Tuesday: more than the picker says)
	CHECK(!SpecOf("FREQ=YEARLY;BYMONTH=5;BYMONTHDAY=5", &spec));
	CHECK(!SpecOf("FREQ=HOURLY", &spec));
	CHECK(!SpecOf("", &spec));
	// (what the picker makes reads back the same)
	Spec made; made.unit = Spec::kWeek; made.interval = 3; made.weekdays = 0x60; made.end = Spec::kUntil; made.until = "2031-02-03";
	Spec back;
	CHECK(SpecOf(RuleOf(made), &back) && RuleOf(back) == RuleOf(made) && WordsOf(back) == WordsOf(made));
}

TEST(AttributeName)
{
	CHECK(IsWordsAttribute("schema:repeatFrequency"));
	CHECK(!IsWordsAttribute("SEN:recurrence"));
	CHECK(!IsWordsAttribute(NULL));
}

TEST_MAIN()
