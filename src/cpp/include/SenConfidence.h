/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenConfidence.h
 * @brief How sure a program is about a relation that it made: `SEN:REL:Confidence`, a number from 0 to 1. Plain C++, no Haiku API.
 *
 * **What the number means.** An estimate, read as a percentage or as a probability that the relation is right (0.8: four of five such relations are). It is not measured: the weights below
 * are judgements, and what counts is that two relations rank right (a relation that two sources agree on is surer than one that one source guesses). A relation that has **no value**
 * was made by a person, or taken from a source that states it, or confirmed: it counts as 1 (kAsserted). An enrichment never gives a relation the 1 (kMaxEstimate is the limit).
 *
 * **How a program estimates.** It lists the *evidence* for the relation, one number for each (the weights in `weight`), and combines them with Combine(): the relation is
 * wrong only if all the evidence is wrong (a noisy OR), but evidence that comes from the same place (the authors of one paper repeat each other) counts less (`correlation`). When the
 * evidence does not point to one relation alone (a person has two positions on the day of the paper: it can be either), Share() divides it.
 *
 * **What the server does with it.** A relation that is made again is the same relation, whatever its confidence; the value that is stored goes up when the new estimate is higher, and
 * never changes a relation that has no value or the value 1: see Merge().
 */

#include <algorithm>
#include <cmath>
#include <vector>

namespace sen {
namespace confidence {

/** The value of a relation that a person made or confirmed, or that a source states without doubt. */
inline constexpr double kAsserted = 1.0;
/** The highest estimate of a program: only a person or a source gives more. */
inline constexpr double kMaxEstimate = 0.99;

/** The weights of the evidence, how much one kind of it says for a relation (see the table in docs/semantic-types.md). */
namespace weight {
/** a register's identifier that both sides share (an ORCID iD, a ROR id, a DOI): it names the same thing */
inline constexpr double kIdentifier = 0.97;
/** the source states the relation as data: the paper lists its authors and the institutions that it names; a person keeps their employments at ORCID */
inline constexpr double kStated = 0.9;
/** a stay or a position that has a start and an end, and the event (the day of the paper) is between them */
inline constexpr double kDatedOverlap = 0.75;
/** a position that began before the event and has no end: where the person is now, so possibly also then */
inline constexpr double kOpenOverlap = 0.55;
/** a membership that has no dates at all: it says nothing about the day */
inline constexpr double kUndated = 0.35;
/** the same name and nothing else: a guess that is right for a rare name */
inline constexpr double kNameOnly = 0.5;
}	// namespace weight

/** The default: how much of the second and later evidence counts when the evidence is of one source or of people who copy each other (0: independent, 1: it adds nothing). */
inline constexpr double kRepeatedSource = 0.5;

inline double
Clamp(double value)
{
	if (!(value > 0))		// (also NaN)
		return 0;
	return value > 1 ? 1 : value;
}

/** The estimate is never more than kMaxEstimate. */
inline double
Estimate(double value)
{
	return std::min(kMaxEstimate, Clamp(value));
}

/**
 * The confidence that all the evidence together gives: the strongest counts in full, each further one by what is left (`1 - e`) and by how independent it is.
 * No evidence is no confidence (0). `correlation` is how much the evidence repeats itself, from 0 (independent) to 1 (a copy: nothing is added).
 */
inline double
Combine(std::vector<double> evidence, double correlation = 0.0)
{
	if (evidence.empty())
		return 0;
	std::sort(evidence.begin(), evidence.end(), [](double a, double b) { return a > b; });
	const double independent = 1.0 - Clamp(correlation);
	double wrong = 1.0 - Clamp(evidence[0]);
	for (size_t i = 1; i < evidence.size(); i++)
		wrong *= 1.0 - Clamp(evidence[i]) * independent;
	return Estimate(1.0 - wrong);
}

/**
 * The share of the evidence for one of several things that it fits equally: a person with two positions on the day of the paper has evidence for both, and the paper may come from either or from
 * both. The evidence is divided by the square root of their number (two: 0.71, three: 0.58): less than the whole, more than an equal part, because the paper often comes from both.
 */
inline double
Share(double evidence, int candidates)
{
	return Clamp(candidates <= 1 ? evidence : evidence / std::sqrt((double)candidates));
}

/** Whether a stored value is the one of a relation that nobody doubts (no value is read as 1 by the caller: see the header). */
inline bool
IsAsserted(double stored)
{
	return stored >= kAsserted;
}

/**
 * The value to keep when a relation that has `stored` is made again with the estimate `incoming`: the higher one, but a value of 1 stays. A relation that has no value stays without
 * one: the caller does not call this for it.
 */
inline double
Merge(double stored, double incoming)
{
	if (IsAsserted(stored))
		return stored;
	return std::max(Clamp(stored), Estimate(incoming));
}

}	// namespace confidence
}	// namespace sen
