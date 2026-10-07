/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenId.h (TSID). Plain C++, no Haiku API: runs on Linux, macOS and Haiku. */

#include <set>
#include <string>
#include <thread>
#include <vector>

#include "TestHarness.h"
#include "../src/cpp/include/SenId.h"

using namespace sen::id;

TEST(EncodeDecodeRoundTrip)
{
	const uint64_t values[] = {0, 1, 31, 32, 0x123456789ABCULL, Compose(kEpochMs + 86400000ULL, 7, 99)};
	for (uint64_t value : values) {
		char text[kLength];
		Encode(value, text);
		CHECK_EQ(strlen(text), kTextLength);
		uint64_t back = 0;
		CHECK(Decode(text, &back));
		CHECK_EQ(back, value);
	}
}

TEST(DecodeRejectsMalformed)
{
	uint64_t value;
	CHECK(!Decode(nullptr, &value));
	CHECK(!Decode("", &value));
	CHECK(!Decode("0Q8F2H3K9X1Z", &value));		// too short
	CHECK(!Decode("0Q8F2H3K9X1ZMM", &value));	// too long
	CHECK(!Decode("0Q8F2H3K9X1ZU", &value));	// U is not in the alphabet
	CHECK(!Decode("0Q8F2H3K9X1ZI", &value));	// nor I, L, O
	CHECK(IsValid("0q8f2h3k9x1zm"));			// lowercase is accepted
}

TEST(ComposeLayout)
{
	uint64_t value = Compose(kEpochMs + 1000, 5, 3);
	CHECK_EQ(TimeOf(value), kEpochMs + 1000);
	CHECK_EQ((value >> kCounterBits) & ((1 << kMachineBits) - 1), 5u);
	CHECK_EQ(value & ((1 << kCounterBits) - 1), 3u);
	// the machine and counter are masked, they must not spill into the neighbours
	uint64_t spilled = Compose(kEpochMs, 0xFFFF, 0xFFFFF);
	CHECK_EQ(TimeOf(spilled), kEpochMs);
	// a time before the epoch is the epoch
	CHECK_EQ(TimeOf(Compose(0, 0, 0)), kEpochMs);
}

TEST(ProducesValidIds)
{
	std::string id = New();
	CHECK_EQ(id.size(), kTextLength);
	CHECK(IsValid(id.c_str()));
}

TEST(IdsAreUniqueAndGrow)
{
	std::string last;
	std::set<std::string> seen;
	for (int i = 0; i < 20000; i++) {
		std::string id = New();
		CHECK(seen.insert(id).second);		// unique
		CHECK(last < id);					// strictly growing within the process, also in the same millisecond
		last = id;
	}
}

TEST(IdsAreUniqueAcrossThreads)
{
	std::vector<std::vector<std::string> > results(4);
	std::vector<std::thread> threads;
	for (int t = 0; t < 4; t++) {
		threads.emplace_back([&results, t]() {
			for (int i = 0; i < 5000; i++)
				results[t].push_back(New());
		});
	}
	for (auto& thread : threads)
		thread.join();
	std::set<std::string> seen;
	for (auto& list : results)
		for (auto& id : list)
			CHECK(seen.insert(id).second);
}

TEST(CreationTimeIsNow)
{
	struct timeval now;
	gettimeofday(&now, nullptr);
	uint64_t nowMs = (uint64_t)now.tv_sec * 1000 + now.tv_usec / 1000;
	uint64_t value;
	CHECK(Decode(New().c_str(), &value));
	uint64_t created = TimeOf(value);
	CHECK(created + 2000 >= nowMs && created <= nowMs + 2000);
}

TEST(IriConversion)
{
	CHECK_STR(ToIri("0Q8F2H3K9X1ZM"), "urn:sen:0Q8F2H3K9X1ZM");
	CHECK_STR(ToIri("urn:uuid:1234"), "urn:uuid:1234");		// has a scheme already
	CHECK_STR(ToIri(""), "");
	CHECK_STR(FromIri("urn:sen:0Q8F2H3K9X1ZM"), "0Q8F2H3K9X1ZM");
	CHECK_STR(FromIri("urn:uuid:1234"), "1234");
	CHECK_STR(FromIri("0Q8F2H3K9X1ZM"), "0Q8F2H3K9X1ZM");
}

TEST(IdsFitAnIndexedAttribute)
{
	// BFS indexes the first 255 bytes of a string: the maximum list must fit
	size_t bytes = kMaxPerAttribute * kTextLength + (kMaxPerAttribute - 1);
	CHECK(bytes <= 255);
}

TEST_MAIN()
