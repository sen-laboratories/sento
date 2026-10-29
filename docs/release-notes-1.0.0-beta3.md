<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN 1.0.0 Beta 3 "Sofia" — *Look Closer*

> From the graph to the details of a thing...

Beta 2 let you *see* the graph of your files. Beta 3 lets you **look closer**: select an object and a card shows what is known about it, **Shoji** opens an entity to read and edit its attributes in a form, values that are
lists *are* lists now, and the plugins ask you when a lookup has more than one answer.

**SEN (Semantic Extensions Native)** on **Haiku R1/beta6 nightlies (x86_64), from hrev60299 on**. If you have beta 2 installed, `pkgman update` brings you here (see *Upgrading* below).

> [!IMPORTANT]
> **SEN follows the Haiku nightlies.** Akita links against private classes of Haiku, which change from time to time (in hrev60299_3 `BPrivate::JobQueue` got another constructor, and an Akita built before that fails to load: no file
> manager). The packages of Akita and SEN FileTypes therefore require `haiku >= r1~beta6_hrev60299_1` and set **no upper limit**, so `pkgman` never holds your Haiku back. They are built on hrev60299+11. A check against the newest
> nightly runs every day (`abi-check.sh` in the [akita](https://github.com/sen-laboratories/akita) repository), and a new build follows when a nightly breaks Akita.
> Because SEN Core requires FileTypes, **this beta needs a Haiku nightly from hrev60299_1 on**; `pkgman update` updates Haiku too (it asks for a reboot).

## Cards for what you point at

Point at a dot in the graph and a **card** appears next to it: the thumbnail or icon of the file, its title and the attributes that matter for its type, and at the bottom **how many relations** it has. The background is
translucent, so the graph shows through; it is not a window. It replaces the old little pop-up with the name and the number of relations.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/graph-card-node.png" alt="The graph with the card of a person" width="760">

* **Move the mouse onto the card** (it stays while you go there) and it **lights up** a little; **click it** and the file or folder opens, just as with a double click on the dot.
* **Which attributes** are in the card, in which order and under which labels is what the **Shoji template** of the type says (so a card and the window of Shoji show the same); a type without a template shows the viewable attributes of
  its MIME type. What a file does not have is left out, so a card is as small as it can be.
* **Lists** are shown as simple CSV and are cut in the middle ("Family, Friends, Colle...rs, Haiku Users") when they are too long.
* **A line (a relation) has a card as well**: the icon of the relation, its label as seen from the end that the mouse is nearer to ("cited by" at one end, "Citation" at the other), the two objects, and how many **targets**
  that end has. Move along the line and the card follows; click it to open the folder of those relations.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/graph-card-relation.png" alt="The card of a relation in the graph" width="760">

* The **Preview** checkbox in the window of the graph switches the cards off. The graph has a filter for **Ontologies** (off by default), like the one for classifications and objects without relations.

## Shoji: an entity as a form

**Shoji** is new in the repository: it shows and edits the attributes of a file in a form that belongs to its type. Open a file with it (or *Shoji* from the Applications menu of the Deskbar).

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/shoji-book.png" alt="Shoji showing a book with its cover" width="460">

* **Templates** for contacts (`application/x-person`) and books (`entity/x-vnd.sen-labs.entity.book`; the cover is the thumbnail that BERT fetched and sits on the stripe). Any other type gets a generated form with a control for
  each viewable attribute of its MIME type. A template of your own goes to `~/config/settings/shoji/templates/<MIME type>/template.view` and replaces the one of a package.
* **Edits are stored as you make them** (Enter, leaving a field, closing the window), not with a Save button; *Revert* puts the file back as it was opened. If somebody else changes an attribute while the window is open, Shoji asks whether to keep
  your version or take the new one.
* **A list is a list**: an attribute that its type declares as a list (authors, subjects, the groups of a person) shows as a list of values with *Add* and *Remove*, and the one selected is edited in a field.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/shoji-contact-list.png" alt="Shoji showing a contact with its groups as a list" width="520">

## Lists of values

An attribute can hold a **list in one string** (`Andy Hunt,Dave Thomas`). Storage and representation are separate: the file keeps the text, a program that knows it is a list shows the values one by one.

* The **attribute info** of the MIME type says so: `SEN:is_list` and the one character `SEN:list_separator`, a comma unless the schema says otherwise (a comma is what People uses for `META:group`). The ontologies write them from
  their schemas (`is_list: true` on the slot): authors, subjects, languages and publishers of books and papers, ISBN, classification, and the **groups of a person**, including an overlay for the attributes of Haiku's People
  (`application/x-person`), so the groups of an existing contact are a list as well.
* The values are split and joined as in CSV: blanks around a value do not belong to it, a value in quotes (`"Hunt, Andy"`) may contain the separator.
* **SEN FileTypes** has the two fields now: *List of values* (for a String) and *Separator* in the dialog of an attribute.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/filetypes-list.png" alt="The attribute dialog of FileTypes with List of values and Separator" width="300">

* **BERT and Paperboy** write list values as lists (comma, quotes where needed).
  *Data written by beta 2 used a semicolon: it shows as one value until the plugin is run on the file again or you edit it.*

## Plugins that ask

When a lookup has **more than one answer**, **BERT** (a book from OpenLibrary) and **Paperboy** (a paper by its title) no longer take the first: a window lists the candidates, the best first, with a bar for how good each is (how close the title
is to what was asked, how complete and well known the entry is) and the source on the right. `--auto` takes the best without asking (for scripts). BERT also writes the cover as `Media:Thumbnail` and renames the file after the title.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.3/docs/images/beta3/choose-a-paper.png" alt="Choosing one of several papers" width="620">

## The About windows

**About SEN** and the About windows of the plugins have the dark stripe with the picture and show the **full version**, from the version resource of the program: `1.0.0-beta.3` (the package is `1.0.0~beta3`; development
builds of 2.0 show `2.0.0-develop.1`).

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

## Known limits

* The graph shows up to 2000 objects; with more, the status line says how many were read.
* BFS volumes only, as in beta 1 and 2 (SEN keeps everything in extended attributes).
* Shoji has templates for contacts and books; other types get the generated form.
* A Haiku older than hrev60299_1 cannot be used with this beta (Akita would not load there).

## What comes next

SEN 2.0 "Gnosis": the unified registry of semantic types, an Akita of its own with a graph view next to the list and icon views, a way to walk the graph with nested menus from any file (a "SEN mode" of the context menu),
and the lists of values as real relations (the groups of a person as a *Collection*).

## Your support is essential

Please consider hitting the "Support" button in the About dialog and contribute to the ongoing development of SEN, or simply donate right from here to [SEN Labs Liberapay](https://liberapay.com/sen-labs/).

## Feedback & Community

Report issues, share workflows, and inspect the code:

* **Issue Tracker & Source:** [https://github.com/sen-laboratories](https://github.com/sen-laboratories)
* **Package Repository:** `https://kiri.sen-labs.org/x86_64`
* **Community Discussion:** Haiku Community Forums
