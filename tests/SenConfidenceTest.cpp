/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenConfidence.h: the combination of evidence, the share, the merge of a stored value. Plain C++, no Haiku API. */

#include "TestHarness.h"
#include "../src/cpp/include/SenConfidence.h"

using namespace sen::confidence;

static bool Near(double a, double b) { return std::fabs(a - b) < 0.005; }

TEST(OneEvidenceIsItself)
{
	CHECK(Near(Combine({0.75}), 0.75));
	CHECK(Combine({}) == 0);
	CHECK(Combine({1.0}) == kMaxEstimate);			// (a program never says 1)
}

TEST(MoreEvidenceIsSurerButNeverCertain)
{
	const double one = Combine({0.75}), two = Combine({0.75, 0.75}), four = Combine({0.75, 0.75, 0.75, 0.75});
	CHECK(one < two && two < four && four <= kMaxEstimate);
	CHECK(Near(Combine({0.75, 0.75}), 0.9375));		// independent: 1 - 0.25 * 0.25
	CHECK(Near(Combine({0.55, 0.9}), Combine({0.9, 0.55})));	// (the order does not matter)
}

TEST(EvidenceOfOneSourceCountsLess)
{
	// four authors of one paper at one institution: surer than one, less sure than four independent witnesses
	const double copied = Combine({0.75, 0.75, 0.75, 0.75}, kRepeatedSource);
	CHECK(copied > Combine({0.75}) && copied < Combine({0.75, 0.75, 0.75, 0.75}));
	CHECK(Near(Combine({0.75, 0.75, 0.75}, 1.0), 0.75));	// a copy adds nothing
}

TEST(ShareDividesAmongCandidates)
{
	CHECK(Share(0.75, 1) == 0.75 && Share(0.75, 0) == 0.75);
	CHECK(Near(Share(0.75, 2), 0.53) && Near(Share(0.75, 4), 0.375));
	CHECK(Share(0.75, 3) < Share(0.75, 2));
}

TEST(MergeKeepsTheHigherAndNeverTouchesAnAsserted)
{
	CHECK(Near(Merge(0.5, 0.8), 0.8));				// (more evidence arrived)
	CHECK(Near(Merge(0.8, 0.5), 0.8));				// (an enrichment never lowers)
	CHECK(Merge(1.0, 0.3) == 1.0);					// (confirmed by a person)
	CHECK(Merge(0.5, 1.0) == kMaxEstimate);			// (an enrichment never confirms)
	CHECK(IsAsserted(1.0) && !IsAsserted(0.99));
}

TEST(RealCases)
{
	// the paper names the institution (stated) and an author's position covers the day (dated): two sources, surer than either
	const double both = Combine({weight::kStated, weight::kDatedOverlap});
	CHECK(both > weight::kStated);
	// an author with two positions that both cover the day: each is less sure than one alone
	CHECK(Combine({Share(weight::kDatedOverlap, 2)}) < Combine({weight::kDatedOverlap}));
	// a current position alone (no date of the paper in it): a guess that is better than none and worse than a dated one
	CHECK(weight::kOpenOverlap < weight::kDatedOverlap && weight::kDatedOverlap < weight::kStated);
}

TEST_MAIN()
