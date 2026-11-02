/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenSettingsPath.h
 * @brief Path keys for the fields of nested messages (plain C++, no Haiku API, tested on every platform in tests/SenSettingsPathTest.cpp).
 *
 * The form that shows a settings message names the control of every value with the **path key** of the field (see docs/spec-settings-and-message-template.md).
 * A path key is a list of segments separated by dots, a segment is the name of a field, with an index if the field has several values:
 *
 *     nested.property_02          the third value of the field "property" of the message "nested"
 *
 *   - the index is the suffix `_` and two or more digits (`_00` ... `_99`, `_100`), zero based; a suffix with one digit is part of the name;
 *   - a segment is looked up as a field with exactly that name first; only if there is none and it ends in an index suffix, the rest is the field and the
 *     digits the index (so a field that is called `max_02` is found by `max_02`);
 *   - a `.` or `\` in a name is written `\.` or `\\` in the path; a backslash before anything else is an ordinary character;
 *   - a path has no empty segments (`a..b`, `.a` and `a.` are not paths); the empty path is the message itself.
 *
 * The functions here know nothing of the message: what they need to know about it is given as a FieldInfo (does a field with this name exist, and how many
 * values does it have). SenMessagePath.h binds them to BMessage.
 */

#include <stddef.h>
#include <stdio.h>

#include <functional>
#include <string>
#include <vector>

namespace sen {
namespace settings {

/** A name as a segment of a path: `.` and `\` get a backslash. */
inline std::string
EscapeSegment(const std::string& segment)
{
	std::string escaped;
	for (char c : segment) {
		if (c == '.' || c == '\\')
			escaped += '\\';
		escaped += c;
	}
	return escaped;
}


/** The path of the segments (names, not escaped). */
inline std::string
JoinPath(const std::vector<std::string>& segments)
{
	std::string path;
	for (size_t i = 0; i < segments.size(); i++) {
		if (i > 0)
			path += '.';
		path += EscapeSegment(segments[i]);
	}
	return path;
}


/** The path below another one (`parent` empty: the segment alone). */
inline std::string
ChildPath(const std::string& parent, const std::string& segment)
{
	return parent.empty() ? EscapeSegment(segment) : parent + '.' + EscapeSegment(segment);
}


/**
 * The segments of a path (without the escapes). False, and no segments, if a segment is empty. The empty path has no segments and is fine.
 */
inline bool
SplitPath(const std::string& path, std::vector<std::string>* segments)
{
	segments->clear();
	if (path.empty())
		return true;

	std::string current;
	for (size_t i = 0; i < path.size(); i++) {
		const char c = path[i];
		if (c == '\\' && i + 1 < path.size() && (path[i + 1] == '.' || path[i + 1] == '\\')) {
			current += path[++i];
		} else if (c == '.') {
			if (current.empty()) {
				segments->clear();
				return false;
			}
			segments->push_back(current);
			current.clear();
		} else {
			current += c;
		}
	}
	if (current.empty()) {
		segments->clear();
		return false;
	}
	segments->push_back(current);
	return true;
}


/** The suffix of an index: at least two digits (`_00`, `_07`, `_12`, `_100`). */
inline std::string
IndexSuffix(size_t index)
{
	char suffix[32];
	snprintf(suffix, sizeof(suffix), "_%02zu", index);
	return suffix;
}


/**
 * Whether a segment ends in an index suffix: `tags_02` is `tags` and 2. The name must not be empty, the suffix has two to nine digits (`_2`, `_x2` and
 * `_1234567890` are part of a name).
 */
inline bool
ParseIndexSuffix(const std::string& segment, std::string* name, size_t* index)
{
	const size_t underscore = segment.rfind('_');
	if (underscore == std::string::npos || underscore == 0)
		return false;
	const size_t digits = segment.size() - underscore - 1;
	if (digits < 2 || digits > 9)
		return false;

	size_t value = 0;
	for (size_t i = underscore + 1; i < segment.size(); i++) {
		if (segment[i] < '0' || segment[i] > '9')
			return false;
		value = value * 10 + (segment[i] - '0');
	}
	*name = segment.substr(0, underscore);
	*index = value;
	return true;
}


/** What the functions need to know of a message: whether it has a field with this name, and how many values the field has. */
using FieldInfo = std::function<bool(const std::string& name, size_t* count)>;

/** A segment resolved: the field, the index of the value, and how many values the field has (the index may be outside of them: the caller decides). */
struct Step {
	std::string	name;
	size_t		index = 0;
	size_t		count = 0;
	bool		suffixed = false;		///< the segment had an index suffix
};


/**
 * Resolves a segment: the field with exactly this name if there is one (the first value), else the field of the name without the index suffix.
 * False if there is no such field.
 */
inline bool
ResolveSegment(const std::string& segment, const FieldInfo& info, Step* step)
{
	size_t count = 0;
	if (info(segment, &count)) {
		step->name = segment;
		step->index = 0;
		step->count = count;
		step->suffixed = false;
		return true;
	}

	std::string name;
	size_t index = 0;
	if (ParseIndexSuffix(segment, &name, &index) && info(name, &count)) {
		step->name = name;
		step->index = index;
		step->count = count;
		step->suffixed = true;
		return true;
	}
	return false;
}


/**
 * The segment that names one value of a field: the name alone if the field has one value, the name and the index suffix if it has several (also for the
 * first, so that the controls of a list are named alike). Empty if the segment would be resolved to another value (a field `tags` with three values
 * and another field that is called `tags_02`: the third value of `tags` cannot be addressed, and the form shows it read-only).
 */
inline std::string
SegmentFor(const std::string& name, size_t index, size_t count, const FieldInfo& info)
{
	const std::string segment = count > 1 ? name + IndexSuffix(index) : name;
	Step step;
	if (!ResolveSegment(segment, info, &step))
		return std::string();
	return step.name == name && step.index == index ? segment : std::string();
}

}	// namespace settings
}	// namespace sen
