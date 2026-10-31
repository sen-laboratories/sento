<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN 1.0.0 Beta 3 "Sofia" — *Look Closer*

> “Without context, words and actions have no meaning at all.” – Gregory Bateson

Beta 2 introduced the graph view. Beta 3 lets you **look closer**: point at an object or a relation and a card shows its metadata.

**Shoji** acts as a universal entity editor that opens files in a template based metadata view, exposing its attributes in a dynamically configurable form. List values are real lists now, and enrichment plugins let you choose from a list of results, weighted by score.

**SEN (Semantic Extensions Native)** on **Haiku R1/beta6 nightlies (x86_64), from hrev60299 on**. If you have beta 2 installed, `pkgman update` brings you here (see *Upgrading* below).

> [!IMPORTANT]
> **SEN follows the Haiku nightlies.** Akita links against private classes of Haiku, which change from time to time (in hrev60299_3 `BPrivate::JobQueue` got another constructor, and an Akita built before that fails to load: no file
> manager). The packages of Akita and SEN FileTypes therefore require `haiku >= r1~beta6_hrev60299_1` and set **no upper limit**, so `pkgman` never holds your Haiku back. They are built on hrev60299+11. A check against the newest
> nightly runs every day (`abi-check.sh` in the [akita](https://github.com/sen-laboratories/akita) repository), and a new build follows when a nightly breaks Akita.
> Because SEN Core requires FileTypes, **this beta needs a Haiku nightly from hrev60299_1 on**; `pkgman update` updates Haiku too (it asks for a reboot).

> [!NOTE]
> **Update, 31 October:** `sen_core ~beta3-5` fixes the menu of the SEN key setting (it had an unneeded disabled entry, and the standard key is the default now), and `shoji ~beta3-4` has its own icon. `pkgman update` brings both.

## Cards in the Graph View

The graph view from beta 2 now shows **cards** when you point at a node or a relation.

A node gets a **card** next to it: the thumbnail or icon of the file, its title and the attributes from the file type definition, and the relation count at the bottom:

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/graph-card-node.png" alt="The graph with the card of a person" width="760">

* **Move the mouse onto the card** and it **lights up** a little; **click it** and the file or folder opens, just as with a double click on the dot.
* **Attributes** shown on the card, their order and label is defined by the **Shoji template** (an archived BView wired to the attribute names) for the particular type.
* **Lists** are expanded from CSV style comma separated lists (like the `Groups` attribute for People files) and written back in the original compact format. This allows for more intuitive editing, while keeping full compatibility and efficient storage.
* **Edges** between nodes represent relations: the icon of the relation, its label as seen from the nearest end ("cites" at one end, "cited by" at the other), the two relation ends, and how many **targets**
  that end has. Move along the line and the card follows; click it to open the folder of those relations.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/graph-card-relation.png" alt="The card of a relation in the graph" width="760">

* The **Preview** checkbox in the window of the graph switches the cards off. The graph filters internal types like **Ontologies** (off by default), as well as meta types like classifications, and objects without relations.
* The **SEN key** (Settings, via the Deskbar knot) now offers the standard Haiku shortcut key first, with the physical key named, and applies to the knot as well. Standard is the default; the other keys follow below the separator.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/main/docs/images/beta3/sen-key-settings.png" alt="The settings window of SEN with the SEN key" width="620">

## shōji – a universal template driven entity form editor

**Shoji** (from the Japanese word 障子 for paper doors with a wooden frame - symbolizing the template framework and the transparent handling of metadata) is new in the repository: it allows you to view and edit file attributes in a form that adapts to the file type:

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/shoji-book.png" alt="Shoji showing a book with its cover" width="460">

* **Templates** for contacts (`application/x-person`) and books (`entity/x-vnd.sen-labs.entity.book`; the cover is the thumbnail that BERT fetched, nicely tacked to the side). Any other type gets a generated form with a suitable control for each viewable attribute, as defined in the MIME type database. A template of your own goes to `~/config/settings/shoji/templates/<MIME type>/template.view` and replaces the one of a package.
* **Edits are stored as you make them**. External changes to attribute values while editing them are integrated safely.
* **A list is a list**: an attribute that its type declares as a list (authors, subjects, the groups of a person) shows as a list of values with *Add* and *Remove*, and the one selected is edited in a field.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/shoji-contact-list.png" alt="Shoji showing a contact with its groups as a list" width="520">

* **Shoji has an icon** now: the knot of its logo, with a plainer drawing for 16 and 24 px.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/main/docs/images/beta3/shoji-icon.png" alt="The icon of Shoji" width="128">

* **SEN FileTypes** has the two fields now: *List of values* (for a String) and *Separator* in the dialog of an attribute.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/filetypes-list.png" alt="The attribute dialog of FileTypes with List of values and Separator" width="300">

## Enrichment plugins handle multiple results now

When a lookup has **more than one answer**, **BERT** (a book from OpenLibrary) and **Paperboy** (a paper by its title) no longer take the first: a window lists the candidates, along with a scoring indicator bar, ordered by match quality (how close the title is to the query, how complete and well known the entry is) and the source on the right. `--auto` takes the best without asking (for scripts). BERT also writes the cover as `Media:Thumbnail` and renames the file after the title.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/choose-a-paper.png" alt="Choosing one of several papers" width="620">

## About eye candy

**About SEN** and the About windows of the plugins now have the standard, elegant striped look of Alert dialogs, with the version taken from the version resource: `1.0.0-beta.3` (the package is `1.0.0~beta3`; development builds of 2.0 show `2.0.0-develop.1`):

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/about-sen.png" alt="About SEN with the version 1.0.0-beta.3" width="520">

## Packages

* **New in Kiri: `qpdf` and `qpdf_devel`** (12.4.2), HaikuPorts has no recipe for it. `sen_plugin_pdf_extractor` requires `lib:libqpdf` now instead of bringing its own copy.
* **`shoji`** (`0.2.0~beta3`) is new; **`akita` and `sen_filetypes`** are `1.0.0~beta3` (the Haiku requirement above); the ontology packages are at `0.1.0-20` (the list fields, the overlay for People);
  all plugins and `sen_plugins`, `sen_core`, `sen_oni` are `~beta3`. The stage and its count are in the version resource of each program, and the package says the same.
* **For developers** (sento): `SenEntityCard.h` (the card), `SenChoice.h` (the window to choose one answer), `SenList.h` and `SenAttrInfo.h` (lists), `SenVersion.h` (the version of a program), and `AddContent()` / `AddButton()` in
  the About dialog; see the developer guide.

## Upgrading from Beta 2

Beta 3 sorts after beta 2, so `pkgman update` finds it. It updates Haiku first if it is older than hrev60299_1, and asks for a reboot; the knot in the Deskbar is loaded from the program of the server, so stop SEN while the packages
are replaced, and start it again afterwards:

```sh
sen stop
pkgman update
sen start       # and, to have the new Akita:  akita restart
```

New ontology packages bring the list fields; if the groups of your contacts are not a list yet, update `sen_ontology_people` (it is part of `pkgman update`) and reopen the contact.

## Quick Install

```sh
pkgman add-repo https://kiri.sen-labs.org/x86_64                  # once
pkgman install sen_core sen_plugins akita sen_filetypes shoji
pkgman install sen_ontology_people sen_ontology_books sen_ontology_papers \
               sen_ontology_movies sen_ontology_music sen_ontology_sourcecode     # optional
```

The server starts with your next session (or run `sen start`); `akita start` switches to Akita at once, `akita disable` goes back to the Tracker of Haiku.

## Known limitations

* The graph shows up to 2000 objects; with more, the status line says how many were read.
* BFS volumes only, as in beta 1 and 2 (SEN keeps everything in extended attributes).
* Shoji currently has templates for contacts and books; other types to follow. As a generic fallback, Shoji will show a dynamic form generated from the file type definition, honoring readonly attributes, and hiding those with their Display flag set to `false`.
* A Haiku older than hrev60299_1 cannot be used with this beta (Akita would not load there).

## What comes next

Besides probably one last beta and then release candidates for the 1.0 train, I am working at full speed on SEN 2.0 "Gnosis": the unified registry of semantic types in a rich, schema like hierarchy, breaking free from the strict 2-level MIME taxonomy.

Akita will become an independent information browser of its own, with the Shoji form view integrated, more flexible, multi-dimensional attribute views, an integrated graph view, and a way to walk the graph with nested menus from any file (a "SEN mode" of the context menu), and value lists as real relations (e.g., the groups of a person as a *Collection*).

More plugins, templates and ontologies will also follow.

## Your support is essential

Please consider hitting the "Support" button in the About dialog and contribute to the ongoing development of SEN, or simply donate right from here to [SEN Labs Liberapay](https://liberapay.com/sen-labs/).

## Feedback & Community

Report issues, share workflows, and inspect the code:

* **Issue Tracker & Source:** [https://github.com/sen-laboratories](https://github.com/sen-laboratories)
* **Package Repository:** `https://kiri.sen-labs.org/x86_64`
* **Community Discussion:** Haiku Community Forums
