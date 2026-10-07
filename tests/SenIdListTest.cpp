/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenIdList.h: the chunked lists of ids in SEN:TO and SEN:META. Plain C++, no Haiku API. */

#include "TestHarness.h"
#include "../src/cpp/include/SenIdList.h"

using namespace sen::idlist;

static std::vector<std::string> Ids(size_t count)
{
    std::vector<std::string> ids;
    for (size_t i = 0; i < count; i++)
        ids.push_back(sen::id::New());
    return ids;
}

TEST(ChunkNames)
{
    CHECK_STR(ChunkName("SEN:TO", 0), "SEN:TO");
    CHECK_STR(ChunkName("SEN:TO", 1), "SEN:TO:1");
    CHECK_STR(ChunkName("SEN:META", 7), "SEN:META:7");
}

TEST(SplitAndJoin)
{
    CHECK_EQ(Split("").size(), 0u);
    CHECK_EQ(Split(",").size(), 0u);
    CHECK_EQ(Split(" , ,").size(), 0u);
    CHECK_EQ(Split("A").size(), 1u);
    auto ids = Split("A,B , C,,D ");
    CHECK_EQ(ids.size(), 4u);
    CHECK_STR(ids[0], "A");
    CHECK_STR(ids[1], "B");
    CHECK_STR(ids[2], "C");
    CHECK_STR(ids[3], "D");
    CHECK_STR(Join(ids), "A,B,C,D");
    CHECK_STR(Join(ids, 1, 3), "B,C");
    CHECK_STR(Join(ids, 3, 99), "D");
    CHECK_STR(Join({}), "");
}

TEST(OneChunkUpToTheLimit)
{
    std::vector<std::string> chunks;
    CHECK(Chunk({}, &chunks));
    CHECK_EQ(chunks.size(), 0u);

    CHECK(Chunk(Ids(sen::id::kMaxPerAttribute), &chunks));
    CHECK_EQ(chunks.size(), 1u);
    CHECK_EQ(Split(chunks[0]).size(), sen::id::kMaxPerAttribute);
}

TEST(AChunkFitsTheIndexedBytes)
{
    // BFS indexes the first 255 bytes: the whole value of a chunk must be inside
    std::vector<std::string> chunks;
    CHECK(Chunk(Ids(kMaxIds), &chunks));
    for (const auto& chunk : chunks)
        CHECK(chunk.size() <= 255);
}

TEST(IdsContinueInTheNextChunk)
{
    auto ids = Ids(sen::id::kMaxPerAttribute + 1);
    std::vector<std::string> chunks;
    CHECK(Chunk(ids, &chunks));
    CHECK_EQ(chunks.size(), 2u);
    CHECK_STR(chunks[1], ids.back());
    // round trip
    std::vector<std::string> back;
    for (const auto& chunk : chunks)
        for (const auto& id : Split(chunk))
            back.push_back(id);
    CHECK(back == ids);
}

TEST(TheLimitOfAList)
{
    std::vector<std::string> chunks;
    CHECK(Chunk(Ids(kMaxIds), &chunks));
    CHECK_EQ(chunks.size(), kMaxChunks);
    chunks.assign(1, "unchanged");
    CHECK(!Chunk(Ids(kMaxIds + 1), &chunks));
    CHECK_EQ(chunks.size(), 1u);    // not changed when too long
    CHECK_STR(chunks[0], "unchanged");
}

TEST(AddAndRemove)
{
    std::vector<std::string> ids;
    CHECK(Add(&ids, "A"));
    CHECK(Add(&ids, "B"));
    CHECK(!Add(&ids, "A"));         // no duplicates
    CHECK_EQ(ids.size(), 2u);
    CHECK(Contains(ids, "B"));
    CHECK(Remove(&ids, "A"));
    CHECK(!Remove(&ids, "A"));
    CHECK(!Contains(ids, "A"));
    CHECK_EQ(ids.size(), 1u);
}

TEST(PredicateAsksEveryChunk)
{
    std::string predicate = ContainsPredicate("SEN:TO", "02T0XZKMKPMGB");
    CHECK(predicate.find("SEN:TO == '*02T0XZKMKPMGB*'") == 0);
    CHECK(predicate.find("SEN:TO:7 == '*02T0XZKMKPMGB*'") != std::string::npos);
    size_t parts = 1;
    for (size_t pos = predicate.find("||"); pos != std::string::npos; pos = predicate.find("||", pos + 2))
        parts++;
    CHECK_EQ(parts, kMaxChunks);
}

TEST_MAIN()
