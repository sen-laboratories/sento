/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

#include <string.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <Resources.h>
#include <TypeConstants.h>

/**
 * @file SenTypes.h
 * @brief The registry of the semantic types (header only, Haiku only; see docs/semantic-types.md).
 *
 * What a file *means* (a scientific paper, a person, a friendship) is an identifier in the attribute `META:TYPE`, `org.sen-labs.paper`,
 * not an entry in the MIME database, which says what the file *is* (`application/pdf`) and has two levels only. The identifiers form a tree:
 * a type *conforms to* others (a paper to a document). The types are described by the ontologies, which bring them in resource files
 * `<data>/sen/ontologies/<ontology>/types/<id>.rsrc` of their packages (the data folders of the system and the user, packaged or not);
 * the registry reads them all. It is read-only, nothing has to be installed, and a package that comes or goes changes it.
 *
 *     sen::types::Registry registry;
 *     if (registry.ConformsTo(metaType, "org.sen-labs.document")) ...
 *     BMessage attributes = registry.Attributes(metaType);   // the attribute info, as of a MIME type
 *
 * A registry is a snapshot: it is made when it is constructed (a few small files), a program that lives long makes a new one when
 * something may have changed (a package was activated).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"

namespace sen {
namespace types {

/** How a type is related to a term of another vocabulary or system (the rows of an SSSOM file). */
struct Mapping {
	std::string	predicate;		// skos:exactMatch, skos:broadMatch...
	std::string	object;			// schema:ScholarlyArticle, public.document...
	std::string	justification;	// semapv:ManualMappingCuration...
	std::string	confidence;		// "0.95" or empty
};

struct Type {
	std::string					id;				// org.sen-labs.paper
	std::string					title;			// the short name that menus show: "Scientific Paper"
	std::string					description;
	std::string					ontology;		// the name of the ontology that defines it: "papers"
	std::vector<std::string>	conformsTo;		// what it is a special case of, nearest first (the identifiers of other systems, public.document, too)
	std::vector<std::string>	formats;		// the MIME types of the files that it can be found on (may be empty)
	bool						abstract = false;
	BMessage					attributes;		// the attribute info of the type itself (what a MIME type has as META:ATTR_INFO)
	std::vector<Mapping>		mappings;
};

class Registry {
public:
	Registry() { Load(); }

	/** Reads the types of all ontologies that are installed. */
	void
	Load()
	{
		fTypes.clear();
		directory_which folders[] = {B_SYSTEM_DATA_DIRECTORY, B_USER_DATA_DIRECTORY, B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
			B_USER_NONPACKAGED_DATA_DIRECTORY};
		for (directory_which which : folders) {
			BPath path;
			if (find_directory(which, &path) != B_OK || path.Append("sen/ontologies") != B_OK)
				continue;
			BDirectory ontologies(path.Path());
			BEntry ontology;
			while (ontologies.GetNextEntry(&ontology) == B_OK) {
				BPath types(&ontology);
				if (types.Append("types") != B_OK)
					continue;
				_LoadFolder(types.Path(), ontology.IsDirectory() ? _LeafOf(&ontology) : std::string());
			}
		}
	}

	const std::vector<Type>&
	All() const
	{
		return fTypes;
	}

	/** The type with this identifier, NULL if the registry does not know it (it may be a type of another system). */
	const Type*
	Find(const std::string& id) const
	{
		for (const Type& type : fTypes) {
			if (type.id == id)
				return &type;
		}
		return NULL;
	}

	/** What the type conforms to, directly or not, nearest first, each once; the identifiers that the registry does not know are in it, too (they end the chain). */
	std::vector<std::string>
	Ancestors(const std::string& id) const
	{
		std::vector<std::string> result;
		std::set<std::string> seen = {id};
		std::vector<std::string> level = {id};
		while (!level.empty()) {
			std::vector<std::string> next;
			for (const std::string& each : level) {
				const Type* type = Find(each);
				if (type == NULL)
					continue;
				for (const std::string& parent : type->conformsTo) {
					if (seen.insert(parent).second) {
						result.push_back(parent);
						next.push_back(parent);
					}
				}
			}
			level = next;
		}
		return result;
	}

	/** Whether the type is the ancestor or conforms to it. */
	bool
	ConformsTo(const std::string& id, const std::string& ancestor) const
	{
		if (id == ancestor)
			return true;
		std::vector<std::string> ancestors = Ancestors(id);
		return std::find(ancestors.begin(), ancestors.end(), ancestor) != ancestors.end();
	}

	/** The types that conform to this one, directly or not (not the type itself). */
	std::vector<const Type*>
	Descendants(const std::string& id) const
	{
		std::vector<const Type*> result;
		for (const Type& type : fTypes) {
			if (type.id != id && ConformsTo(type.id, id))
				result.push_back(&type);
		}
		return result;
	}

	/** The types that can be found on files of this format (a MIME type). */
	std::vector<const Type*>
	ForFormat(const std::string& mimeType) const
	{
		std::vector<const Type*> result;
		for (const Type& type : fTypes) {
			if (std::find(type.formats.begin(), type.formats.end(), mimeType) != type.formats.end())
				result.push_back(&type);
		}
		return result;
	}

	/** The types of an ontology (the name of its folder: "papers"). */
	std::vector<const Type*>
	OfOntology(const std::string& ontology) const
	{
		std::vector<const Type*> result;
		for (const Type& type : fTypes) {
			if (type.ontology == ontology)
				result.push_back(&type);
		}
		return result;
	}

	/**
	 * The attribute info of a type: its own attributes and those that it inherits from what it conforms to, nearest first. An attribute that two
	 * types in the chain have is there once, as the nearest defines it. The message is what BMimeType::GetAttrInfo() gives (parallel arrays),
	 * so that the code that shows the attributes of a MIME type can show these.
	 */
	BMessage
	Attributes(const std::string& id) const
	{
		static const char* const kFields[] = {"attr:name", "attr:public_name", "attr:type", "attr:viewable", "attr:editable", "SEN:searchable",
			"attr:width", "attr:alignment", "attr:display_as"};
		BMessage result;
		std::set<std::string> names;
		std::vector<std::string> chain = {id};
		for (const std::string& ancestor : Ancestors(id))
			chain.push_back(ancestor);
		for (const std::string& each : chain) {
			const Type* type = Find(each);
			if (type == NULL)
				continue;
			const char* name;
			for (int32 index = 0; type->attributes.FindString("attr:name", index, &name) == B_OK; index++) {
				if (!names.insert(name).second)
					continue;
				for (const char* field : kFields)
					_CopyField(type->attributes, field, index, &result);
			}
		}
		result.AddString("type", id.c_str());
		return result;
	}

private:
	static void
	_CopyField(const BMessage& from, const char* field, int32 index, BMessage* to)
	{
		type_code type;
		if (from.GetInfo(field, &type) != B_OK)
			return;
		const void* data;
		ssize_t size;
		if (from.FindData(field, type, index, &data, &size) == B_OK)
			to->AddData(field, type, data, size, type != B_STRING_TYPE);
	}

	static std::string
	_LeafOf(BEntry* entry)
	{
		char name[B_FILE_NAME_LENGTH];
		return entry->GetName(name) == B_OK ? name : "";
	}

	static bool
	_String(BResources& resources, const char* name, std::string* value, type_code type = B_STRING_TYPE)
	{
		size_t size;
		const void* data = resources.LoadResource(type, name, &size);
		if (data == NULL)
			return false;
		*value = std::string((const char*) data, strnlen((const char*) data, size));
		return true;
	}

	static bool
	_Message(BResources& resources, const char* name, BMessage* message)
	{
		size_t size;
		const void* data = resources.LoadResource(B_MESSAGE_TYPE, name, &size);
		return data != NULL && message->Unflatten((const char*) data) == B_OK;
	}

	void
	_LoadFolder(const char* folder, const std::string& ontology)
	{
		BDirectory directory(folder);
		BEntry entry;
		while (directory.GetNextEntry(&entry) == B_OK) {
			BPath path(&entry);
			if (!entry.IsFile() || path.Path() == NULL || strstr(path.Path(), ".rsrc") == NULL)
				continue;
			BFile file(path.Path(), B_READ_ONLY);
			BResources resources;
			if (file.InitCheck() != B_OK || resources.SetTo(&file) != B_OK)
				continue;

			Type type;
			if (!_String(resources, "SEN:TYPE:ID", &type.id) || Find(type.id) != NULL)
				continue;		// (not a type, or one that a folder before this one has: the first wins)
			// (the descriptions have the types of those of a MIME type: 'MSDC' and 'MLDC')
			_String(resources, "META:S:DESC", &type.title, 'MSDC');
			_String(resources, "META:L:DESC", &type.description, 'MLDC');
			type.ontology = ontology;

			size_t size;
			const void* flag = resources.LoadResource(B_BOOL_TYPE, "SEN:TYPE:ABSTRACT", &size);
			type.abstract = flag != NULL && size > 0 && *(const char*) flag != 0;

			BMessage message;
			const char* each;
			if (_Message(resources, "SEN:TYPE:CONFORMS", &message)) {
				for (int32 i = 0; message.FindString("conformsTo", i, &each) == B_OK; i++)
					type.conformsTo.push_back(each);
			}
			message.MakeEmpty();
			if (_Message(resources, "SEN:TYPE:FORMATS", &message)) {
				for (int32 i = 0; message.FindString("format", i, &each) == B_OK; i++)
					type.formats.push_back(each);
			}
			message.MakeEmpty();
			if (_Message(resources, "SEN:TYPE:MAPPINGS", &message)) {
				const char* predicate;
				for (int32 i = 0; message.FindString("predicate", i, &predicate) == B_OK; i++) {
					Mapping mapping;
					mapping.predicate = predicate;
					mapping.object = message.GetString("object", i, "");
					mapping.justification = message.GetString("justification", i, "");
					mapping.confidence = message.GetString("confidence", i, "");
					type.mappings.push_back(mapping);
				}
			}
			_Message(resources, "META:ATTR_INFO", &type.attributes);
			fTypes.push_back(type);
		}
	}

	std::vector<Type>	fTypes;
};

}	// namespace types
}	// namespace sen

#pragma GCC diagnostic pop
