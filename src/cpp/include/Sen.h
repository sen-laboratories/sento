/**
 * @author Gregor Rosenauer <gregor.rosenauer@gmail.com>
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#pragma once

#include <stdio.h>

#include <Entry.h>
#include <String.h>

#define SEN_SERVER_SIGNATURE         "application/x-vnd.sen-labs.sen-server"

#define SEN_CONTEXT_TYPE             "association/x-vnd.sen-labs.entity.context"

/**
 * semantic type of a file, e.g. text/scientific-paper vs. application/pdf
 */
 //todo: use META:TYPE ?
#define SEN_TYPE                     "SEN:TYPE"

/** used as a display name, e.g. when the folder name should be unique but is too cumbersome to read,
  * also useful for translation. */
#define META_FOLDER_NAME             "META:FOLDER_NAME"

/** used for relation folders to display a matching icon */
#define SEN_RELATION_FOLDER_ICON     "SEN:ICON:FOLDER"

/** used to denote a relation folder, which is a specialized virtual folder for relations */
#define SEN_RELATION_FOLDER_TYPE     "application/x-vnd.sen-labs.sen-relation-folder"

// simple logging, todo: integrate simple but more standard logging
#define LOG(x...)                    printf(x);
#define ERROR(x...)                  fprintf(stderr, x);

//
// message commands CORE
//

#define SEN_CORE_INFO               'SCin'
#define SEN_CORE_STATUS             'SCst'
// validate and repair config
#define SEN_CORE_CHECK              'SCck'
// some sanity testing
#define SEN_CORE_TEST               'SCts'
// core functionality
#define SEN_QUERY_ID                'SCqi'
// used for SEN relation definitions in resource files
#define SEN_RELATION_DEFINITION     'SCrd'

// Actions for communication with clients
#define SEN_ACTION_CMD              "SEN:action"

//
// Configuration
//

// returns static configuration as BMessage, read-only for now
#define SEN_CONFIG_GET                      'SCge'

// *** classification

// fields, mainly for internal use: classifications path and ref
#define SEN_CONFIG_PATH                     "senConfigPath"
#define SEN_CONFIG_CLASS_BASE_PATH          "classificationPath"
#define SEN_CONFIG_CLASS_BASE_PATH_REF      "classificationPathRef"
#define SEN_CONFIG_CLASS_PATH_NAME          "classifications"

// common classification message fields "context", "type" and "name"
#define SEN_CONFIG_CLASS_CONTEXT            "context"
#define SEN_CONFIG_CLASS_TYPE               "type"
#define SEN_CONFIG_CLASS_NAME               "name"

// add a classification entity with fields below
#define SEN_CONFIG_CLASS_ADD                'SZad'

// get classification by context and name (message with type/refs map)
#define SEN_CONFIG_CLASS_GET                'SZge'

// find classifications (message with type/refs map), optionally filtered for context(s, type(s) and/or names
#define SEN_CONFIG_CLASS_FIND               'SZfi'

// classification context specific
#define SEN_CONFIG_CONTEXT_BASE_PATH        "contextPath"
#define SEN_CONFIG_CONTEXT_BASE_PATH_REF    "contextPathRef"
#define SEN_CONFIG_CONTEXT_PATH_NAME        "contexts"
#define SEN_CONFIG_CONTEXT_GLOBAL           "global"

// message commands RELATIONS
// todo: aggregate into RELATIONS_GET/ADD/REMOVE with parameters for specialisation and filtering, see filters
#define SEN_RELATIONS_GET                   'SRge'
#define SEN_RELATIONS_GET_ALL               'SRga'
#define SEN_RELATIONS_GET_SELF              'SRsg'
#define SEN_RELATIONS_GET_ALL_SELF          'SRsa'
#define SEN_RELATIONS_GET_COMPATIBLE        'SRgc'
#define SEN_RELATIONS_GET_COMPATIBLE_TYPES  'SRgt'
#define SEN_RELATIONS_GET_NEW_TARGET        'SRgn'

// filters
#define SEN_MSG_FILTER                      "filter"
#define SEN_MSG_FILTER_COMPATIBLE           "compatible"

// common specification fields
#define SEN_MSG_CONTEXT                     "context"
#define SEN_MSG_NAME                        "name"
#define SEN_MSG_TYPE                        "type"
#define SEN_MSG_COUNT                       "count"

// get flat or deep result together with properties? (currently used for relations only)
#define SEN_MSG_PROPERTIES                  "withProps"
// get result with configurations? (currently used for relations only)
#define SEN_MSG_CONFIGS                     "withConfigs"

#define SEN_INCLUDE_TYPES                   "mimeIncludes"
#define SEN_EXCLUDE_TYPES                   "mimeExcludes"

#define SEN_RELATION_ADD                    'SRad'
#define SEN_RELATION_REMOVE                 'SRrm'
#define SEN_RELATIONS_REMOVE_ALL            'SRra'

// Tracker integration
#define SEN_OPEN_RELATION_TARGET            'STot'
#define SEN_OPEN_RELATION_VIEW              'STrv'
#define SEN_OPEN_RELATION_TARGET_VIEW       'STtv'

// Message Fields and other markers

// Relations

// used in file types
#define SEN_CLASS_SUPERTYPE                 "classification"
#define SEN_ENTITY_SUPERTYPE                "entity"
#define SEN_META_SUPERTYPE                  "meta"
#define SEN_RELATION_SUPERTYPE              "relation"

// used for relations in messages
#define SEN_RELATIONS               "SEN:relations"

// short name of the relation type
#define SEN_RELATION_NAME           "SEN:relationName"

// label used for a particular relation
#define SEN_RELATION_LABEL          "SEN:relationLabel"

// relation label attribute
#define SEN_RELATION_LABEL_ATTR     "SEN:REL:Label"

// unique relation MIME type
#define SEN_RELATION_TYPE           "SEN:relationType"

// relation root (ptr), used in nested relations (self or n-ary)
#define SEN_RELATION_ROOT           "SEN:relationRoot"

// selected relation target (nested item nodes, xpath style), used in nested relations (self or n-ary)
#define SEN_RELATION_ITEM_ID        "SEN:relationItemId"

// msg source ref using common name
#define SEN_RELATION_SOURCE_ID       "SEN:sourceId"
#define SEN_RELATION_SOURCE_REF      "refs"
#define SEN_RELATION_SOURCE_REF_ATTR "SEN:REL:SRC"

#define SEN_RELATION_TARGET_ID       "SEN:targetId"
#define SEN_RELATION_TARGET_REF      "SEN:targetRef"
#define SEN_RELATION_TARGET_REF_ATTR "SEN:REL:TRG"

// used in relation target views as target (e.g. file) name
#define SEN_RELATION_TARGET_LABEL   "SEN:targetLabel"

// used to find compatible relations / types
#define SEN_RELATION_TARGET_TYPE    "SEN:targetType"

// maps SEN:ID to entry_refs, sent in relation replies
#define SEN_ID_TO_REF_MAP           "SEN:id_to_ref"

#define SEN_RELATION_PROPERTIES     "SEN:relationProps"

// relation configuration from the Relations's MIME type,
#define SEN_RELATION_CONFIG         "SEN:relationConfig"

// resource ID and fs attribute for relation configuration
#define SEN_RELATION_CONFIG_ATTR    "SEN:REL:CONFIG"

// define properties for the inverse relation from target back to src (e.g. a different Label) if bidirectional
#define SEN_RELATION_CONFIG_INVERSE "SEN:inverse"

// is the relation is bidirectional? If so, SEN will create the opposite relation automatically by default.
#define SEN_RELATION_IS_BIDIR       "SEN:bidir"

// relation properties used at runtime (e.g. as returned by plugins)
/** relation was created on-the-fly and is not persisted. */
#define SEN_RELATION_IS_DYNAMIC     "SEN:dynamic"

/** indicates a self referencing relation. */
#define SEN_RELATION_IS_SELF        "SEN:self"

/** relation used for classification and context */
#define SEN_ASSOC_RELATION_TYPE     SEN_RELATION_SUPERTYPE "/x-vnd.sen-labs.relation.association"

//
// common message fields used in requests/replies
//

// message field name for reverse relation label
#define SEN_INVERSE_RELATION_LABEL    "SEN:REL:Label"   // same as SEN_RELATION_LABEL_ATTR but different use
#define SEN_RELATION_COMPATIBLE_TYPES "SEN:types"

// standard length of a TSID = 14 + NULL byte, see IceDustGenerator
#define SEN_ID_LEN                  15

// used on any linked file
#define SEN_ATTR_PREFIX             "SEN:"
#define SEN_ID_ATTR                 SEN_ATTR_PREFIX "ID"
#define SEN_TO_ATTR                 SEN_ATTR_PREFIX "TO"
#define SEN_RELATION_ATTR_PREFIX    SEN_ATTR_PREFIX "REL:"
#define SEN_RELATION_ATTR_PREFIX_LEN 8

// self-relation pseudo ID
#define SEN_ID_SELF                 "_self"
#define SEN_ID_SELF_LEN             5

// used for ad-hoc created relation files pointing to the source and target of the relation
#define SEN_RELATION_SOURCE_ATTR    SEN_RELATION_ATTR_PREFIX "ID"
#define SEN_RELATION_TARGET_ATTR    SEN_RELATION_ATTR_PREFIX "TO"

// Message Replies
#define SEN_RESULT_INFO             'SCri'
#define SEN_RESULT_STATUS           'SCrs'
#define SEN_RESULT_RELATIONS        'SCre'

// simple data exchange object needed for creating and populating the relation view directory
struct RelationInfo {
	entry_ref srcRef;
	entry_ref targetRef;
	entry_ref relationDirRef;

	BString relationType;
	BString relationLabel;
	BString srcId;
	BString targetId;
};
