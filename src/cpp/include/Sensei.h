/**
 * @author Gregor Rosenauer <gregor.rosenauer@gmail.com>
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#pragma once

// attributes, also used as message keys
#define SENSEI_ATTR_PREFIX         "SENSEI:"
#define SENSEI_INPUT_TYPES         SENSEI_ATTR_PREFIX "input_types"
#define SENSEI_TYPE_MAPPING        SENSEI_ATTR_PREFIX "type_mapping"
#define SENSEI_ATTR_MAPPING        SENSEI_ATTR_PREFIX "attr_mapping"

// message codes
enum SENSEI_CMD {
	SENSEI_CMD_EXTRACT	= 'SEex',
	SENSEI_CMD_ENRICH	  = 'SEer',
	SENSEI_CMD_IDENTIFY	= 'SEid',
	SENSEI_CMD_NAVIGATE	= 'SEnv',
};

// features
#define SENSEI_PLUGIN_FEATURE_ATTR "SEN:plugin"
#define SENSEI_FEATURE_SEARCH      "search"
#define SENSEI_FEATURE_EXTRACT     "extract"
#define SENSEI_FEATURE_NAVIGATE    "navigate"
#define SENSEI_FEATURE_ENRICH      "enrich"
#define SENSEI_FEATURE_IDENTIFY    "identify"

// result message
#define SENSEI_MESSAGE_RESULT      'SErs'
// reserved internal / common parameters
#define SENSEI_ITEM                "_item"
#define SENSEI_LABEL               "_label"
#define SENSEI_TYPE                "_type"
// optional custom path reflecting the structure (e.g. used in Tracker for relation folder of self relations)
#define SENSEI_PATH                "_path"

// SENSEI meta type for all SEN plugins
#define SENSEI_PLUGIN_TYPE         "application/x-vnd.sen-labs.plugin"

// dynamic relations
#define SENSEI_PLUGIN_CONFIG_KEY   SENSEI_ATTR_PREFIX "plugin_config"
#define SENSEI_PLUGIN_KEY          SENSEI_ATTR_PREFIX "plugin"
#define SENSEI_TYPES_PLUGINS_KEY   SENSEI_ATTR_PREFIX "types_plugins"
#define SENSEI_DEFAULT_TYPE_KEY    SENSEI_ATTR_PREFIX "default_type"
#define SENSEI_DEFAULT_TYPE        SENSEI_ATTR_PREFIX "default"
