/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenMessagePath.h
 * @brief Reading and changing the fields of a nested BMessage by path keys (Haiku only, tested in tests/vm/SenMessagePathTest.cpp).
 *
 * The rules of the paths are those of SenSettingsPath.h. The functions work on a settings message as a form shows it: a value is read and written as **text**
 * (what a control holds), and the type of the field is kept: a number that does not fit the type, or a word for a number, is refused and the message is not
 * changed. Nothing here makes a field: a path names a value that is there, or the one after the last value of an array (a write there appends it).
 *
 * A BMessage hands out copies of the messages in it, so a change of a nested field is made on the copy and put back (at every level).
 *
 * Types that a text can stand for: bool, the integers (8 to 64 bit, signed and unsigned), float, double, string and time (`B_TIME_TYPE`, as `2026-11-03` or
 * `2026-11-03T14:30:00Z`, see FormatTime()). A field of another type can be
 * found and listed, not read or written as text (B_BAD_TYPE): a form shows it read-only and keeps it as it is.
 */

#include <Message.h>
#include <SupportDefs.h>

#include <charconv>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "SenSettingsPath.h"

namespace sen {
namespace settings {

namespace detail {

inline FieldInfo
InfoOf(const BMessage& message)
{
	return [&message](const std::string& name, size_t* count) {
		type_code type;
		int32 values;
		if (message.GetInfo(name.c_str(), &type, &values) != B_OK)
			return false;
		*count = (size_t)values;
		return true;
	};
}


inline bool
IsText(type_code type)
{
	switch (type) {
		case B_BOOL_TYPE:
		case B_INT8_TYPE: case B_INT16_TYPE: case B_INT32_TYPE: case B_INT64_TYPE:
		case B_UINT8_TYPE: case B_UINT16_TYPE: case B_UINT32_TYPE: case B_UINT64_TYPE:
		case B_FLOAT_TYPE: case B_DOUBLE_TYPE: case B_STRING_TYPE:
		case B_TIME_TYPE:
			return true;
	}
	return false;
}


inline bool
ParseBool(const std::string& text, bool* value)
{
	std::string lower;
	for (char c : text)
		lower += (char)tolower((unsigned char)c);
	if (lower == "true" || lower == "on" || lower == "yes" || lower == "1") {
		*value = true;
		return true;
	}
	if (lower == "false" || lower == "off" || lower == "no" || lower == "0") {
		*value = false;
		return true;
	}
	return false;
}


/** An integer in the text, nothing else around it but blanks, within [low, high]. */
inline bool
ParseInteger(const std::string& text, bool isSigned, long long low, unsigned long long high, long long* signedValue, unsigned long long* unsignedValue)
{
	size_t start = text.find_first_not_of(" \t");
	size_t end = text.find_last_not_of(" \t");
	if (start == std::string::npos)
		return false;
	const std::string trimmed = text.substr(start, end - start + 1);
	if (trimmed[0] == '+' || (!isSigned && trimmed[0] == '-'))
		return false;

	errno = 0;
	char* stop = NULL;
	if (isSigned) {
		const long long value = strtoll(trimmed.c_str(), &stop, 10);
		if (errno != 0 || *stop != '\0' || value < low || (value > 0 && (unsigned long long)value > high))
			return false;
		*signedValue = value;
	} else {
		const unsigned long long value = strtoull(trimmed.c_str(), &stop, 10);
		if (errno != 0 || *stop != '\0' || value > high)
			return false;
		*unsignedValue = value;
	}
	return true;
}


inline bool
ParseDouble(const std::string& text, double* value)
{
	size_t start = text.find_first_not_of(" \t");
	if (start == std::string::npos)
		return false;
	errno = 0;
	char* stop = NULL;
	const double parsed = strtod(text.c_str() + start, &stop);
	if (errno != 0 || stop == text.c_str() + start)
		return false;
	while (*stop == ' ' || *stop == '\t')
		stop++;
	if (*stop != '\0')
		return false;
	*value = parsed;
	return true;
}


/** The segments of a path and the last step: where the value is. */
struct Located {
	BMessage		container;		///< a copy of the message that has the field
	Step			step;
	type_code		type = 0;
};


inline status_t
Locate(const BMessage& root, const std::string& path, Located* located)
{
	std::vector<std::string> segments;
	if (!SplitPath(path, &segments) || segments.empty())
		return B_BAD_VALUE;

	BMessage current(root);
	for (size_t at = 0; at < segments.size(); at++) {
		Step step;
		if (!ResolveSegment(segments[at], InfoOf(current), &step))
			return B_NAME_NOT_FOUND;
		type_code type = 0;
		int32 count = 0;
		current.GetInfo(step.name.c_str(), &type, &count);
		if (at + 1 == segments.size()) {
			located->container = current;
			located->step = step;
			located->type = type;
			return B_OK;
		}
		if (type != B_MESSAGE_TYPE)
			return B_BAD_TYPE;
		if (step.index >= (size_t)count)
			return B_BAD_INDEX;
		BMessage child;
		status_t result = current.FindMessage(step.name.c_str(), (int32)step.index, &child);
		if (result != B_OK)
			return result;
		current = child;
	}
	return B_BAD_VALUE;
}


/** Changes what a path names, with `change(message, step, type)` on the message that has the field; nested messages are put back after the change. */
template<typename Change>
status_t
Modify(BMessage* message, const std::vector<std::string>& segments, size_t at, const Change& change)
{
	Step step;
	if (!ResolveSegment(segments[at], InfoOf(*message), &step))
		return B_NAME_NOT_FOUND;
	type_code type = 0;
	int32 count = 0;
	message->GetInfo(step.name.c_str(), &type, &count);
	if (at + 1 == segments.size())
		return change(message, step, type);

	if (type != B_MESSAGE_TYPE)
		return B_BAD_TYPE;
	if (step.index >= (size_t)count)
		return B_BAD_INDEX;
	BMessage child;
	status_t result = message->FindMessage(step.name.c_str(), (int32)step.index, &child);
	if (result == B_OK)
		result = Modify(&child, segments, at + 1, change);
	if (result == B_OK)
		result = message->ReplaceMessage(step.name.c_str(), (int32)step.index, &child);
	return result;
}


template<typename Change>
status_t
ModifyPath(BMessage* message, const std::string& path, const Change& change)
{
	std::vector<std::string> segments;
	if (!SplitPath(path, &segments) || segments.empty())
		return B_BAD_VALUE;
	// work on a copy: a change that fails half way leaves the message as it was
	BMessage copy(*message);
	status_t result = Modify(&copy, segments, 0, change);
	if (result == B_OK)
		*message = copy;
	return result;
}


/** Puts the value of the text as the one at `index` of the field, or appends it if the index is the count. */
inline status_t
StoreText(BMessage* message, const char* name, type_code type, size_t index, size_t count, const std::string& text)
{
	if (index > count)
		return B_BAD_INDEX;
	const bool append = index == count;
	const int32 at = (int32)index;
	long long signedValue = 0;
	unsigned long long unsignedValue = 0;

	switch (type) {
		case B_BOOL_TYPE: {
			bool value;
			if (!ParseBool(text, &value))
				return B_BAD_VALUE;
			return append ? message->AddBool(name, value) : message->ReplaceBool(name, at, value);
		}
		case B_STRING_TYPE:
			return append ? message->AddString(name, text.c_str()) : message->ReplaceString(name, at, text.c_str());
		case B_TIME_TYPE: {
			int64_t seconds;
			if (!ParseTime(text, &seconds))
				return B_BAD_VALUE;
			const time_t value = (time_t)seconds;
			return append ? message->AddData(name, B_TIME_TYPE, &value, sizeof(value)) : message->ReplaceData(name, B_TIME_TYPE, at, &value, sizeof(value));
		}
		case B_FLOAT_TYPE: {
			double value;
			if (!ParseDouble(text, &value))
				return B_BAD_VALUE;
			return append ? message->AddFloat(name, (float)value) : message->ReplaceFloat(name, at, (float)value);
		}
		case B_DOUBLE_TYPE: {
			double value;
			if (!ParseDouble(text, &value))
				return B_BAD_VALUE;
			return append ? message->AddDouble(name, value) : message->ReplaceDouble(name, at, value);
		}
#define SEN_SIGNED(TYPE, CTYPE, LOW, HIGH, NAME) \
		case TYPE: \
			if (!ParseInteger(text, true, LOW, HIGH, &signedValue, &unsignedValue)) \
				return B_BAD_VALUE; \
			return append ? message->Add##NAME(name, (CTYPE)signedValue) : message->Replace##NAME(name, at, (CTYPE)signedValue);
#define SEN_UNSIGNED(TYPE, CTYPE, HIGH, NAME) \
		case TYPE: \
			if (!ParseInteger(text, false, 0, HIGH, &signedValue, &unsignedValue)) \
				return B_BAD_VALUE; \
			return append ? message->Add##NAME(name, (CTYPE)unsignedValue) : message->Replace##NAME(name, at, (CTYPE)unsignedValue);
		SEN_SIGNED(B_INT8_TYPE, int8, -128LL, 127ULL, Int8)
		SEN_SIGNED(B_INT16_TYPE, int16, -32768LL, 32767ULL, Int16)
		SEN_SIGNED(B_INT32_TYPE, int32, -2147483648LL, 2147483647ULL, Int32)
		SEN_SIGNED(B_INT64_TYPE, int64, (-9223372036854775807LL - 1), 9223372036854775807ULL, Int64)
		SEN_UNSIGNED(B_UINT8_TYPE, uint8, 255ULL, UInt8)
		SEN_UNSIGNED(B_UINT16_TYPE, uint16, 65535ULL, UInt16)
		SEN_UNSIGNED(B_UINT32_TYPE, uint32, 4294967295ULL, UInt32)
		SEN_UNSIGNED(B_UINT64_TYPE, uint64, 18446744073709551615ULL, UInt64)
#undef SEN_SIGNED
#undef SEN_UNSIGNED
	}
	return B_BAD_TYPE;
}


template<typename Number>
std::string
NumberText(Number value)
{
	char buffer[64];
	auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
	return std::string(buffer, result.ptr);
}

}	// namespace detail


/** Whether the path names a value of the message. */
inline bool
Has(const BMessage& message, const std::string& path)
{
	detail::Located located;
	if (detail::Locate(message, path, &located) != B_OK)
		return false;
	return located.step.index < located.step.count;
}


/** The type of the field and the number of its values (an array has more than one). B_NAME_NOT_FOUND if there is no such field. */
inline status_t
FieldOf(const BMessage& message, const std::string& path, type_code* type, size_t* count)
{
	detail::Located located;
	status_t result = detail::Locate(message, path, &located);
	if (result != B_OK)
		return result;
	*type = located.type;
	*count = located.step.count;
	return B_OK;
}


/** The value as the text that a control shows. B_BAD_INDEX if the index is outside the values, B_BAD_TYPE for a field that no text stands for. */
inline status_t
GetText(const BMessage& message, const std::string& path, std::string* text)
{
	detail::Located located;
	status_t result = detail::Locate(message, path, &located);
	if (result != B_OK)
		return result;
	if (located.step.index >= located.step.count)
		return B_BAD_INDEX;
	if (!detail::IsText(located.type))
		return B_BAD_TYPE;

	const char* name = located.step.name.c_str();
	const int32 at = (int32)located.step.index;
	const BMessage& m = located.container;
	switch (located.type) {
		case B_BOOL_TYPE: *text = m.GetBool(name, at, false) ? "true" : "false"; return B_OK;
		case B_STRING_TYPE: *text = m.GetString(name, at, ""); return B_OK;
		case B_INT8_TYPE: *text = detail::NumberText((int)m.GetInt8(name, at, 0)); return B_OK;
		case B_INT16_TYPE: *text = detail::NumberText(m.GetInt16(name, at, 0)); return B_OK;
		case B_INT32_TYPE: *text = detail::NumberText(m.GetInt32(name, at, 0)); return B_OK;
		case B_INT64_TYPE: *text = detail::NumberText(m.GetInt64(name, at, 0)); return B_OK;
		case B_UINT8_TYPE: *text = detail::NumberText((unsigned)m.GetUInt8(name, at, 0)); return B_OK;
		case B_UINT16_TYPE: *text = detail::NumberText(m.GetUInt16(name, at, 0)); return B_OK;
		case B_UINT32_TYPE: *text = detail::NumberText(m.GetUInt32(name, at, 0)); return B_OK;
		case B_UINT64_TYPE: *text = detail::NumberText(m.GetUInt64(name, at, 0)); return B_OK;
		case B_FLOAT_TYPE: *text = detail::NumberText(m.GetFloat(name, at, 0.0f)); return B_OK;
		case B_DOUBLE_TYPE: *text = detail::NumberText(m.GetDouble(name, at, 0.0)); return B_OK;
		case B_TIME_TYPE: {
			const void* data = NULL;
			ssize_t size = 0;
			if (m.FindData(name, B_TIME_TYPE, at, &data, &size) != B_OK || size != (ssize_t)sizeof(time_t))
				return B_BAD_VALUE;
			time_t value;
			memcpy(&value, data, sizeof(value));
			*text = FormatTime((int64_t)value);
			return B_OK;
		}
	}
	return B_BAD_TYPE;
}


/**
 * Sets the value that the path names to what the text says, in the type of the field. If the index is the number of the values, the value is appended (the
 * path `tags_03` of a field with three values). The message is not changed if this fails: B_BAD_VALUE (the text does not fit the type), B_BAD_TYPE,
 * B_NAME_NOT_FOUND, B_BAD_INDEX.
 */
inline status_t
SetText(BMessage* message, const std::string& path, const std::string& text)
{
	return detail::ModifyPath(message, path, [&text](BMessage* container, const Step& step, type_code type) {
		return detail::StoreText(container, step.name.c_str(), type, step.index, step.count, text);
	});
}


/** Appends a value to the field that the path names (`tags` or `nested.tags`, with or without an index: it goes after the last value). */
inline status_t
Append(BMessage* message, const std::string& path, const std::string& text)
{
	return detail::ModifyPath(message, path, [&text](BMessage* container, const Step& step, type_code type) {
		return detail::StoreText(container, step.name.c_str(), type, step.count, step.count, text);
	});
}


/** Removes the value that the path names; a field that has no value left is removed with it. */
inline status_t
Remove(BMessage* message, const std::string& path)
{
	return detail::ModifyPath(message, path, [](BMessage* container, const Step& step, type_code) {
		if (step.index >= step.count)
			return (status_t)B_BAD_INDEX;
		return container->RemoveData(step.name.c_str(), (int32)step.index);
	});
}


/**
 * Sets all values of a field to the texts (an array that is edited as a whole): the values that are there are replaced in place, the others appended, the
 * surplus ones removed. `parent` is the path of the nested message that has the field (empty: the message itself). The field is made, in `type`, if it is not there
 * (an array that was emptied has no field left), and it goes with its last value if there are no texts. The message is not changed if this fails:
 * B_BAD_VALUE (a text does not fit the type), B_BAD_TYPE (not a type of text, or the field has another one), and the errors of the path.
 */
inline status_t
SetValues(BMessage* message, const std::string& parent, const std::string& name, type_code type, const std::vector<std::string>& texts)
{
	if (!detail::IsText(type))
		return B_BAD_TYPE;

	auto apply = [&](BMessage* target) -> status_t {
		type_code have = 0;
		int32 count = 0;
		if (target->GetInfo(name.c_str(), &have, &count) != B_OK)
			count = 0;
		else if (have != type)
			return B_BAD_TYPE;

		const size_t existing = (size_t)count;
		for (size_t i = 0; i < texts.size(); i++) {
			status_t result = detail::StoreText(target, name.c_str(), type, i, i < existing ? existing : i, texts[i]);
			if (result != B_OK)
				return result;
		}
		for (size_t i = existing; i > texts.size(); i--) {
			status_t result = target->RemoveData(name.c_str(), (int32)(i - 1));
			if (result != B_OK)
				return result;
		}
		return B_OK;
	};

	if (parent.empty()) {
		BMessage copy(*message);
		status_t result = apply(&copy);
		if (result == B_OK)
			*message = copy;
		return result;
	}

	return detail::ModifyPath(message, parent, [&apply](BMessage* container, const Step& step, type_code stepType) -> status_t {
		if (stepType != B_MESSAGE_TYPE)
			return B_BAD_TYPE;
		if (step.index >= step.count)
			return B_BAD_INDEX;
		BMessage child;
		status_t result = container->FindMessage(step.name.c_str(), (int32)step.index, &child);
		if (result == B_OK)
			result = apply(&child);
		if (result == B_OK)
			result = container->ReplaceMessage(step.name.c_str(), (int32)step.index, &child);
		return result;
	});
}


/** One value of a message, as the form lists them. */
struct Leaf {
	std::string	path;				///< the path key; empty if the value cannot be addressed (see SegmentFor)
	std::string	name;				///< the name of the field
	type_code	type = 0;
	size_t		index = 0;			///< of the value in its field
	size_t		count = 1;			///< the values of the field (more than one: an array)
	int			depth = 0;			///< of the nesting, 0 for the fields of the message itself
	bool		addressable = true;
};


/**
 * Lists the values of a message in the order of its fields, each value of an array on its own; a nested message is listed (type B_MESSAGE_TYPE) before
 * the values in it. The paths are those that the other functions take.
 */
inline void
Enumerate(const BMessage& message, std::vector<Leaf>* leaves, const std::string& prefix = std::string(), int depth = 0)
{
	const FieldInfo info = detail::InfoOf(message);
	char* fieldName = NULL;
	type_code type = 0;
	int32 count = 0;
	for (int32 i = 0; message.GetInfo(B_ANY_TYPE, i, &fieldName, &type, &count) == B_OK; i++) {
		const std::string name = fieldName;
		for (int32 index = 0; index < count; index++) {
			Leaf leaf;
			leaf.name = name;
			leaf.type = type;
			leaf.index = (size_t)index;
			leaf.count = (size_t)count;
			leaf.depth = depth;
			const std::string segment = SegmentFor(name, (size_t)index, (size_t)count, info);
			leaf.addressable = !segment.empty();
			if (leaf.addressable)
				leaf.path = ChildPath(prefix, segment);
			leaves->push_back(leaf);

			if (type == B_MESSAGE_TYPE && leaf.addressable) {
				BMessage child;
				if (message.FindMessage(name.c_str(), index, &child) == B_OK)
					Enumerate(child, leaves, leaf.path, depth + 1);
			}
		}
	}
}

}	// namespace settings
}	// namespace sen
