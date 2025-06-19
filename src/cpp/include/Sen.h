/**
 * @author Gregor Rosenauer <gregor.rosenauer@gmail.com>
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#pragma once

#include <stdio.h>

#define SEN_SERVER_SIGNATURE "application/x-vnd.sen-labs.sen-server"

// simple logging, todo: integrate simple but more standard logging
#define LOG(x...)		printf(x);
#define ERROR(x...)		fprintf(stderr, x);

// core

#define SEN_CORE_INFO 				'SCin'
#define SEN_CORE_STATUS 			'SCst'
// validate and repair config
#define SEN_CORE_CHECK				'SCck'
// some sanity testing
#define SEN_CORE_TEST               'SCts'
// core functionality
#define SEN_QUERY_ID 			    'SCqi'
// used for SEN relation definitions in resource files
#define SEN_RELATION_DEFINITION     'SCrd'

// Actions for communication with clients
#define SEN_ACTION_CMD		        "SEN:action"

// relations

// used in file types
#define SEN_ENTITY_SUPERTYPE        "entity"
#define SEN_META_SUPERTYPE          "meta"
#define SEN_RELATION_SUPERTYPE      "relation"

// relation source path as TEXT - only use for external clients that cannot send a native ref.
#define SEN_RELATION_SOURCE         "SEN:source"
// relation source entry_ref
#define SEN_RELATION_SOURCE_REF     "SEN:sourceRef"

#define SEN_RELATION_SOURCE_ID      "SEN:sourceId"
#define SEN_RELATION_TARGET_ID      "SEN:targetId"

// used for relations in messages
#define SEN_RELATIONS               "SEN:relations"

// short name of the relation type
#define SEN_RELATION_NAME           "SEN:relation"

// label used for a particular relation
#define SEN_RELATION_LABEL          "SEN:relationLabel"

// unique relation MIME type
#define SEN_RELATION_TYPE           "SEN:relationType"

// relation target. see notes on SEN_RELATION_SOURCE
#define SEN_RELATION_TARGET         "SEN:target"
#define SEN_RELATION_TARGET_REF     "SEN:targetRef"

#define SEN_RELATION_PROPERTIES     "SEN:relationProps"

//
// relation config properties declared in a BMessage using SEN Relation properties.
// used to declare defaults and restrictions in file type definitions (see sen-oni)
//
#define SEN_RELATION_CONFIG			"sen:relation"
// define properties for the reverse relation (e.g. a different Label) if bidirectional
#define SEN_RELATION_CONFIG_REVERSE "sen:reverse"

// is the relation is bidirectional? If so, SEN will create the opposite relation automatically by default.
#define SEN_RELATION_IS_BIDIR		"sen:bidir"
// is the relation allowed to point back to its origin? (e.g. NO for parent-child relations)
#define SEN_RELATION_IS_REFLEXIVE   "sen:reflexive"

// relation properties used at runtime (e.g. as returned by plugins)
#define SEN_RELATION_IS_DYNAMIC     "sen:dynamic"		// relation was created on-the-fly and is not persisted.
#define SEN_RELATION_IS_SELF 	    "sen:self"			// indicates a particular relation is self referencing.

// message field name for reverse relation label
#define SEN_INVERSE_RELATION_LABEL  "SEN:REL:Label"

// meta relations are used for classification and context
#define SEN_RELATION_IS_META        "sen:meta"

// command messages
#define SEN_RELATIONS_GET                  'SRge'
#define SEN_RELATIONS_GET_ALL              'SRga'
#define SEN_RELATIONS_GET_SELF             'SRsg'
#define SEN_RELATIONS_GET_ALL_SELF         'SRsa'
#define SEN_RELATIONS_GET_COMPATIBLE       'SRgc'
#define SEN_RELATIONS_GET_COMPATIBLE_TYPES 'SRgt'
#define SEN_RELATIONS_GET_NEW_TARGET       'SRgn'

#define SEN_RELATION_ADD	        'SRad'
#define SEN_RELATION_REMOVE	    	'SRrm'
#define SEN_RELATIONS_REMOVE_ALL    'SRra'

// Tracker integration
#define SEN_OPEN_RELATION_VIEW		    'STor'
#define SEN_OPEN_RELATION_TARGET_VIEW	'STot'
// sent when a relation is invoked to include arguments for handling plugin
#define SEN_OPEN_RELATION_ARGS_KEY	    "sen:args"

// standard length of a TSID = 14 + NULL byte, see IceDustGenerator
#define SEN_ID_LEN					15

// used on any linked file
#define SEN_ATTR_PREFIX     		"SEN:"
#define SEN_ID_ATTR        			SEN_ATTR_PREFIX "ID"
#define SEN_TO_ATTR        			SEN_ATTR_PREFIX "TO"
#define SEN_RELATION_ATTR_PREFIX    SEN_ATTR_PREFIX "REL:"
// used only for ad-hoc created relation files pointing to the target
#define SEN_RELATION_SOURCE_ATTR    SEN_RELATION_ATTR_PREFIX "ID"
#define SEN_RELATION_TARGET_ATTR    SEN_RELATION_ATTR_PREFIX "TO"
#define SEN_RELATION_META_ATTR		SEN_RELATION_ATTR_PREFIX "META"

// Message Replies
#define SEN_RESULT_INFO             'SCri'
#define SEN_RESULT_STATUS           'SCrs'
#define SEN_RESULT_RELATIONS        'SCre'

// common message fields used in requests/replies
#define SEN_MSG_TYPES               "types"
