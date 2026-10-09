/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024-2026 SEN Labs e.U.
 */
#pragma once

#include <stdint.h>

/**
 * @file SenMessages.h
 * @brief The message protocol of the SEN server: commands, message fields, relation config keys and status codes.
 *
 * Requests are BMessages whose `what` is a command of sen::cmd and whose fields are named by sen::key. SEN fields are
 * `SEN:camelCase`; the three fields of the reply envelope (`status`, `detail`, `apiVersion`) and `refs` are the exception
 * and read like the usual Haiku fields.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"

namespace sen {

/** Version of the message protocol, reported in every reply as sen::key::kApiVersion. */
inline constexpr int32_t kApiVersion = 1;

namespace cmd {

// ---- core
inline constexpr uint32_t kCoreInfo   = 'SCin';
inline constexpr uint32_t kCoreStatus = 'SCst';
/** validate and repair the configuration and the relations of a file */
inline constexpr uint32_t kCoreCheck  = 'SCck';
/** some sanity testing */
inline constexpr uint32_t kCoreTest   = 'SCts';
/** find the entry_ref for a `SEN:ID` */
inline constexpr uint32_t kQueryRefForId = 'SQri';
/** find the `SEN:ID` for an entry_ref */
inline constexpr uint32_t kQueryIdForRef = 'SQir';

/** `what` of the messages that hold the flavor of a relation direction in resource files (see sen::conf). */
inline constexpr uint32_t kRelationDefinition = 'SCrd';

// ---- configuration and classification
/** returns the static configuration as a message, read-only for now */
inline constexpr uint32_t kConfigGet = 'SCge';
/** add a classification entity (fields: context, type, name) */
inline constexpr uint32_t kClassificationAdd  = 'SZad';
/** get a classification by context and name */
inline constexpr uint32_t kClassificationGet  = 'SZge';
/** find classifications, optionally filtered by context(s), type(s) and/or names */
inline constexpr uint32_t kClassificationFind = 'SZfi';

// ---- relations
inline constexpr uint32_t kRelationsGet                 = 'SRge';
inline constexpr uint32_t kRelationsGetAll              = 'SRga';
/** contained (self) relations of one type */
inline constexpr uint32_t kRelationsGetSelf             = 'SRsg';
inline constexpr uint32_t kRelationsGetAllSelf          = 'SRsa';
inline constexpr uint32_t kRelationsGetCompatible       = 'SRgc';
inline constexpr uint32_t kRelationsGetCompatibleTypes  = 'SRgt';
inline constexpr uint32_t kRelationsGetNewTarget        = 'SRgn';
inline constexpr uint32_t kRelationAdd                  = 'SRad';
inline constexpr uint32_t kRelationRemove               = 'SRrm';
/** change the properties, target or type of an existing relation */
inline constexpr uint32_t kRelationUpdate               = 'SRup';
inline constexpr uint32_t kRelationsRemoveAll           = 'SRra';

// ---- Tracker integration
inline constexpr uint32_t kOpenRelationTarget     = 'STot';
inline constexpr uint32_t kOpenRelationView       = 'STrv';
inline constexpr uint32_t kOpenRelationTargetView = 'STtv';

// ---- applications that manage the types (our FileTypes, see sen::kFileTypesSignature)
/** Open an attribute of a MIME type: the type that has it (kMimeType) and the name of the attribute (kAttributeName). The application
 *  decides what to do: the one of SEN selects the type, closes the dialog of any other attribute and opens one for this. */
inline constexpr uint32_t kOpenMimeAttribute      = 'SOma';

// ---- replies
inline constexpr uint32_t kReplyInfo      = 'SCri';
inline constexpr uint32_t kReplyStatus    = 'SCrs';
inline constexpr uint32_t kReplyRelations = 'SCre';

}	// namespace cmd

namespace key {

// ---- reply envelope: in every reply, whatever the request
/** int32: a SEN status code, see sen::status */
inline constexpr char kStatus[]     = "status";
/** string: what happened, for people and logs */
inline constexpr char kDetail[]     = "detail";
/** int32: sen::kApiVersion of the server */
inline constexpr char kApiVersion[] = "apiVersion";
/** int32: the technical `status_t` of the operation (what `strerror()` understands) */
inline constexpr char kResult[]     = "result";

// ---- common request fields
/** uint32 command to perform, sent by Tracker */
inline constexpr char kAction[]         = "SEN:action";
inline constexpr char kFilter[]         = "SEN:filter";
inline constexpr char kContext[]        = "SEN:context";
inline constexpr char kName[]           = "SEN:name";
inline constexpr char kType[]           = "SEN:type";
inline constexpr char kCount[]          = "SEN:count";
/** bool: also return the properties of relations */
inline constexpr char kWithProperties[] = "SEN:withProperties";
/** bool: also return the configuration of relations */
inline constexpr char kWithConfigs[]    = "SEN:withConfigs";
inline constexpr char kIncludeTypes[]   = "SEN:includeTypes";
inline constexpr char kExcludeTypes[]   = "SEN:excludeTypes";

// ---- configuration paths in the config message
inline constexpr char kConfigPath[]            = "SEN:configPath";
inline constexpr char kClassificationPath[]    = "SEN:classificationPath";
inline constexpr char kClassificationPathRef[] = "SEN:classificationPathRef";
inline constexpr char kContextPath[]           = "SEN:contextPath";
inline constexpr char kContextPathRef[]        = "SEN:contextPathRef";

// ---- relations
/** the relations of a source: relation type names in a request, nested relation messages in a result */
inline constexpr char kRelations[]          = "SEN:relations";
/** short name of the relation type */
inline constexpr char kRelationName[]       = "SEN:relationName";
/** label used for a particular relation */
inline constexpr char kRelationLabel[]      = "SEN:relationLabel";
/** unique relation MIME type */
inline constexpr char kRelationType[]       = "SEN:relationType";
/** relation root (pointer), used in nested relations (self or n-ary) */
inline constexpr char kRelationRoot[]       = "SEN:relationRoot";
/** properties of one relation */
inline constexpr char kRelationProperties[] = "SEN:relationProperties";
/**
 * identifier of a single relation (a TSID). It is named like an attribute of a relation file because the properties of a
 * relation are shown as attributes of the file, and edited there. Only present where several property sets of one type exist between the same
 * two files; it is the same for a relation and its opposite direction.
 */
inline constexpr char kRelationId[]         = "SEN:REL:relationId";
/** in the properties of a relation: bool, the target does not exist (any more); a dangling relation */
inline constexpr char kTargetMissing[]      = "SEN:REL:missing";
/** remove: bool, remove all relations of the type to the target, not only one */
inline constexpr char kAllRelations[]       = "SEN:allRelations";
/** update: the new target of the relation (entry_ref), it moves to that target */
inline constexpr char kNewTargetRef[]       = "SEN:newTargetRef";
/** update: the new type of the relation, it becomes a relation of that type */
inline constexpr char kNewRelationType[]    = "SEN:newRelationType";
/** the configurations of several relation types, keyed by relation type */
inline constexpr char kRelationConfigMap[]  = "SEN:relationConfigMap";
/** the configuration of one relation type (from the MIME type of the relation) */
inline constexpr char kRelationConfig[]     = "SEN:relationConfig";
/** selected relation target in nested relations (self or n-ary) */
inline constexpr char kItemId[]             = "SEN:itemId";
/** the source of a relation as entry_ref, named the way Haiku names it */
inline constexpr char kSourceRef[]          = "refs";
inline constexpr char kSourceId[]           = "SEN:sourceId";
inline constexpr char kTargetId[]           = "SEN:targetId";
inline constexpr char kTargetRef[]          = "SEN:targetRef";
/** the type of a target, used to find compatible relations */
inline constexpr char kTargetType[]         = "SEN:targetType";
/** maps `SEN:ID` to entry_refs, sent in relation replies */
inline constexpr char kIdToRefMap[]         = "SEN:idToRef";
/** message: the name to show for each target (by SEN:ID), next to kIdToRefMap: the title of the file (dc:title), the short
 *  description of a MIME type, else the name of the file */
inline constexpr char kIdToNameMap[]        = "SEN:idToName";
/** string, with kOpenMimeAttribute: the MIME type that has the attribute */
inline constexpr char kMimeType[]           = "SEN:mimeType";
/** string, with kOpenMimeAttribute: the name of the attribute (SEN:attr:name) */
inline constexpr char kAttributeName[]      = "SEN:attr:name";
/** message, with add: properties of the opposite direction that replace the ones it gets from the relation (e.g. its own label) */
inline constexpr char kInverseProperties[]  = "SEN:inverseProperties";
/** bool, with remove, update and removeAll: also changes the relations that are read-only (for the installer of ontologies) */
inline constexpr char kOverride[]           = "SEN:override";

}	// namespace key

/** Values of sen::key::kFilter. */
namespace filter {
inline constexpr char kCompatible[] = "compatible";
}

/**
 * Keys of the configuration of a relation type: the resource `SEN:REL:CONFIG` of the MIME type of the relation, and the
 * message fields derived from it. The `SEN:relation` and `SEN:inverse` entries are messages (what `'SCrd'`) that hold the
 * label and properties of the relation and of its opposite direction.
 */
namespace conf {

/** the forward direction (label, default properties) */
inline constexpr char kRelation[]      = "SEN:relation";
/** the opposite direction; written to the target of a bidirectional relation */
inline constexpr char kInverse[]       = "SEN:inverse";
/** bool, default true: SEN writes the opposite relation to the target. Associations are unidirectional. */
inline constexpr char kBidirectional[] = "SEN:bidir";
/** bool: resolved at run time (by a plugin or a query), never stored, read-only */
inline constexpr char kDynamic[]       = "SEN:dynamic";
/** bool: reflexive; the relation lives inside the source file (shown as "contained") */
inline constexpr char kSelf[]          = "SEN:self";
/**
 * strings: the relation can start only at files of these types (without it, at any file). A type is a MIME type or the
 * start of one, e.g. a supertype ("audio"), like the filters for the templates of a new file.
 */
inline constexpr char kSourceTypes[]        = "SEN:sourceTypes";
/** strings: the relation cannot start at files of these types (same kind of types); this wins over kSourceTypes */
inline constexpr char kExcludeSourceTypes[] = "SEN:excludeSourceTypes";

}	// namespace conf

/** Names of folders below the SEN settings folder. */
namespace config {
inline constexpr char kClassificationsDir[] = "classifications";
inline constexpr char kContextsDir[]        = "contexts";
inline constexpr char kContextGlobal[]      = "global";
}

/**
 * SEN status codes, in the `status` field of every reply. Like HTTP they are telling on the domain level; the first digit
 * is the class (2 ok, 4 the request cannot be done as asked, 5 SEN or the system failed). The technical `status_t` goes
 * into `result`, the explanation into `detail`.
 */
namespace status {

/** The first digit is the class of the result: 2 ok, 4 the request cannot be done as asked, 5 SEN or the system failed. */

inline constexpr int32_t kOk        = 200;
inline constexpr int32_t kCreated   = 201;
/** nothing to return, e.g. a file without relations */
inline constexpr int32_t kNoContent = 204;

inline constexpr int32_t kErrBadRequest           = 400;
inline constexpr int32_t kErrForbidden            = 403;	///< e.g. the relation is read-only (SEN:REL:readonly)
inline constexpr int32_t kErrNotFound             = 404;
inline constexpr int32_t kErrConflict             = 409;
inline constexpr int32_t kErrRelationResolveFailed = 4001;	///< the relation could not be resolved
inline constexpr int32_t kErrRelationTargetMissing = 4002;	///< dangling: the target does not exist any more
inline constexpr int32_t kErrRelationNotFound      = 4003;
inline constexpr int32_t kErrIdNotUnique           = 4004;	///< two files carry the same `SEN:ID`
inline constexpr int32_t kErrUnknownRelationType   = 4005;
inline constexpr int32_t kErrTooManyTargets        = 4006;	///< the list of targets of a file is full (sen::idlist::kMaxIds)
inline constexpr int32_t kErrAmbiguousRelation     = 4007;	///< several relations match: name one by SEN:REL:relationId

inline constexpr int32_t kErrFailed               = 500;
inline constexpr int32_t kErrUnavailable          = 503;
inline constexpr int32_t kErrPluginFailed         = 5001;
inline constexpr int32_t kErrPluginTimeout        = 5002;
inline constexpr int32_t kErrAttributeWriteFailed = 5003;
inline constexpr int32_t kErrIndexMissing         = 5004;	///< no BFS index for an attribute that is queried

}	// namespace status
}	// namespace sen

#pragma GCC diagnostic pop
