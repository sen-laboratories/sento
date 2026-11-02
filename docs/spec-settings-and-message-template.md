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
* the controls are made from the data (generic), by **one implementation**: Shoji's view for a `BMessage`. Shoji shows it for any message file; the SEN settings window **hosts the same view as a replicant** (section 7).

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

Live editing: every control changes the **working copy** of the message that the view holds and posts the change (path and value) to its **owner**: the window of Shoji, which writes the file, or the SEN settings window, which **sends the change to the plugin** (section 6). **Revert** replaces the working copy by the **original copy** that was taken when the data was set (and reports the whole message); **Defaults** replaces it by the defaults message.

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

## 7. Shoji: the view for messages, and its reuse as a replicant

Shoji knows two kinds of data: the **attributes** of a file (as now) and **messages** (a flattened message file, or one in memory; a message is the attributes' model with nesting and arrays). It knows nothing of SEN plugins. The settings window of SEN gets its settings from the plugins as messages (section 6) and needs nothing else from Shoji than the view.

* **`ShojiMessageView`** is a `BView` that builds the form of section 5 for a message. Shoji uses it in its own window for a message file (the template for messages: any flattened message opened with *Open With*; a settings file is recognised by its type `application/x-vnd.sen-labs.settings`, written as `BEOS:TYPE`). The path function set and the control building are shared with the attribute templates (control name = attribute or path key).
* **As a replicant.** The same class is archived with `add_on = application/x-vnd.sen-labs.shoji`. The settings window makes the stub of the archive (the class name and the signature, nothing else), and `instantiate_object()` **loads the class from the executable of Shoji**, found with `BRoster::FindApp()`: the way the knot of SEN is loaded from `sen_server` into the Deskbar. The window does not link Shoji. It talks to the view with messages only:

| Message | To / from the view | Content |
|---|---|---|
| `kSetData` | to | `settings`, optionally `schema`, `defaults`, `layout` (archived view of section 5), `target` (the messenger for the changes), `protocol` |
| `kChanged` | from | `path`, `value` (as text and typed), `settings` (the whole working copy) |
| `kGetData` | to, with reply | `settings` |
| `kRevert`, `kDefaults` | to | – (the view answers with `kChanged` for the whole message) |

  The codes and field names are in a header of sento (`SenSettingsForm.h`: constants only); the form itself is code of Shoji only.
* **Tried in the VM** (a provider application with the view class, a host without a link to it; 2026-11-02): the class was loaded from the other executable, the form was built from a message with a nested message and an array, six changes (a check box, text, a number, an array element, two fields of the nested message) arrived at the host with their path keys, and the data could be read back. Two points that cost an hour: the loader looks up `Class::Instantiate(BMessage*)` among the dynamic symbols of the executable, so it must be **defined out of the class body** (an inline function is not emitted and the load fails with no message); and the host and the class share the process, so the class cannot use `be_app`, its catalog or its resources as if the application were Shoji (below).
* **What the view must not assume**, because the application is the host: no use of `be_app` (the `ShojiApp` of Shoji), no global state of the application, the catalog of Shoji bound explicitly (`BCatalog` with its signature, not the one of the running application), icons and other resources read from the file of Shoji (`BResources` of the executable found with the signature), the window found with `Window()`.
* **Without Shoji** (not installed, or too old for the protocol): the settings window says so in the pane and offers to open the settings file of Core; no second implementation of the form is kept. SEN's packages recommend `shoji`; `sen_core` does not require it.
* **Isolation.** The class runs in the process of the host: a crash in the form is a crash of the host. The settings window of the server would take the server down with it. Hosting the window in a small helper application of `sen_core` (`sen_settings`, started by the *Settings…* item of the Deskbar menu, talking to the server and the plugins) keeps the server out of it, and is cheap as the window is the only part. **Decided** for beta 5.

## 8. Compatibility

* A plugin keeps its present settings storage until it chooses to change it: Paperboy implements the protocol over its `Settings` structure (to and from a message, field names = the keys of `paperboy.settings`, with the file as it is). The command line and the environment keep their precedence.
* The schema carries a `version`; fields that the schema does not know are kept when the settings are sent back.
* A key (OpenAlex) travels and is stored as the plugin decides; `secret` only hides it in the window. The window does not claim that it is encrypted.

## 9. Order of work (beta 5)

1. **Format and code:** `SenSettingsPath.h` (path functions, plain C++, tests on every platform) and `SenMessagePath.h` (`GetText`, `SetText`, `Append`, `Remove`, `Enumerate` on a `BMessage`, tests in `tests/vm`): **done**; the schema and defaults fields; `SenSettingsForm.h` (the message codes).
2. **`ShojiMessageView`:** the generated form with the controls of section 2, layouts by control name, Revert and Defaults; Shoji's window for a message file (the proof of concept, with the settings of Shoji and a flattened message of SEN as examples); the view free of the assumptions in section 7; tested in the VM with screenshots.
3. **Replicant:** the host side (`instantiate_object`, the messages, the fallback without Shoji) with a test host in the sento tests (the proof of concept of 2026-11-02 as its start).
4. **Protocol and window:** `kSettings`, `kSettingsSet`, `kSettingsClose` in `Sensei.h` with the plugin side in `common/` (a small helper that a plugin app uses to answer), the tree and the pane, the plugin lookup; *Core* first, then Paperboy, then BERT and the others as they get settings.
5. *Later:* the inspector for arbitrary messages (variant B), the movie enricher (a new plugin that will need an API key and therefore the settings window).

## 10. Open points

1. ~~Where the settings window lives~~ — **decided: a helper application** (`sen_settings`, section 7): the replicant is loaded into it and a fault in the form cannot take the server down.
2. Where the translations of the labels come from for a plugin whose catalog is not bound into the binary (the plugin gives translated strings in its schema, as it has its own catalog, or the server asks its catalog by signature).
3. Whether `restart` is shown in the window or acted on.
4. How the window behaves when the plugin cannot be started or does not answer (a message in the pane, the other plugins stay usable).
5. The `protocol` number of the replicant messages and what the host does with a view that answers with another one.
