<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN 1.0.0 Beta 2 "Sofia" — *The Graph*

> From files and folders to nodes and edges...

Beta 1 made the knowledge graph of your files something you can *walk* through: folders as dynamic projections of relations. Beta 2 lets you *see* it.

**SEN (Semantic Extensions Native)** on **Haiku R1/beta6 (x86_64)** gets a **graph view** of all your objects and their relations, ontologies that open as the folders they are, and a round of
fixes found while living with beta 1. If you have beta 1 installed, `pkgman update` brings you here (see *Upgrading* below).

## The Graph

Every file with a `SEN:ID` is a dot, every relation a line. Things that belong together stand together: an ontology with the types it defines, a book with its author, a paper with the papers it cites.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.2/docs/images/beta2/graph-dark.png" alt="The graph of SEN in dark mode" width="760">

**Open it** with the **SEN key (Command) + click on the knot** in the Deskbar, or choose **Graph...** in its menu (below *Ontologies*). In Akita, **Command + double click on a file** opens the graph centred on that file;
the same happens with the knot when a file is selected in Akita.

* **Select** a dot to emphasise its relations and see the names of its neighbours, **double-click** it to open the file. **Click a line** to select a relation, **double-click it** to open the folder of the
  relations of that type (a relation is in both objects, so it opens at the one that is nearer to your click).
* **Zoom** with the mouse wheel, **drag** to move, **F** fits everything; resizing the window fits the graph again until you have zoomed or moved it yourself.
* Colours show the types, the size shows how many relations an object has. The labels of the relations are those of the relations themselves ("parent of / child of").
  Classifications, contexts and objects without relations can be switched on when you want them.
* **Dark mode** follows the colours of your system until you choose; the choice is remembered (the setting `SEN:darkMode`).
* **Save as SVG** saves the picture of the window as you see it. **Save selection** saves the selected object with its direct connections, which makes a clean picture of one thing and what it is related to.

The graph is read from the attributes of your files when you open it (up to 2000 objects), nothing else is stored.

## Ontologies are folders

An **ontology** now looks and acts like what it is, a folder of relations: it has the icon of a relation folder, and opening it in Akita shows its *Dependency* relations, what it defines and needs, instead of
starting FileTypes with an arbitrary one of its types.

## Fixed and improved

* **The menu of the Ontologies in the Deskbar**: the attributes of a type did nothing when chosen. Items of submenus of the menu did not get their target (a platform pitfall, documented in the developer guide).
* **Open related... > relation > target**: choosing a target in the menu of a relation (a type in the *Dependency* relations of an ontology, for example) ended in an error dialog; it opens now (a type in FileTypes).
* **Citation** is offered between books, movies, persons, documents and papers (PDF, EPUB), not between any two files any more.
* **Ontology packages** are at revision `0.1.0-19`, and an ontology that changes alone can have a revision of its own, so that an update of one ontology is an update for `pkgman` without touching the others.
* **Quiet log**: temporary files of the shell on volumes without attributes no longer fill the log with errors.
* **The knot stays**: it disappeared when a window of the SEN server was closed with Alt+Q, which asks the application to quit. Such a window closes now, and the knot is removed only when the server really quits.
* **Packaging**: `sen_plugins` requires each plugin at least in the version that it was built with (an older plugin from a repository can no longer take its place); builds of all plugins in one step (`sensei/build.sh`);
  `akita` and `sen_filetypes` build and package in one step and stop on a failed build.
* **Documentation**: the user guide and the developer guide describe the graph, the ontology folders and the helpers in sento (`SenGraph.h`, the type functions of `SenLookup.h`).

## Upgrading from Beta 1

Beta 2 sorts after beta 1, so `pkgman update` finds it. The knot in the Deskbar is loaded from the program of the server, so stop SEN while the packages are replaced, and start it again afterwards:

```sh
sen stop
pkgman update
sen start       # and, to have the new Akita:  akita restart
```

Your data is untouched: relations and metadata stay in the attributes of your files.

> [!IMPORTANT]
> **Akita follows the Haiku nightlies.** Akita links against private classes of Haiku, which change from time to time: in the nightly hrev60299_3 `BPrivate::JobQueue` got another constructor, and an Akita built before it fails to load,
> which would leave you without a file manager. The packages of Akita and SEN FileTypes therefore require the nightlies from there on (`haiku >= r1~beta6_hrev60299_1`) and set **no upper limit**, so `pkgman` never holds your Haiku back.
> The packages `1.0.0~beta3-1` of both (they replace the ones that this beta came with) are built on hrev60299+11: `pkgman update` brings them together with the update of Haiku. A check against the newest nightly runs every day,
> and a new build follows when one breaks Akita (`abi-check.sh` in the [akita](https://github.com/sen-laboratories/akita) repository).

## Quick Install

```sh
pkgman add-repo https://kiri.sen-labs.org/x86_64                  # once
pkgman install sen_core sen_plugins akita sen_filetypes
pkgman install sen_ontology_people sen_ontology_books sen_ontology_papers \
               sen_ontology_movies sen_ontology_music sen_ontology_sourcecode     # optional
```

The server starts with your next session (or run `sen start`); `akita start` switches to Akita at once, `akita disable` goes back to the Tracker of Haiku.

## Known limits

* The graph shows up to 2000 objects; with more, the status line says how many were read.
* BFS volumes only, as in beta 1 (SEN keeps everything in extended attributes).
* Akita works with the nightlies from hrev60299_3 on only with its packages `1.0.0~beta3-1` (see above); the Akita that this beta was first published with does not load there.

## What comes next

SEN 2.0 "Gnosis": the unified registry of semantic types, an Akita of its own with a graph view next to the list and icon views, and a way to walk the graph with nested menus from any file (a "SEN mode" of the context menu).

## Your support is essential

Please consider hitting the "Support" button in the About dialog and contribute to the ongoing development of SEN, or simply donate right from here to [SEN Labs Liberapay](https://liberapay.com/sen-labs/).

## Feedback & Community

Report issues, share workflows, and inspect the code:

* **Issue Tracker & Source:** [https://github.com/sen-laboratories](https://github.com/sen-laboratories)
* **Package Repository:** `https://kiri.sen-labs.org/x86_64`
* **Community Discussion:** Haiku Community Forums
