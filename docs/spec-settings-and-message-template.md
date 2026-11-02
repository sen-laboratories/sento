<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# Settings for SEN and its plugins, and a generic template for BMessage data

Status: **accepted for beta 5** (the generic form and the Shoji template for messages first, then the settings window). The movie enricher is not part of it.

## 1. Goal

Today a plugin has no window: Paperboy reads `key = value` lines from `~/config/settings/sen/paperboy.settings`, the SEN server keeps three settings in a window of its own, and
a key such as the one of OpenAlex has to be typed into a text file. The aim:

* a plugin **describes** its settings (a message, optionally a schema and a layout) and has no settings window of its own;
* the user configures SEN **and** its plugins in **one** settings window, with controls that match the data;
* the controls are made from the data (generic), so that the same code shows any archived `BMessage`, for example a settings file in Shoji.

Principles:

* **Loose coupling.** SEN does not know where a plugin keeps its settings and never reads or writes them itself. It asks the plugin for them and gives the changed ones back.
* **The plugin stays in charge.** It decides how and when a change takes effect (a running job keeps the settings it started with) and what is stored.

## 2. The data: a settings message

A settings object is a `BMessage`. Supported field types and how they are edited:

| Field type | Control |
|---|---|
| `B_BOOL_TYPE` | check box |
| `B_INT8/16/32/64_TYPE`, `B_UINT*` | text field with a number check; a slider with a spin field if the schema gives a range; a menu if it gives choices |
| `B_FLOAT_TYPE`, `B_DOUBLE_TYPE` | text field with a number check (range from the schema) |
| `B_STRING_TYPE` | text field; menu if the schema gives choices; text field and a *Choose…* button (file panel) if `kind = path`; masked if `secret` |
| `B_REF_TYPE` | path of the file with a *Choose…* button |
| `B_RGB_COLOR_TYPE` | `BColorControl` |
| `B_POINT_TYPE`, `B_RECT_TYPE` | two or four number fields |
| `B_MESSAGE_TYPE` | a group of the controls of its fields (a node in the tree, a section in the form) |
| several values under one name (an array) | a list that is edited in the list (`ShojiListControl`: double click changes, Delete removes, `+` adds); an array of messages is a list of groups |
| everything else (`B_RAW_TYPE`, `B_MESSENGER_TYPE`, pointers, unknown types) | read-only line with the type and the size; the value is kept as it is when the message is sent back |

A field never changes its type through the editor; a value that does not fit its type (or the range of the schema) is refused and the control goes back, as in Shoji.

## 3. Path keys

The link between data and view is the **name of the control**: the control of a value is named with the **path key** of the field, as the controls of Shoji are named after
the attributes today (`BView::FindView(name)`).

```
path      = segment { "." segment }
segment   = name [ "_" index ]
index     = two or more digits, zero based (the 2-digit form: _00 ... _99, then _100)
```

`nested.property_02` is the third value of the field `property` of the message `nested`. The path is relative to the settings message (the tree shows a root node with the name of the object above it, which is not part of the path). Rules:

* a name that contains `.` has it escaped as `\.` (and `\` as `\\`);
* a segment is first looked up as a field with exactly that name; only if there is none and the name ends in `_<digits>`, the digits are the index of an array field of the rest (so a field that is literally called `max_02` still works);
* an index outside the array is an error for a read and appends for a write when it equals the count.

Resolution (`Get(path)`, `Set(path, value)`, `Has(path)`, enumerate the fields in order) is one small function set, header-only like the other SEN headers (`SenSettingsPath.h`).

## 4. Schema and defaults

A message alone has no label, no help text, no range. The **schema** is a second message with the same structure; per path it can hold:

`label`, `help`, `kind` (`text`, `path`, `choice`, `slider`, `secret`, `color`), `choices` (strings, or value/label pairs), `min`, `max`, `step`, `order`, `restart` (a change takes effect when the program is started again), `hidden`.

Without a schema the view derives the label from the field name (`max_authors` becomes *Max authors*) and the control from the type. A second message, **defaults**, holds the value of every field when nothing was set: it is what *Revert to defaults* puts back.

Settings, schema and defaults come **from the plugin**, in its answer to the request of section 6: the plugin knows them, SEN does not read any file or resource for them.

## 5. The view

A form is made from `(values, schema)`. Two ways to draw a nested message were considered:

* **A. an outline list as the way in, real controls in a pane.** The tree (`BOutlineListView`) has the messages as nodes; the pane shows the controls of the selected node. Real controls give tabbing, focus, menus, drag and drop and the look of the rest of the system, and one pane is the same widget that Shoji shows for a form. **Chosen.**
* **B. a property grid in one `BOutlineListView`**, a row per field with an editor that is put over the row when it is selected (the pattern of `ShojiListControl`). It looks like an inspector and shows a deep message at once, but a `BListItem` is drawn, not a control: focus, keyboard, scrolling and accessibility are all code of our own. *Later*, for an inspector of arbitrary messages.

**Layout from a plugin.** A plugin may bring a **layout**: an archived `BView` in a resource of its binary, read by SEN **without starting the plugin**. Its controls are named with path keys and are connected to the values as the generated form is. Only **layout** is allowed: the view is made of classes of the system (`BGridLayout`, `BViewLayoutItem`, `BTextControl`, `BCheckBox`, `BMenuField`, `BStringView` ...), because a process can only unarchive classes that it has: a plugin cannot give another process a control of its own. This is what the templates of Shoji are already (the archives of the installed templates contain only `BGridLayout`, `BViewLayoutItem`, `BTextControl` and `BStringView`; Shoji puts the list control in place of a text field when the form is shown). The resource is made at build time by the plugin (a small step like `shoji -g`, then `xres`). A plugin without a layout gets the generated form.

Live editing: every control posts its change with the path and the new value to the owner of the view, which changes the **working copy** of the message. For plugin settings the owner then **sends the change to the plugin** (section 6); Shoji writes the file. **Revert** replaces the working copy by the **original copy** that was taken when the view was opened, and sends it back; **Defaults** replaces it by the defaults message.

## 6. The SEN settings window and the plugin protocol

```
 ┌──────────────────┬───────────────────────────────────────────┐
 │ ▾ SEN            │  Paperboy                                 │
 │    Core          │  E-mail address  [                      ] │
 │   ▾ Plugins      │  Ask Unpaywall   [x]                      │
 │    ▾ Enrich      │  ...                                      │
 │       BERT       │  [Revert] [Defaults]                      │
 │       Paperboy   │                                           │
 │    ▸ Extract     │                                           │
 └──────────────────┴───────────────────────────────────────────┘
```

* **Left:** a `BOutlineListView`. *Core* holds the settings of the server (the three that are in the window now; the server owns them). *Plugins* has a node per feature (`extract`, `enrich`, `identify`, `navigate`, `search`); a plugin is listed under **every** feature that it supports (the attributes `SEN:plugin:<feature>`), and each item stores the **signature** of the plugin. The list is made with the plugin lookup of the server (`sen::FindPlugins`), so a plugin that is installed or removed shows up at the next opening.
* **Right:** the settings of the selected item: the layout of the plugin, else the generated form (section 5), filled with the settings that the plugin gave.

The protocol (`Sensei.h`; the plugin is started with `be_roster->Launch(signature, &message)` if it is not running, as the About window is asked for today with `B_ABOUT_REQUESTED`):

| Message | From | Content | Reply |
|---|---|---|---|
| `SEN_PLUGIN_SETTINGS` (`sensei::cmd::kSettings`) | window | – | `settings` (message), `schema`, `defaults` (messages), `result` |
| `SEN_PLUGIN_SETTINGS_SET` (`kSettingsSet`) | window | `settings`: the **changed** fields, with their path keys (or the whole message on Revert and Defaults) | `result`, `detail` (why a value was refused), and the settings **as the plugin now has them**, which the window shows |
| `SEN_PLUGIN_SETTINGS_CLOSE` (`kSettingsClose`) | window | – | none; the plugin ends its settings session |

* A request starts a **settings session**: a plugin that was started for it stays until it gets `kSettingsClose`, until the window that asked (its `BMessenger`) is gone, or until it has heard nothing for a minute (a plugin that was already running for a job is not stopped by the close).
* The plugin **validates** and **stores** what it gets, in the format and the place of its own choice (Paperboy: its file; later perhaps a message file). A change does not touch a job that runs: the plugin applies it as it sees fit (typically: from the next job).
* The window **never writes a file** of a plugin and does not assume that one exists.
* A plugin that needs more than the layout (a login, a list from a service) has a button in its layout that sends it a message and shows a window of its own, as About does.
* *Core* is the same form with the settings of the server as values; the server handles its own requests (`UpdateSettings`, already there).

## 7. Shoji: a template for messages (first consumer, proof of concept)

The generic form is first built and tested with Shoji, because Shoji needs the same pieces: the controls named by path keys, the hooks that store a change, the list control, Revert.

* Shoji takes the values from a **store** and no longer from the attributes of a file directly. The interface: `Get(path)`, `Set(path, value)`, enumerate, a change signal, `Revert`. Implementations: `AttributeStore` (what Shoji does now), `MessageFileStore` (a flattened message file: Shoji reads and writes it; this is a user opening a file in Shoji, not SEN touching a plugin's files) and `PluginSettingsStore` (section 6: the values come from the plugin and go back to it). The template views, the list control and the hooks stay as they are; the name of a control is the attribute or the path key.
* The generic template for a message builds the form of section 5 for the file that is opened. A settings file is recognised by its type `application/x-vnd.sen-labs.settings` (the attribute `BEOS:TYPE`), so that Shoji opens it by double click and *Open With*; any other flattened message can be opened with *Open With*.
* The builder and the stores' interface are a header-only or small static library of sento (`SenSettingsForm.h`), because the server cannot depend on Shoji. Shoji and the settings window of SEN then show the same form for the same message.

## 8. Compatibility

* A plugin keeps its present settings storage until it chooses to change it: Paperboy implements the protocol over its `Settings` structure (to and from a message, field names = the keys of `paperboy.settings`, with the file as it is). The command line and the environment keep their precedence.
* The schema carries a `version`; fields that the schema does not know are kept when the settings are sent back.
* A key (OpenAlex) travels and is stored as the plugin decides; `secret` only hides it in the window. The window does not claim that it is encrypted.

## 9. Order of work (beta 5)

1. **Format and code:** `SenSettingsPath.h` (path functions) with tests (no GUI); the schema and defaults fields; the store interface.
2. **Generic form** (`SenSettingsForm.h`): controls of section 2, Revert and Defaults, layouts by control name; tested in the VM with screenshots.
3. **Shoji:** `MessageFileStore` and the template for messages (the proof of concept), with the settings of Shoji itself and a flattened message of SEN as examples.
4. **Protocol and window:** `kSettings`, `kSettingsSet`, `kSettingsClose` in `Sensei.h` with the plugin side in `common/` (a small helper that a plugin app uses to answer), the tree and the pane in the server, the plugin lookup; *Core* first, then Paperboy, then BERT and the other plugins as they get settings.
5. *Later:* the inspector for arbitrary messages (variant B), the movie enricher (a new plugin that will need an API key and therefore the settings window).

## 10. Open points

* Where the translations of the labels come from for a plugin whose catalog is not bound into the binary (the plugin gives translated strings in its schema, as it has its own catalog, or the server asks its catalog by signature).
* Whether `restart` is shown in the window or acted on.
* How the window behaves when the plugin cannot be started or does not answer (a message in the pane, the other plugins stay usable).
* A plugin that is a script or has no `BApplication` of its own (none so far: all plugins are applications).
