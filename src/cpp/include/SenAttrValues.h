/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenAttrValues.h
 * @brief The common values of an attribute of a type (the kinds of an event), as the ontology gives them (header only, Haiku only).
 *
 * An attribute whose range in the schema is an enum is text that has common values (`Birthday`, `Wedding`, `Meeting`), each with what it means (a Wikidata item), and
 * the schema says whether other text is allowed (`allow_custom`, yes by default). The ontology installer keeps them in the attribute `SEN:ATTR:VALUES` of the entry of the type
 * in the MIME database, keyed by the NAME of the attribute: not in the attribute info, which FileTypes rewrites and which is read by position.
 *
 *     sen::attrvalues::Values values;
 *     if (sen::attrvalues::Load("entity/x-vnd.sen-labs.entity.event", "SEN:eventType", &values)) {
 *         // values.texts[i], values.meanings[i] (Wikidata item, may be empty), values.allowCustom
 *     }
 */

#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <FindDirectory.h>
#include <fs_attr.h>

#include <string>
#include <vector>

#include "SenAttributes.h"

namespace sen {
namespace attrvalues {

struct Values {
	std::vector<std::string>	texts;		///< what is stored
	std::vector<std::string>	meanings;	///< what each stands for (`Q47223`), as many as the texts, empty if it has none
	bool						allowCustom = true;		///< other text may be entered
	bool						allowMultiple = false;	///< several values at once (for a control that offers it; no attribute of the ontologies asks for it yet)

	bool IsEmpty() const { return texts.empty(); }
};


/** The common values of the attribute of a type from the MIME database of the user; false if the type has none for it. */
inline bool
Load(const char* mimeType, const char* attribute, Values* values)
{
	BPath path;
	if (mimeType == NULL || attribute == NULL || find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK || path.Append("mime_db") != B_OK || path.Append(mimeType) != B_OK)
		return false;
	BNode node(path.Path());
	attr_info info;
	if (node.InitCheck() != B_OK || node.GetAttrInfo(attr::kAttributeValues, &info) != B_OK || info.type != B_MESSAGE_TYPE || info.size <= 0 || info.size > 256 * 1024)
		return false;
	std::vector<char> buffer((size_t)info.size);
	if (node.ReadAttr(attr::kAttributeValues, B_MESSAGE_TYPE, 0, buffer.data(), buffer.size()) != info.size)
		return false;
	BMessage all, own;
	if (all.Unflatten(buffer.data()) != B_OK || all.FindMessage(attribute, &own) != B_OK)
		return false;

	Values found;
	const char* text;
	for (int32 i = 0; own.FindString("text", i, &text) == B_OK; i++) {
		found.texts.push_back(text);
		const char* meaning = "";
		own.FindString("meaning", i, &meaning);
		found.meanings.push_back(meaning != NULL ? meaning : "");
	}
	found.allowCustom = own.GetBool("allow_custom", true);
	found.allowMultiple = own.GetBool("allow_multiple", false);
	if (found.IsEmpty())
		return false;
	*values = found;
	return true;
}

}	// namespace attrvalues
}	// namespace sen
