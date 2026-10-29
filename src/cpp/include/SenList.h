/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenList.h
 * @brief Values that are lists, stored in one string of an attribute (plain C++, no Haiku API, tested on every platform in tests/SenListTest.cpp).
 *
 * Storage and representation are two things: a file keeps `Andy Hunt;Dave Thomas` in one string attribute, and a program that knows that the attribute is a
 * list (the attribute info of its MIME type says so: `SEN:is_list` and the one character `SEN:list_separator`, see SenAttrInfo.h) shows and edits the names
 * one by one. The same rules everywhere, in the style of CSV:
 *
 *   - a value is separated from the next by the separator; blanks around a value that is not quoted do not belong to it;
 *   - a value in quotes ("..." or '...') is taken as it is, so it may contain the separator or blanks at its ends: `"Hunt, Andy";"Thomas, Dave"`;
 *   - inside quotes the other kind of quote is an ordinary character (`"it's"`, `'say "hi"'`), a backslash escapes the quote that ends the value
 *     and the backslash itself (`"a \"quoted\" word"`), as in code; a backslash before anything else is an ordinary character;
 *   - empty values are not values: `a;;b` has two, as `Split("", ';')` has none;
 *   - lenient about what a person types: text after the closing quote up to the next separator is added to the value, a missing closing quote
 *     takes the rest of the text.
 *
 * Join() writes what Split() reads: a value is put in quotes only if it has to be (it contains the separator, a line break, or a quote or a blank at
 * its ends), so that `Split(Join(values, c), c) == values` for all values that are not empty, and that lists without such values look the way people
 * have always written them (`a;b;c`).
 */

#include <string>
#include <vector>

namespace sen {
namespace list {

/** A separator is one character that has no other meaning here: not a quote, a backslash, or the end of a line. */
inline bool
IsValidSeparator(char separator)
{
	return separator != '\0' && separator != '"' && separator != '\'' && separator != '\\' && separator != '\n' && separator != '\r';
}


namespace detail {

inline bool
IsBlank(char c)
{
	return c == ' ' || c == '\t';
}

inline void
TrimEnd(std::string* text)
{
	while (!text->empty() && IsBlank(text->back()))
		text->pop_back();
}

}	// namespace detail


/** The values of a list in a string; the empty ones are left out. */
inline std::vector<std::string>
Split(const std::string& text, char separator)
{
	std::vector<std::string> values;
	const size_t length = text.size();
	size_t i = 0;

	for (;;) {
		// blanks before a value (but a blank that is the separator is not one)
		while (i < length && detail::IsBlank(text[i]) && text[i] != separator)
			i++;

		std::string value;
		if (i < length && (text[i] == '"' || text[i] == '\'')) {
			const char quote = text[i++];
			while (i < length && text[i] != quote) {
				if (text[i] == '\\' && i + 1 < length && (text[i + 1] == quote || text[i + 1] == '\\'))
					i++;	// (the character after the backslash is the one that counts)
				value += text[i++];
			}
			if (i < length)
				i++;	// the closing quote
			// what a person typed after it, up to the separator, belongs to the value
			std::string tail;
			while (i < length && text[i] != separator)
				tail += text[i++];
			detail::TrimEnd(&tail);
			size_t first = 0;
			while (first < tail.size() && detail::IsBlank(tail[first]))
				first++;
			value += tail.substr(first);
		} else {
			while (i < length && text[i] != separator)
				value += text[i++];
			detail::TrimEnd(&value);
		}

		if (!value.empty())
			values.push_back(value);
		if (i >= length)
			break;
		i++;	// the separator
	}
	return values;
}


/** A value as it is written in a list: in quotes if it has to be, else as it is. An empty value stays empty. */
inline std::string
Quote(const std::string& value, char separator)
{
	if (value.empty())
		return value;

	bool needsQuotes = detail::IsBlank(value.front()) || detail::IsBlank(value.back()) || value.front() == '"' || value.front() == '\'';
	for (char c : value) {
		if (c == separator || c == '\n' || c == '\r')
			needsQuotes = true;
	}
	if (!needsQuotes)
		return value;

	// double quotes, unless the value has some and no single ones: then a quote of the value needs no escape
	const bool hasDouble = value.find('"') != std::string::npos;
	const bool hasSingle = value.find('\'') != std::string::npos;
	const char quote = (hasDouble && !hasSingle) ? '\'' : '"';

	std::string quoted(1, quote);
	for (char c : value) {
		if (c == '\\' || c == quote)
			quoted += '\\';
		quoted += c;
	}
	quoted += quote;
	return quoted;
}


/** The values as one string for an attribute; the empty ones are left out. */
inline std::string
Join(const std::vector<std::string>& values, char separator)
{
	std::string joined;
	bool first = true;
	for (const std::string& value : values) {
		if (value.empty())
			continue;
		if (!first)
			joined += separator;
		joined += Quote(value, separator);
		first = false;
	}
	return joined;
}

}	// namespace list
}	// namespace sen
