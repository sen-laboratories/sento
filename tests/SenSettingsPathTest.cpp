/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenSettingsPath.h: path keys for the fields of nested messages. Plain C++, no Haiku API. */

#include "TestHarness.h"
#include "../src/cpp/include/SenSettingsPath.h"

#include <map>

using namespace sen::settings;
using Segments = std::vector<std::string>;

/** A message as far as the paths are concerned: the names of its fields and the number of their values. */
struct FakeMessage {
	std::map<std::string, size_t> fields;
	FieldInfo Info() const
	{
		return [this](const std::string& name, size_t* count) {
			auto it = fields.find(name);
			if (it == fields.end())
				return false;
			*count = it->second;
			return true;
		};
	}
};

static Segments
Split(const std::string& path, bool* ok = nullptr)
{
	Segments segments;
	const bool result = SplitPath(path, &segments);
	if (ok != nullptr)
		*ok = result;
	return segments;
}

TEST(SplitPathSplitsAtTheDots)
{
	CHECK(Split("a") == Segments{"a"});
	CHECK(Split("nested.property_02") == (Segments{"nested", "property_02"}));
	CHECK(Split("a.b.c.d") == (Segments{"a", "b", "c", "d"}));
	bool ok = false;
	CHECK(Split("", &ok).empty());
	CHECK(ok);		// the empty path is the message itself
}

TEST(SplitPathRefusesEmptySegments)
{
	for (const char* path : {".", ".a", "a.", "a..b", ".."}) {
		bool ok = true;
		CHECK(Split(path, &ok).empty());
		CHECK(!ok);
	}
}

TEST(EscapedDotsAndBackslashesBelongToTheName)
{
	CHECK(Split("a\\.b") == Segments{"a.b"});
	CHECK(Split("a\\.b.c") == (Segments{"a.b", "c"}));
	CHECK(Split("a\\\\.b") == (Segments{"a\\", "b"}));		// an escaped backslash, then the separator
	CHECK(Split("a\\\\\\.b") == Segments{"a\\.b"});
	// a backslash before anything else is an ordinary character, also at the end
	CHECK(Split("a\\b") == Segments{"a\\b"});
	CHECK(Split("a\\") == Segments{"a\\"});
}

TEST(JoinPathWritesWhatSplitPathReads)
{
	const std::vector<Segments> cases = {
		{"a"}, {"nested", "property_02"}, {"a.b", "c"}, {"a\\", "b"}, {"a\\.b"}, {"with space", "ü"}, {"x", "y", "z"},
	};
	for (const Segments& segments : cases) {
		const std::string path = JoinPath(segments);
		bool ok = false;
		CHECK(Split(path, &ok) == segments);
		CHECK(ok);
	}
	CHECK_STR(JoinPath({"a.b", "c"}), "a\\.b.c");
	CHECK_STR(JoinPath({}), "");
}

TEST(ChildPathAddsASegment)
{
	CHECK_STR(ChildPath("", "a"), "a");
	CHECK_STR(ChildPath("a", "b"), "a.b");
	CHECK_STR(ChildPath("a.b", "c.d"), "a.b.c\\.d");
}

TEST(AnIndexSuffixIsAnUnderscoreAndTwoOrMoreDigits)
{
	CHECK_STR(IndexSuffix(0), "_00");
	CHECK_STR(IndexSuffix(2), "_02");
	CHECK_STR(IndexSuffix(42), "_42");
	CHECK_STR(IndexSuffix(100), "_100");

	std::string name;
	size_t index = 0;
	CHECK(ParseIndexSuffix("tags_02", &name, &index));
	CHECK_STR(name, "tags");
	CHECK_EQ(index, (size_t)2);
	CHECK(ParseIndexSuffix("tags_100", &name, &index));
	CHECK_EQ(index, (size_t)100);
	CHECK(ParseIndexSuffix("tags_007", &name, &index));		// more digits than needed are fine
	CHECK_EQ(index, (size_t)7);
	CHECK(ParseIndexSuffix("a_b_03", &name, &index));		// the last underscore counts
	CHECK_STR(name, "a_b");
	CHECK_EQ(index, (size_t)3);

	CHECK(!ParseIndexSuffix("tags_2", &name, &index));			// one digit: part of the name
	CHECK(!ParseIndexSuffix("tags_", &name, &index));
	CHECK(!ParseIndexSuffix("tags", &name, &index));
	CHECK(!ParseIndexSuffix("_02", &name, &index));				// no name
	CHECK(!ParseIndexSuffix("tags_0x", &name, &index));
	CHECK(!ParseIndexSuffix("tags_-1", &name, &index));
	CHECK(!ParseIndexSuffix("tags_1234567890", &name, &index));	// too long for an index
}

TEST(AFieldWithTheExactNameWinsOverAnIndex)
{
	FakeMessage message{{{"max_02", 1}, {"tags", 3}, {"depth", 1}}};
	Step step;

	CHECK(ResolveSegment("max_02", message.Info(), &step));		// a field that is called like an index
	CHECK_STR(step.name, "max_02");
	CHECK_EQ(step.index, (size_t)0);
	CHECK(!step.suffixed);

	CHECK(ResolveSegment("tags_02", message.Info(), &step));	// no such field: the third value of tags
	CHECK_STR(step.name, "tags");
	CHECK_EQ(step.index, (size_t)2);
	CHECK_EQ(step.count, (size_t)3);
	CHECK(step.suffixed);

	CHECK(ResolveSegment("tags", message.Info(), &step));		// no suffix: the first value
	CHECK_EQ(step.index, (size_t)0);
	CHECK_EQ(step.count, (size_t)3);
}

TEST(ASegmentWithoutAFieldIsNotResolved)
{
	FakeMessage message{{{"tags", 3}}};
	Step step;
	CHECK(!ResolveSegment("other", message.Info(), &step));
	CHECK(!ResolveSegment("other_01", message.Info(), &step));
	CHECK(!ResolveSegment("tag_01", message.Info(), &step));
	CHECK(!ResolveSegment("", message.Info(), &step));
}

TEST(AnIndexBeyondTheValuesIsLeftToTheCaller)
{
	// the caller refuses it for a read, and for a write it appends if the index is the count
	FakeMessage message{{{"tags", 3}}};
	Step step;
	CHECK(ResolveSegment("tags_03", message.Info(), &step));
	CHECK_EQ(step.index, (size_t)3);
	CHECK_EQ(step.count, (size_t)3);
	CHECK(ResolveSegment("tags_99", message.Info(), &step));
	CHECK_EQ(step.index, (size_t)99);
}

TEST(TheSegmentOfAValueResolvesBackToIt)
{
	FakeMessage message{{{"tags", 3}, {"depth", 1}, {"max_02", 1}}};
	// a field with several values: the suffix, also for the first one
	CHECK_STR(SegmentFor("tags", 0, 3, message.Info()), "tags_00");
	CHECK_STR(SegmentFor("tags", 2, 3, message.Info()), "tags_02");
	// a field with one value: the name alone
	CHECK_STR(SegmentFor("depth", 0, 1, message.Info()), "depth");
	CHECK_STR(SegmentFor("max_02", 0, 1, message.Info()), "max_02");
	// for all of them: resolving the segment gives the same value back
	for (size_t index = 0; index < 3; index++) {
		Step step;
		CHECK(ResolveSegment(SegmentFor("tags", index, 3, message.Info()), message.Info(), &step));
		CHECK_STR(step.name, "tags");
		CHECK_EQ(step.index, index);
	}
}

TEST(AValueThatCannotBeAddressedHasNoSegment)
{
	// "tags_02" is a field of its own, so the third value of "tags" cannot be named; the others can
	FakeMessage message{{{"tags", 3}, {"tags_02", 1}}};
	CHECK_STR(SegmentFor("tags", 2, 3, message.Info()), "");
	CHECK_STR(SegmentFor("tags", 1, 3, message.Info()), "tags_01");
	CHECK_STR(SegmentFor("tags_02", 0, 1, message.Info()), "tags_02");
	// a value that is not there
	CHECK_STR(SegmentFor("nothing", 0, 1, message.Info()), "");
	// an index for a field with one value is no address
	CHECK_STR(SegmentFor("tags_02", 1, 1, message.Info()), "");
}

TEST_MAIN()
