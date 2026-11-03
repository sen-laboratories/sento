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
 * feature, `SEN:plugin:<feature>`.
 *
 * The signature of a plugin starts with kSignaturePrefix, followed by the vendor and the name of the plugin, separated by dots:
 * `application/x-vnd.sen-plugin.sen-labs.pdf-extractor`. The server finds plugins with a query on `BEOS:APP_SIG` for
 * kSignaturePattern: that attribute is indexed on every volume, also on those of installed packages (packagefs can only
 * query a few attributes, and `META:TYPE` is not among them). The candidates are then checked by their plugin type and by the
 * feature attributes.
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

// ---- the settings of a plugin (see docs/spec-settings-and-message-template.md in sento). The plugin keeps and validates its settings itself, in the place and
// format of its choice; SEN never reads or writes them. The settings window starts the plugin with kSettings if it is not running.
/** Ask for the settings. The reply has `settings`, `schema` and `defaults` (messages, see SenSettingsForm.h; the last two may be left out), and `result`. This starts a settings
 *  session of the plugin: it stays until it gets kSettingsClose, until the window that asked is gone, or until it has heard nothing for a minute. */
inline constexpr uint32_t kSettings      = 'SEse';
/** Change settings: `settings` holds the changed values, each under its path key (the whole message after a revert or defaults). The plugin validates and stores what is valid
 *  and applies it as it sees fit (a job that runs keeps the settings it started with). The reply has `result`, `detail` (why a value was refused) and `settings`, as the plugin
 *  now has them. */
inline constexpr uint32_t kSettingsSet   = 'SEss';
/** The window is done with the settings; the plugin ends the session (no reply). A plugin that was running for a job is not stopped by it. */
inline constexpr uint32_t kSettingsClose = 'SEsc';
/** The answer to kSettings or kSettingsSet that is sent to the messenger in the field kReplyTo of the request: a plugin that was started for the request has nobody
 *  that waits for a reply (the roster delivers the message, with no return address). Its content is that of the reply. */
inline constexpr uint32_t kSettingsReply = 'SEsr';
}

/** The signature of every plugin starts with this, then comes `<vendor>.<name>` (e.g. `sen-labs.pdf-extractor`). */
inline constexpr char kSignaturePrefix[] = "application/x-vnd.sen-plugin.";
/** The pattern of a query for the signatures of all plugins. */
inline constexpr char kSignaturePattern[] = "application/x-vnd.sen-plugin.*";

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
/**
 * Result of the feature `identify`: the semantic type of the file (`text/x-python`), the value that a plugin has set as the attribute META:TYPE of the file.
 * A plugin that identifies a file also returns it in its reply under this name.
 */
inline constexpr char kSemanticType[] = "META:TYPE";
/** BMessenger in a settings request: where a plugin that was started for it sends kSettingsReply (it answers a request that is waited for with a reply as usual). */
inline constexpr char kReplyTo[] = "reply_to";
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
