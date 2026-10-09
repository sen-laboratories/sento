/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024-2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenMimeTypes.h
 * @brief MIME types, supertypes and application signatures of SEN.
 */
namespace sen {

/** Signature of the FileTypes application of SEN (it understands sen::cmd::kOpenMimeAttribute). Stock Haiku has its own. */
inline constexpr char kFileTypesSignature[] = "application/x-vnd.sen-labs.FileTypes";
/** Signature of the FileTypes application of Haiku. */
inline constexpr char kHaikuFileTypesSignature[] = "application/x-vnd.Haiku-FileTypes";

/** Signature of the SEN server application. */
inline constexpr char kServerSignature[] = "application/x-vnd.sen-labs.sen-server";

namespace mime {

// ---- supertypes of the ontologies, and the same with the separator (for building and stripping type names)

inline constexpr char kClassificationSupertype[] = "classification";
inline constexpr char kEntitySupertype[]         = "entity";
inline constexpr char kRelationSupertype[]       = "relation";
inline constexpr char kMetaSupertype[]           = "meta";

inline constexpr char kClassificationPrefix[]    = "classification/";
inline constexpr char kEntityPrefix[]            = "entity/";
inline constexpr char kRelationPrefix[]          = "relation/";
inline constexpr char kMetaPrefix[]              = "meta/";

// ---- types

/** The generic association relation: links any file to a classification entity (label, topic, ...) or context. */
inline constexpr char kAssociationRelation[] = "relation/x-vnd.sen-labs.relation.association";
/** Generic reference to another entity, also what dropping a file on the top level of a relation view creates. */
inline constexpr char kReferenceRelation[]   = "relation/x-vnd.sen-labs.relation.reference";
/** A context: binds files to a project, domain or life area. */
inline constexpr char kContext[]             = "classification/x-vnd.sen-labs.entity.context";
/** Type of the folder that Tracker creates to show the relations of a file. */
inline constexpr char kRelationFolder[]      = "application/x-vnd.sen-labs.sen-relation-folder";
/** Type of every SEN plugin (the feature flags are attributes of the plugin file, see Sensei.h). */
inline constexpr char kPlugin[]              = "application/x-vnd.sen-labs.plugin";
/** Type of an installed ontology. */
inline constexpr char kOntology[]            = "application/x-vnd.sen-labs.ontology";

}	// namespace mime
}	// namespace sen
