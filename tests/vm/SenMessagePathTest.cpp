/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/**
 * Tests of SenMessagePath.h: reading and changing the fields of a nested BMessage by path keys, as text. Haiku only (BMessage):
 *     g++ -std=c++20 -Wall -Wextra -Wno-multichar -o message-path-test SenMessagePathTest.cpp -lbe && ./message-path-test
 */

#include <algorithm>

#include "../TestHarness.h"
#include "../../src/cpp/include/SenMessagePath.h"

using namespace sen::settings;

/** A settings message with every kind of field that the form meets. */
static BMessage
MakeSettings()
{
	BMessage items1, items2, deeper, nested, settings;
	items1.AddString("name", "one");
	items1.AddInt32("n", 1);
	items2.AddString("name", "two");
	items2.AddInt32("n", 2);
	deeper.AddString("deepest", "z");
	nested.AddInt32("depth", 2);
	nested.AddString("label", "deep");
	nested.AddMessage("items", &items1);
	nested.AddMessage("items", &items2);
	nested.AddMessage("deeper", &deeper);

	settings.AddString("email", "a@b.c");
	settings.AddBool("unpaywall", true);
	settings.AddInt32("max_authors", 10);
	settings.AddDouble("ratio", 0.5);
	settings.AddFloat("scale", 1.25f);
	settings.AddInt8("small", 5);
	settings.AddUInt8("flags", 200);
	settings.AddInt64("big", 5000000000LL);
	settings.AddString("tags", "x");
	settings.AddString("tags", "y");
	settings.AddString("tags", "w");
	settings.AddMessage("nested", &nested);
	settings.AddString("a.b", "dot");
	settings.AddInt32("max_02", 77);
	settings.AddData("blob", B_RAW_TYPE, "12345", 5);
	return settings;
}

static std::string
Text(const BMessage& message, const std::string& path)
{
	std::string text = "<none>";
	GetText(message, path, &text);
	return text;
}

static size_t
CountOf(const BMessage& message, const std::string& path)
{
	type_code type;
	size_t count = 0;
	FieldOf(message, path, &type, &count);
	return count;
}

TEST(AValueIsReadAsTextInEveryType)
{
	const BMessage s = MakeSettings();
	CHECK_STR(Text(s, "email"), "a@b.c");
	CHECK_STR(Text(s, "unpaywall"), "true");
	CHECK_STR(Text(s, "max_authors"), "10");
	CHECK_STR(Text(s, "ratio"), "0.5");
	CHECK_STR(Text(s, "scale"), "1.25");
	CHECK_STR(Text(s, "small"), "5");
	CHECK_STR(Text(s, "flags"), "200");
	CHECK_STR(Text(s, "big"), "5000000000");
}

TEST(ArraysAndNestedMessagesAreAddressedByThePath)
{
	const BMessage s = MakeSettings();
	CHECK_STR(Text(s, "tags_00"), "x");
	CHECK_STR(Text(s, "tags_01"), "y");
	CHECK_STR(Text(s, "tags_02"), "w");
	CHECK_STR(Text(s, "tags"), "x");						// no index: the first value
	CHECK_STR(Text(s, "nested.depth"), "2");
	CHECK_STR(Text(s, "nested.items_00.name"), "one");
	CHECK_STR(Text(s, "nested.items_01.name"), "two");
	CHECK_STR(Text(s, "nested.items_01.n"), "2");
	CHECK_STR(Text(s, "nested.deeper.deepest"), "z");
}

TEST(ANameWithADotIsEscapedAndAnIndexLikeNameIsFoundFirst)
{
	const BMessage s = MakeSettings();
	CHECK_STR(Text(s, "a\\.b"), "dot");
	CHECK_STR(Text(s, "max_02"), "77");						// the field with exactly this name, not the third value of "max"
}

TEST(WhatCannotBeReadIsSaidSo)
{
	const BMessage s = MakeSettings();
	std::string text;
	CHECK_EQ(GetText(s, "tags_03", &text), (status_t)B_BAD_INDEX);
	CHECK_EQ(GetText(s, "nothing", &text), (status_t)B_NAME_NOT_FOUND);
	CHECK_EQ(GetText(s, "nested.nothing", &text), (status_t)B_NAME_NOT_FOUND);
	CHECK_EQ(GetText(s, "email.x", &text), (status_t)B_BAD_TYPE);			// a string has nothing below it
	CHECK_EQ(GetText(s, "blob", &text), (status_t)B_BAD_TYPE);			// no text stands for raw data
	CHECK_EQ(GetText(s, "nested", &text), (status_t)B_BAD_TYPE);
	CHECK_EQ(GetText(s, "nested.items_02.name", &text), (status_t)B_BAD_INDEX);
	CHECK_EQ(GetText(s, "", &text), (status_t)B_BAD_VALUE);
	CHECK_EQ(GetText(s, "nested..depth", &text), (status_t)B_BAD_VALUE);
}

TEST(HasAndFieldOf)
{
	const BMessage s = MakeSettings();
	CHECK(Has(s, "email"));
	CHECK(Has(s, "nested.items_01.name"));
	CHECK(!Has(s, "tags_03"));
	CHECK(!Has(s, "nothing"));
	CHECK(!Has(s, "nested.items_05.name"));

	type_code type = 0;
	size_t count = 0;
	CHECK_EQ(FieldOf(s, "tags", &type, &count), (status_t)B_OK);
	CHECK(type == B_STRING_TYPE);
	CHECK_EQ(count, (size_t)3);
	CHECK_EQ(FieldOf(s, "nested.items", &type, &count), (status_t)B_OK);
	CHECK(type == B_MESSAGE_TYPE);
	CHECK_EQ(count, (size_t)2);
	CHECK_EQ(FieldOf(s, "nothing", &type, &count), (status_t)B_NAME_NOT_FOUND);
}

TEST(AValueIsSetFromTextAndKeepsItsType)
{
	BMessage s = MakeSettings();
	CHECK_EQ(SetText(&s, "unpaywall", "off"), (status_t)B_OK);
	CHECK_STR(Text(s, "unpaywall"), "false");
	CHECK_EQ(SetText(&s, "unpaywall", "Yes"), (status_t)B_OK);
	CHECK_STR(Text(s, "unpaywall"), "true");
	CHECK_EQ(SetText(&s, "max_authors", "11"), (status_t)B_OK);
	CHECK_EQ(s.GetInt32("max_authors", 0), 11);
	CHECK_EQ(SetText(&s, "max_authors", " -4 "), (status_t)B_OK);
	CHECK_EQ(s.GetInt32("max_authors", 0), -4);
	CHECK_EQ(SetText(&s, "ratio", "1e3"), (status_t)B_OK);
	CHECK_STR(Text(s, "ratio"), "1000");
	CHECK_EQ(SetText(&s, "email", "ü ö@example.org"), (status_t)B_OK);
	CHECK_STR(Text(s, "email"), "ü ö@example.org");
	CHECK_EQ(SetText(&s, "big", "9223372036854775807"), (status_t)B_OK);
	CHECK_STR(Text(s, "big"), "9223372036854775807");
	CHECK_EQ(SetText(&s, "email", ""), (status_t)B_OK);			// an empty text is a value of a string
	CHECK_STR(Text(s, "email"), "");
}

TEST(ATextThatDoesNotFitTheTypeIsRefusedAndNothingChanges)
{
	BMessage s = MakeSettings();
	CHECK_EQ(SetText(&s, "unpaywall", "maybe"), (status_t)B_BAD_VALUE);
	CHECK_STR(Text(s, "unpaywall"), "true");
	CHECK_EQ(SetText(&s, "max_authors", "11x"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "max_authors", "7.5"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "max_authors", ""), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "max_authors", "99999999999"), (status_t)B_BAD_VALUE);		// out of range for 32 bits
	CHECK_EQ(SetText(&s, "max_authors", "+5"), (status_t)B_BAD_VALUE);
	CHECK_STR(Text(s, "max_authors"), "10");
	CHECK_EQ(SetText(&s, "small", "200"), (status_t)B_BAD_VALUE);					// int8
	CHECK_EQ(SetText(&s, "small", "-129"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "small", "-128"), (status_t)B_OK);
	CHECK_EQ(SetText(&s, "flags", "-1"), (status_t)B_BAD_VALUE);					// uint8
	CHECK_EQ(SetText(&s, "flags", "256"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "flags", "255"), (status_t)B_OK);
	CHECK_EQ(SetText(&s, "ratio", "abc"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "ratio", "1.5 kg"), (status_t)B_BAD_VALUE);
	CHECK_STR(Text(s, "ratio"), "0.5");
	CHECK_EQ(SetText(&s, "blob", "x"), (status_t)B_BAD_TYPE);
	CHECK_EQ(SetText(&s, "nested", "x"), (status_t)B_BAD_TYPE);
}

TEST(NothingIsMadeBySettingAndTheIndexIsChecked)
{
	BMessage s = MakeSettings();
	CHECK_EQ(SetText(&s, "nothing", "x"), (status_t)B_NAME_NOT_FOUND);
	CHECK(!Has(s, "nothing"));
	CHECK_EQ(SetText(&s, "nested.nothing", "x"), (status_t)B_NAME_NOT_FOUND);
	CHECK_EQ(SetText(&s, "tags_05", "x"), (status_t)B_BAD_INDEX);		// beyond the end, not the next one
	CHECK_EQ(CountOf(s, "tags"), (size_t)3);
	CHECK_EQ(SetText(&s, "", "x"), (status_t)B_BAD_VALUE);
}

TEST(AValueOfAnArrayAndOfANestedMessageIsSet)
{
	BMessage s = MakeSettings();
	CHECK_EQ(SetText(&s, "tags_01", "changed"), (status_t)B_OK);
	CHECK_STR(Text(s, "tags_00"), "x");
	CHECK_STR(Text(s, "tags_01"), "changed");
	CHECK_STR(Text(s, "tags_02"), "w");
	CHECK_EQ(SetText(&s, "nested.items_00.name", "uno"), (status_t)B_OK);
	CHECK_STR(Text(s, "nested.items_00.name"), "uno");
	CHECK_STR(Text(s, "nested.items_01.name"), "two");		// the other message of the array is as it was
	CHECK_STR(Text(s, "nested.items_00.n"), "1");
	CHECK_EQ(SetText(&s, "nested.deeper.deepest", "zz"), (status_t)B_OK);
	CHECK_STR(Text(s, "nested.deeper.deepest"), "zz");
	CHECK_STR(Text(s, "nested.label"), "deep");
	CHECK_EQ(SetText(&s, "a\\.b", "dotted"), (status_t)B_OK);
	CHECK_STR(Text(s, "a\\.b"), "dotted");
	CHECK_EQ(SetText(&s, "max_02", "78"), (status_t)B_OK);		// the field of that name, not "max"
	CHECK_STR(Text(s, "max_02"), "78");
}

TEST(AValueAfterTheLastOneIsAppended)
{
	BMessage s = MakeSettings();
	CHECK_EQ(SetText(&s, "tags_03", "fourth"), (status_t)B_OK);
	CHECK_EQ(CountOf(s, "tags"), (size_t)4);
	CHECK_STR(Text(s, "tags_03"), "fourth");

	CHECK_EQ(Append(&s, "tags", "fifth"), (status_t)B_OK);
	CHECK_STR(Text(s, "tags_04"), "fifth");
	CHECK_EQ(Append(&s, "nested.deeper.deepest", "second"), (status_t)B_OK);		// a field of one value becomes an array
	CHECK_STR(Text(s, "nested.deeper.deepest_01"), "second");
	CHECK_EQ(Append(&s, "max_authors", "12"), (status_t)B_OK);
	CHECK_STR(Text(s, "max_authors_01"), "12");
	CHECK_EQ(Append(&s, "max_authors", "x"), (status_t)B_BAD_VALUE);
	CHECK_EQ(Append(&s, "nested.items", "x"), (status_t)B_BAD_TYPE);
	CHECK_EQ(Append(&s, "nothing", "x"), (status_t)B_NAME_NOT_FOUND);
}

TEST(AValueIsRemoved)
{
	BMessage s = MakeSettings();
	CHECK_EQ(Remove(&s, "tags_01"), (status_t)B_OK);
	CHECK_EQ(CountOf(s, "tags"), (size_t)2);
	CHECK_STR(Text(s, "tags_00"), "x");
	CHECK_STR(Text(s, "tags_01"), "w");						// the later ones move up
	CHECK_EQ(Remove(&s, "tags_07"), (status_t)B_BAD_INDEX);
	CHECK_EQ(Remove(&s, "nested.deeper.deepest"), (status_t)B_OK);
	CHECK(!Has(s, "nested.deeper.deepest"));						// the last value takes its field with it
	CHECK_EQ(Remove(&s, "nested.items_00"), (status_t)B_OK);		// a message of an array
	CHECK_STR(Text(s, "nested.items_00.name"), "two");
	CHECK_EQ(Remove(&s, "nothing"), (status_t)B_NAME_NOT_FOUND);
}

TEST(AChangeThatFailsLeavesTheMessageAsItWas)
{
	BMessage s = MakeSettings();
	CHECK_EQ(SetText(&s, "nested.items_01.n", "x"), (status_t)B_BAD_VALUE);
	CHECK_STR(Text(s, "nested.items_01.n"), "2");
	CHECK_EQ(SetText(&s, "nested.items_01.nothing", "x"), (status_t)B_NAME_NOT_FOUND);
	CHECK_STR(Text(s, "nested.items_01.name"), "two");
	std::vector<Leaf> before, after;
	Enumerate(MakeSettings(), &before);
	Enumerate(s, &after);
	CHECK_EQ(before.size(), after.size());
}

TEST(EnumerateListsEveryValueWithItsPath)
{
	BMessage s;
	BMessage nested;
	nested.AddInt32("depth", 3);
	s.AddString("a", "1");
	s.AddString("tags", "p");
	s.AddString("tags", "q");
	s.AddMessage("nested", &nested);
	s.AddBool("on.off", true);

	std::vector<Leaf> leaves;
	Enumerate(s, &leaves);
	std::vector<std::string> paths;
	for (const Leaf& leaf : leaves)
		paths.push_back(leaf.path);
	const std::vector<std::string> expected = {"a", "tags_00", "tags_01", "nested", "nested.depth", "on\\.off"};
	CHECK(paths == expected);

	CHECK(leaves[0].type == B_STRING_TYPE);
	CHECK_EQ(leaves[1].count, (size_t)2);
	CHECK_EQ(leaves[2].index, (size_t)1);
	CHECK(leaves[3].type == B_MESSAGE_TYPE);
	CHECK_EQ(leaves[3].depth, 0);
	CHECK_EQ(leaves[4].depth, 1);
	CHECK_STR(leaves[4].name, "depth");

	// every path that was listed reads back
	for (const Leaf& leaf : leaves) {
		std::string text;
		if (leaf.type != B_MESSAGE_TYPE)
			CHECK_EQ(GetText(s, leaf.path, &text), (status_t)B_OK);
	}
}

TEST(EnumerateGoesIntoEveryMessageOfAnArray)
{
	const BMessage s = MakeSettings();
	std::vector<Leaf> leaves;
	Enumerate(s, &leaves);
	std::vector<std::string> paths;
	for (const Leaf& leaf : leaves)
		paths.push_back(leaf.path);
	auto has = [&](const char* path) { return std::find(paths.begin(), paths.end(), path) != paths.end(); };
	CHECK(has("nested.items_00"));
	CHECK(has("nested.items_00.name"));
	CHECK(has("nested.items_01.n"));
	CHECK(has("nested.deeper.deepest"));
	CHECK(has("max_02"));
	CHECK(has("a\\.b"));
	CHECK(has("blob"));			// listed: the form shows it read-only
	for (const Leaf& leaf : leaves) {
		std::string text;
		if (leaf.addressable && leaf.type != B_MESSAGE_TYPE && leaf.type != B_RAW_TYPE)
			CHECK_EQ(GetText(s, leaf.path, &text), (status_t)B_OK);
	}
}

TEST(AValueThatCannotBeNamedIsListedWithoutAPath)
{
	// "tags_02" is a field of its own: the third value of "tags" has no path
	BMessage s;
	s.AddString("tags", "p");
	s.AddString("tags", "q");
	s.AddString("tags", "r");
	s.AddString("tags_02", "own");
	std::vector<Leaf> leaves;
	Enumerate(s, &leaves);
	CHECK_EQ(leaves.size(), (size_t)4);
	int unaddressable = 0;
	for (const Leaf& leaf : leaves) {
		if (!leaf.addressable) {
			unaddressable++;
			CHECK(leaf.path.empty());
			CHECK_STR(leaf.name, "tags");
			CHECK_EQ(leaf.index, (size_t)2);
		}
	}
	CHECK_EQ(unaddressable, 1);
	CHECK_STR(Text(s, "tags_02"), "own");
}

TEST(SetValuesReplacesAppendsAndRemovesInPlace)
{
	BMessage s = MakeSettings();
	// as many: replaced in place; more: appended; fewer: the surplus goes
	CHECK_EQ(SetValues(&s, "", "tags", B_STRING_TYPE, {"a", "b", "c"}), (status_t)B_OK);
	CHECK_STR(Text(s, "tags_00"), "a");
	CHECK_STR(Text(s, "tags_02"), "c");
	CHECK_EQ(SetValues(&s, "", "tags", B_STRING_TYPE, {"a", "b", "c", "d"}), (status_t)B_OK);
	CHECK_STR(Text(s, "tags_03"), "d");
	CHECK_EQ(SetValues(&s, "", "tags", B_STRING_TYPE, {"z"}), (status_t)B_OK);
	CHECK_STR(Text(s, "tags"), "z");
	type_code type; size_t count;
	CHECK_EQ(FieldOf(s, "tags", &type, &count), (status_t)B_OK);
	CHECK_EQ(count, (size_t)1);
}

TEST(SetValuesMakesAFieldThatIsNotThereAndRemovesAnEmptiedOne)
{
	BMessage s = MakeSettings();
	CHECK_EQ(SetValues(&s, "", "tags", B_STRING_TYPE, {}), (status_t)B_OK);
	CHECK(!Has(s, "tags"));
	// an array that was emptied comes back in its type
	CHECK_EQ(SetValues(&s, "", "tags", B_STRING_TYPE, {"p", "q"}), (status_t)B_OK);
	CHECK_STR(Text(s, "tags_01"), "q");
	// and in a nested message
	CHECK_EQ(SetValues(&s, "nested", "ids", B_INT32_TYPE, {"4", "5"}), (status_t)B_OK);
	CHECK_STR(Text(s, "nested.ids_01"), "5");
	CHECK_STR(Text(s, "nested.depth"), "2");		// (the rest of the nested message is as it was)
	CHECK_EQ(SetValues(&s, "nested.items_01", "extra", B_BOOL_TYPE, {"true"}), (status_t)B_OK);
	CHECK_STR(Text(s, "nested.items_01.extra"), "true");
}

TEST(SetValuesChangesNothingIfOneTextDoesNotFit)
{
	BMessage s = MakeSettings();
	BMessage before(s);
	CHECK_EQ(SetValues(&s, "nested", "ids", B_INT32_TYPE, {"1", "two"}), (status_t)B_BAD_VALUE);
	CHECK(!Has(s, "nested.ids"));
	CHECK_EQ(SetValues(&s, "", "max_authors", B_INT32_TYPE, {"x"}), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetValues(&s, "", "max_authors", B_STRING_TYPE, {"x"}), (status_t)B_BAD_TYPE);		// the field has another type
	CHECK_EQ(SetValues(&s, "", "blob", B_RAW_TYPE, {"x"}), (status_t)B_BAD_TYPE);
	CHECK_EQ(SetValues(&s, "nope", "x", B_STRING_TYPE, {"x"}), (status_t)B_NAME_NOT_FOUND);
	CHECK_EQ(SetValues(&s, "email", "x", B_STRING_TYPE, {"x"}), (status_t)B_BAD_TYPE);			// not a message
	CHECK(s.HasSameData(before));
}

TEST(ATimeIsReadAndWrittenAsADate)
{
	BMessage s = MakeSettings();
	const time_t first = 86400 * 11017;
	s.AddData("released", B_TIME_TYPE, &first, sizeof(first));
	CHECK_STR(Text(s, "released"), "2000-03-01");
	CHECK_EQ(SetText(&s, "released", "2024-02-29"), (status_t)B_OK);
	CHECK_STR(Text(s, "released"), "2024-02-29");
	type_code type; size_t count;
	CHECK_EQ(FieldOf(s, "released", &type, &count), (status_t)B_OK);
	CHECK_EQ(type, (type_code)B_TIME_TYPE);						// the type stays
	CHECK_EQ(SetText(&s, "released", "2024-02-30"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "released", "soon"), (status_t)B_BAD_VALUE);
	CHECK_STR(Text(s, "released"), "2024-02-29");
	CHECK_EQ(SetText(&s, "released", "2024-03-01T10:20:30Z"), (status_t)B_OK);
	CHECK_STR(Text(s, "released"), "2024-03-01T10:20:30Z");
	CHECK_EQ(SetValues(&s, "", "dates", B_TIME_TYPE, {"2020-01-01", "2021-01-01"}), (status_t)B_OK);
	CHECK_STR(Text(s, "dates_01"), "2021-01-01");
}

TEST(AValueIsCopiedWithItsTypeUnderThePathAsName)
{
	const BMessage s = MakeSettings();
	BMessage changes;
	CHECK_EQ(CopyValue(s, "nested.depth", &changes, "nested.depth"), (status_t)B_OK);
	CHECK_EQ(CopyValue(s, "email", &changes, "email"), (status_t)B_OK);
	CHECK_EQ(CopyValue(s, "tags_01", &changes, "tags_01"), (status_t)B_OK);
	CHECK_EQ(CopyValue(s, "ratio", &changes, "ratio"), (status_t)B_OK);
	CHECK_EQ(CopyValue(s, "unpaywall", &changes, "unpaywall"), (status_t)B_OK);
	CHECK_EQ(CopyValue(s, "nested", &changes, "nested"), (status_t)B_OK);		// a message as a whole
	CHECK_EQ(CopyValue(s, "nope", &changes, "nope"), (status_t)B_NAME_NOT_FOUND);
	CHECK_EQ(CopyValue(s, "tags_09", &changes, "tags_09"), (status_t)B_BAD_INDEX);
	int32 depth = 0;
	CHECK_EQ(changes.FindInt32("nested.depth", &depth), (status_t)B_OK);
	CHECK_EQ(depth, 2);
	CHECK_STR(changes.GetString("tags_01", ""), "y");
	double ratio = 0;
	CHECK_EQ(changes.FindDouble("ratio", &ratio), (status_t)B_OK);
	BMessage nested;
	CHECK_EQ(changes.FindMessage("nested", &nested), (status_t)B_OK);
}

TEST(ChangesAreAppliedByPathAndKeepTheirTypes)
{
	BMessage s = MakeSettings();
	BMessage changes;
	changes.AddInt32("nested.depth", 9);
	changes.AddString("email", "x@y.z");
	changes.AddBool("unpaywall", false);
	changes.AddString("tags_01", "yy");
	std::vector<std::string> refused;
	CHECK_EQ(ApplyChanges(&s, changes, &refused), (status_t)B_OK);
	CHECK(refused.empty());
	CHECK_STR(Text(s, "nested.depth"), "9");
	CHECK_STR(Text(s, "email"), "x@y.z");
	CHECK_STR(Text(s, "unpaywall"), "false");
	CHECK_STR(Text(s, "tags_01"), "yy");

	// the rest is applied if one value is refused (a string for a number, a path that is not there, a type that no text stands for)
	BMessage bad;
	bad.AddString("max_authors", "many");
	bad.AddString("email", "again@y.z");
	bad.AddString("nope", "1");
	bad.AddData("blob", B_RAW_TYPE, "1", 1);
	refused.clear();
	CHECK_EQ(ApplyChanges(&s, bad, &refused), (status_t)B_BAD_VALUE);
	CHECK_EQ(refused.size(), (size_t)3);
	CHECK_STR(Text(s, "email"), "again@y.z");
	CHECK_STR(Text(s, "max_authors"), "10");

	// an integer of another width is the number that it is
	BMessage wide;
	wide.AddInt64("max_authors", 11);
	CHECK_EQ(ApplyChanges(&s, wide), (status_t)B_OK);
	CHECK_STR(Text(s, "max_authors"), "11");
	int32 stored = 0;
	CHECK_EQ(s.FindInt32("max_authors", &stored), (status_t)B_OK);		// (still an int32)
}

TEST(PointsRectanglesAndColoursAreTextToo)
{
	BMessage s;
	s.AddPoint("p", BPoint(10, 20.5f));
	s.AddRect("r", BRect(1, 2, 30, 40));
	s.AddColor("c", (rgb_color){10, 20, 30, 255});
	s.AddColor("translucent", (rgb_color){255, 0, 128, 64});
	CHECK_STR(Text(s, "p"), "10, 20.5");
	CHECK_STR(Text(s, "r"), "1, 2, 30, 40");
	CHECK_STR(Text(s, "c"), "#0a141e");
	CHECK_STR(Text(s, "translucent"), "#ff008040");

	CHECK_EQ(SetText(&s, "p", "3 4"), (status_t)B_OK);
	CHECK_STR(Text(s, "p"), "3, 4");
	CHECK_EQ(SetText(&s, "r", "0,0,100,50.5"), (status_t)B_OK);
	CHECK_STR(Text(s, "r"), "0, 0, 100, 50.5");
	CHECK_EQ(SetText(&s, "c", "#FFF"), (status_t)B_OK);
	CHECK_STR(Text(s, "c"), "#ffffff");
	CHECK_EQ(SetText(&s, "c", "#11223344"), (status_t)B_OK);
	CHECK_STR(Text(s, "c"), "#11223344");

	// a text that is none of them changes nothing
	BMessage before(s);
	CHECK_EQ(SetText(&s, "p", "3"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "p", "3, 4, 5"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "r", "1, 2, 3"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "c", "red"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "c", "#12"), (status_t)B_BAD_VALUE);
	CHECK_EQ(SetText(&s, "c", "#ggg"), (status_t)B_BAD_VALUE);
	CHECK(s.HasSameData(before, true, true));
}

TEST_MAIN()
