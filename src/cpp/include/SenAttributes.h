/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024-2026 SEN Labs e.U.
 */
#pragma once

#include <stddef.h>

/**
 * @file SenAttributes.h
 * @brief Names of the file system attributes (and MIME resources) that SEN reads and writes.
 *
 * These are the names of attributes of files, folders and MIME types. They follow `<prefix>:<name>`, with an established
 * vocabulary where one exists (see the developer guide, "Names: three layers"). The names of message fields are in
 * SenMessages.h, MIME types in SenMimeTypes.h.
 */
namespace sen {
namespace attr {

/** Prefix of every attribute that SEN manages. */
inline constexpr char kPrefix[] = "SEN:";

/**
 * Semantic type of a file, e.g. `document/scientific-paper`, set by identify plugins.
 * Haiku itself only knows the technical MIME type (`application/pdf`).
 */
inline constexpr char kType[] = "META:TYPE";

/** Display name of a folder when its real name is too cumbersome to read (also useful for translation). */
inline constexpr char kFolderName[] = "META:FOLDER_NAME";

// ---- identity: the only attributes that must be indexed to link the object graph

/** The unique identifier of a file, a TSID (see SenId.h). */
inline constexpr char kId[] = "SEN:ID";

/**
 * The `SEN:ID`s of the targets of the normal relations of a file, comma separated, at most
 * sen::id::kMaxPerAttribute per attribute (BFS indexes only the first 255 bytes of a string): `SEN:TO`,
 * `SEN:TO:1`, `SEN:TO:2`, ...
 */
inline constexpr char kTo[] = "SEN:TO";

/** Same for the targets of meta relations (classification and context). */
inline constexpr char kMeta[] = "SEN:META";

/** A target given as a static path: the most fragile way to name a target. */
inline constexpr char kToPath[] = "SEN:TO:PATH";

// ---- relations

/** Prefix of the attributes that hold relations (`SEN:REL:<relation type without "relation/">`) and relation properties. */
inline constexpr char kRelationPrefix[] = "SEN:REL:";
/** bool, field of the attribute info (META:ATTR_INFO) of a MIME type, one entry per attribute like the fields of Haiku: the attribute is
 *  indexed so that it can be queried. A SEN extension, hence not in the "attr:" namespace of Haiku. */
inline constexpr char kAttrInfoSearchable[] = "SEN:searchable";
/** bool, property of a relation: it cannot be changed or removed by users (the relations of the ontologies to their types) */
inline constexpr char kRelationReadOnly[] = "SEN:REL:readonly";
inline constexpr size_t kRelationPrefixLength = sizeof(kRelationPrefix) - 1;

/** In a relation file: the `SEN:ID` (or inode for dynamic relations) of the source. */
inline constexpr char kRelationSource[] = "SEN:REL:ID";
/** In a relation file: the `SEN:ID` of the target. */
inline constexpr char kRelationTarget[] = "SEN:REL:TO";
/** In a relation file: the entry_ref of the source (for sources without a `SEN:ID`). */
inline constexpr char kRelationSourceRef[] = "SEN:REL:SRC";
/** In a relation file: the entry_ref of the target. */
inline constexpr char kRelationTargetRef[] = "SEN:REL:TRG";
/** Label of a particular relation (also the file name of a relation file), and a key of the relation config. */
inline constexpr char kRelationLabel[] = "SEN:REL:Label";
/** string, property of a relation: the kind of a dependency (provides, requires, uses, ...), see DependencyKind in the core ontology */
inline constexpr char kRelationKind[]  = "SEN:REL:Kind";
/** Resource and attribute that hold the configuration of a relation type (see namespace sen::conf). */
inline constexpr char kRelationConfig[] = "SEN:REL:CONFIG";
/** Resource of a relation type with the vector icon of its relation folder. */
inline constexpr char kRelationFolderIcon[] = "SEN:ICON:FOLDER";

// ---- ontologies (written by the ontology installer)

inline constexpr char kOntologyAuthor[]      = "SEN:onto:author";
inline constexpr char kOntologySchemaUrl[]   = "SEN:onto:schema_url";
inline constexpr char kOntologyVersion[]     = "SEN:onto:version";
inline constexpr char kOntologyDescription[] = "SEN:onto:description";
inline constexpr char kOntologyStable[]      = "SEN:onto:stable";

}	// namespace attr
}	// namespace sen
