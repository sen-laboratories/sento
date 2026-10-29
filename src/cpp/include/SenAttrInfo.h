/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenAttrInfo.h
 * @brief What the attribute info of a MIME type says about an attribute, beyond what Haiku has (header only, Haiku only).
 *
 * `BMimeType::GetAttrInfo()` gives a message of parallel arrays: `attr:name`, `attr:type`, `attr:viewable`, `attr:editable`... SEN adds fields of its own,
 * in the namespace `SEN:` (the `attr:` one is Haiku's): `SEN:searchable`, and for the values that are lists `SEN:is_list` and `SEN:list_separator`
 * (the ontologies define them in their schema, the installer writes them). The values are split and joined with SenList.h.
 *
 *     BMessage info;
 *     BMimeType("entity/x-vnd.sen-labs.entity.book").GetAttrInfo(&info);
 *     sen::ListConfig config = sen::ListConfigOf(info, "dc:creator");      // config.isList, config.separator: ';'
 */

#include <Message.h>

#include <string.h>

#include "SenAttributes.h"
#include "SenList.h"

namespace sen {

/** Whether the value of an attribute is a list, and what separates its values. */
struct ListConfig {
	ListConfig() : isList(false), separator(';') {}

	bool	isList;
	char	separator;	///< meaningful only for a list
};


/** The configuration of the attribute at an index of the attribute info. A list needs a usable separator: one character that is no quote and no backslash. */
inline ListConfig
ListConfigAt(const BMessage& attrInfo, int32 index)
{
	ListConfig config;
	bool isList = false;
	const char* separator = NULL;
	attrInfo.FindBool(attr::kAttrInfoIsList, index, &isList);
	attrInfo.FindString(attr::kAttrInfoListSeparator, index, &separator);

	if (isList && separator != NULL && separator[0] != '\0' && separator[1] == '\0' && list::IsValidSeparator(separator[0])) {
		config.isList = true;
		config.separator = separator[0];
	}
	return config;
}


/** The configuration of an attribute by its name: not a list if the attribute info does not have it. */
inline ListConfig
ListConfigOf(const BMessage& attrInfo, const char* attribute)
{
	const char* name = NULL;
	for (int32 i = 0; attrInfo.FindString("attr:name", i, &name) == B_OK; i++) {
		if (name != NULL && strcmp(name, attribute) == 0)
			return ListConfigAt(attrInfo, i);
	}
	return ListConfig();
}

}	// namespace sen
