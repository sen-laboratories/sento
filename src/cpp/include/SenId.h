/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024-2026 SEN Labs e.U.
 */
#pragma once

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <string>

/**
 * @file SenId.h
 * @brief TSID: the compact, time-sorted identifier of SEN (`SEN:ID`, relation ids, annotation ids).
 *
 * The layout is that of a Snowflake ID: 42 bits of milliseconds since 2026-01-01 (good for 139 years), 10 bits for the
 * machine and 12 bits for a counter, written as 13 characters of Crockford's Base32, e.g. `0Q8F2H3K9X1ZM`. They sort by
 * creation time, are a third of the length of a UUID, and only grow within a process. The same algorithm is used by
 * Toji's Web Annotation code; there is deliberately one implementation.
 *
 * Header only and free of Haiku APIs, so that it can be tested and used on other platforms.
 */
namespace sen {
namespace id {

/** Characters of a TSID, without the terminating NUL. */
inline constexpr size_t kTextLength = 13;
/** Size of a buffer for a TSID with the terminating NUL. */
inline constexpr size_t kLength = kTextLength + 1;
/**
 * How many ids fit into one attribute. BFS indexes only the first 255 bytes of a string; 16 ids and 15 commas are 223 bytes,
 * which leaves room to spare.
 */
inline constexpr size_t kMaxPerAttribute = 16;

/** Prefix that makes a TSID an IRI. */
inline constexpr char kIriPrefix[] = "urn:sen:";

/** Milliseconds from the Unix epoch to 2026-01-01T00:00:00Z. */
inline constexpr uint64_t kEpochMs = 1767225600000ULL;
inline constexpr int kMachineBits = 10;
inline constexpr int kCounterBits = 12;

/** The alphabet of Crockford's Base32 (no I, L, O, U). */
inline constexpr char kAlphabet[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

namespace detail {

inline uint64_t
RandomBits(int bits)
{
	uint64_t value = 0;
	bool random = false;
	FILE* file = fopen("/dev/urandom", "rb");
	if (file != nullptr) {
		random = fread(&value, sizeof(value), 1, file) == 1;
		fclose(file);
	}
	if (!random) {
		struct timeval now;
		gettimeofday(&now, nullptr);
		value = ((uint64_t)now.tv_usec << 32) ^ (uint64_t)now.tv_sec ^ (uint64_t)(uintptr_t)&value;
	}
	return value & (UINT64_MAX >> (64 - bits));
}

/** The same on every start: a hash (FNV-1a) of the name of the machine. */
inline uint64_t
MachineId()
{
	char host[256] = "";
	gethostname(host, sizeof(host) - 1);
	uint64_t hash = 1469598103934665603ULL;
	for (const char* c = host; *c != '\0'; c++)
		hash = (hash ^ (unsigned char)*c) * 1099511628211ULL;
	return (hash ^ (hash >> kMachineBits) ^ (hash >> (2 * kMachineBits))) & (UINT64_MAX >> (64 - kMachineBits));
}

}	// namespace detail

/**
 * @brief Encode the 64 bit value of a TSID as 13 characters.
 * @param value the TSID value (the upper 22 bits of the 64 bit word are unused: the value has 64 bits only by layout).
 * @param text  a buffer of at least kLength bytes
 */
inline void
Encode(uint64_t value, char* text)
{
	for (int i = (int)kTextLength - 1; i >= 0; i--, value >>= 5)
		text[i] = kAlphabet[value & 31];
	text[kTextLength] = '\0';
}

/**
 * @brief Decode a TSID text into its 64 bit value.
 * @return false if the text is not exactly 13 characters of the alphabet (lowercase is accepted).
 */
inline bool
Decode(const char* text, uint64_t* value)
{
	if (text == nullptr || strlen(text) != kTextLength)
		return false;
	uint64_t result = 0;
	for (size_t i = 0; i < kTextLength; i++) {
		char c = text[i];
		if (c >= 'a' && c <= 'z')
			c -= 'a' - 'A';
		const char* position = c != '\0' ? strchr(kAlphabet, c) : nullptr;
		if (position == nullptr)
			return false;
		result = (result << 5) | (uint64_t)(position - kAlphabet);
	}
	*value = result;
	return true;
}

/** @return true if the text is a well formed TSID. */
inline bool
IsValid(const char* text)
{
	uint64_t value;
	return Decode(text, &value);
}

/**
 * @brief Make a new TSID from the given time, machine and counter state. Used by NewId() and by the tests.
 * @param unixMs milliseconds since the Unix epoch
 * @param machine the machine number (10 bits)
 * @param counter the counter (12 bits)
 */
inline uint64_t
Compose(uint64_t unixMs, uint64_t machine, uint64_t counter)
{
	if (unixMs < kEpochMs)
		unixMs = kEpochMs;
	return ((unixMs - kEpochMs) << (kMachineBits + kCounterBits))
		| ((machine & ((1ULL << kMachineBits) - 1)) << kCounterBits)
		| (counter & ((1ULL << kCounterBits) - 1));
}

/** @return the creation time of a TSID value, in milliseconds since the Unix epoch. */
inline uint64_t
TimeOf(uint64_t value)
{
	return (value >> (kMachineBits + kCounterBits)) + kEpochMs;
}

/**
 * @brief A new identifier. Thread safe; within a process the ids only grow: a clock that goes back, or more than 2000
 * ids in a millisecond, move on to the next millisecond.
 *
 * The machine is a hash of the host name and the counter of a millisecond starts at a random place, so that two
 * programs that make an id in the same millisecond rarely agree.
 *
 * @param text a buffer of at least kLength bytes that receives the id
 */
inline void
New(char* text)
{
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	static uint64_t lastMs = 0;
	static uint64_t counter = 0;
	static const uint64_t machine = detail::MachineId();

	struct timeval now;
	gettimeofday(&now, nullptr);
	uint64_t ms = (uint64_t)now.tv_sec * 1000 + now.tv_usec / 1000;
	if (ms < kEpochMs)
		ms = kEpochMs;

	pthread_mutex_lock(&lock);
	if (ms > lastMs) {
		lastMs = ms;
		counter = detail::RandomBits(kCounterBits - 1);		// the upper half stays free to count up
	} else if (++counter >= (1ULL << kCounterBits)) {
		lastMs++;
		counter = detail::RandomBits(kCounterBits - 1);
	}
	uint64_t value = Compose(lastMs, machine, counter);
	pthread_mutex_unlock(&lock);

	Encode(value, text);
}

/** @return a new identifier as a string. */
inline std::string
New()
{
	char text[kLength];
	New(text);
	return std::string(text);
}

/** @return the identifier as an IRI (`urn:sen:<tsid>`); text that already has a scheme is returned as it is. */
inline std::string
ToIri(const std::string& id)
{
	if (!id.empty() && id.find(':') == std::string::npos)
		return std::string(kIriPrefix) + id;
	return id;
}

/** @return the identifier in an IRI of ours (or in a `urn:uuid:...`, which other programs make); text without a scheme is returned as it is. */
inline std::string
FromIri(const std::string& iri)
{
	static const char kUuid[] = "urn:uuid:";
	if (iri.compare(0, sizeof(kIriPrefix) - 1, kIriPrefix) == 0)
		return iri.substr(sizeof(kIriPrefix) - 1);
	if (iri.compare(0, sizeof(kUuid) - 1, kUuid) == 0)
		return iri.substr(sizeof(kUuid) - 1);
	return iri;
}

}	// namespace id
}	// namespace sen
