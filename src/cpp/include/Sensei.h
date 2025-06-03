/**
 * @author Gregor Rosenauer <gregor.rosenauer@gmail.com>
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#pragma once

// attributes, also used as message keys
#define SENSEI_TYPE                "SENSEI:type"
#define SENSEI_INPUT_TYPES         "SENSEI:input_types"
#define SENSEI_TYPE_MAPPING        "SENSEI:type_mapping"
#define SENSEI_ATTR_MAPPING        "SENSEI:attr_mapping"

// message codes
enum {
	SENSEI_CMD_EXTRACT	= 'SEex',
	SENSEI_CMD_ENRICH	= 'SEer',
	SENSEI_CMD_IDENTIFY	= 'SEid',
	SENSEI_CMD_NAVIGATE	= 'SEnv',
};

#define SENSEI_MESSAGE_RESULT      'SErs'

// dynamic relations
#define SENSEI_PLUGIN_CONFIG_KEY   "SENSEI:plugin_config"
#define SENSEI_PLUGIN_KEY          "SENSEI:plugin"
#define SENSEI_TYPES_PLUGINS_KEY   "SENSEI:types_plugins"
#define SENSEI_DEFAULT_TYPE_KEY    "SENSEI:default_type"
#define SENSEI_DEFAULT_TYPE        "SENSEI:default"
