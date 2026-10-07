/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024-2026 SEN Labs e.U.
 */
#pragma once

#include <stdint.h>

/**
 * @file Sensei.h
 * @brief The plugin protocol of SENSEI: features, commands, message fields.
 *
 * A plugin is an application that carries the plugin type (sen::mime::kPlugin) and one boolean attribute per supported
 * feature, `SEN:plugin:<feature>`. The server finds plugins with a BQuery for the type, the feature and the file type.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"

namespace sensei {

namespace cmd {
inline constexpr uint32_t kExtract  = 'SEex';
inline constexpr uint32_t kEnrich   = 'SEer';
inline constexpr uint32_t kIdentify = 'SEid';
inline constexpr uint32_t kNavigate = 'SEnv';
/** a plugin's reply */
inline constexpr uint32_t kResult   = 'SErs';
}

/** Prefix of the feature attributes of a plugin: `SEN:plugin:extract` and so on. */
inline constexpr char kFeatureAttrPrefix[] = "SEN:plugin";

/** The features; the attribute of a feature is kFeatureAttrPrefix + ":" + feature. */
namespace feature {
inline constexpr char kSearch[]   = "search";
inline constexpr char kExtract[]  = "extract";
inline constexpr char kNavigate[] = "navigate";
inline constexpr char kEnrich[]   = "enrich";
inline constexpr char kIdentify[] = "identify";
}

/** Command line option shared by plugins, to ask for contained (self) relations. */
inline constexpr char kOptionSelf[] = "--self";

namespace key {

// ---- plugin configuration, as attributes of the plugin and as message fields of the server
inline constexpr char kTypeMapping[]   = "SEN:typeMapping";		///< short alias to relation type
inline constexpr char kAttrMapping[]   = "SEN:attrMapping";		///< short property key to attribute name
inline constexpr char kPluginConfig[]  = "SEN:pluginConfig";
inline constexpr char kPlugin[]        = "SEN:plugin";
inline constexpr char kTypesPlugins[]  = "SEN:typesPlugins";
inline constexpr char kDefaultType[]   = "SEN:defaultType";
/** key in the type mapping that names the type used when an item has none */
inline constexpr char kDefault[]       = "SEN:default";

// ---- fields of a plugin's result. The leading underscore marks them as transient protocol fields that are never
// written to attributes.
inline constexpr char kResult[] = "result";		///< int32 status_t of the plugin
inline constexpr char kItem[]   = "_item";
inline constexpr char kLabel[]  = "_label";
inline constexpr char kName[]   = "_name";
inline constexpr char kType[]   = "_type";
/**
 * ID to uniquely label nodes for tracking (e.g. Tracker maps selected nested menu items to relation folders).
 * Set by the plugin to a custom value or to kToSelf to let SEN resolve it from the source.
 */
inline constexpr char kItemId[] = "_itemId";
/** shortcut for the relation target id: a `SEN:ID` or one of the placeholders below */
inline constexpr char kTo[]     = "_to";

}	// namespace key

/** Placeholder values of key::kTo. */
namespace to {
inline constexpr char kSelf[] = "_self";
inline constexpr char kPath[] = "_path";
}

}	// namespace sensei

#pragma GCC diagnostic pop
