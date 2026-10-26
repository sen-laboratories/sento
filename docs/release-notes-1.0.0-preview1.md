<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->
# SEN 1.0.0 preview 1

The first preview of SEN, the Semantic Extensions Native for Haiku (R1/beta6, x86_64): a semantic layer on top of the file system that relates files to each other and to
things (people, books, ideas), with the files and their attributes staying what they are. **A preview**: it is meant for trying out and for feedback, and the names of
types and attributes may still change before 1.0 (see *What will change*).

## Install

```sh
pkgman add-repo https://kiri.sen-labs.org/x86_64      # once
pkgman install sen_core sen_plugins akita sen_filetypes
```

The SEN server starts with the next login (`sen start` starts it now) and puts its knot into the Deskbar. Akita, the Tracker of SEN, takes the place of Tracker at the next
login (`akita start` switches at once, `akita disable` goes back). More ontologies: `pkgman install sen_ontology_people sen_ontology_books ...`.

## What is in it

| Package | What it is |
|---------|------------|
| `sen_core` | the SEN server: relations, classifications, contexts, the Deskbar item (menus, Info/Check, settings), the command `sen` |
| `sen_oni` | the installer of ontologies (`sen_oni`), also run by the server when packages change |
| `sen_ontology_core`, `_books`, `_movies`, `_music`, `_papers`, `_people`, `_sourcecode` | ontologies: entities, relations and classifications as MIME types with attributes |
| `sen_plugins` (and `sen_plugin_<name>`) | SENSEI plugins: relations of PDF, Markdown and source code files (including the `#include`s of C/C++, Python, Java, Go, Rust, JavaScript and TypeScript), books and papers enriched from online services, navigators that open the targets |
| `akita` | the Tracker of SEN: relations as folders, menus to open and create them, classifications, contexts |
| `sen_filetypes` | FileTypes with searchable attributes and a view of the types of the ontologies |

**Relations as folders.** *Open related...* shows the relations of a file as a window of files that stand for the other ends, with the details of the relation as columns; changing a
column, deleting a file or dropping a file into the window changes the relation. *Open related...* itself lists all outgoing relations in one window. Command+drag relates files
(Shift asks for a label). Deleting a file that has relations asks first and removes them with it.

**Classifications and contexts.** Labels, topics, collections, concepts and contexts; *Associate with...* classifies, a classification opens as the query of the files that have it, and a
*sticky* classification (Shift+click in the menu of SEN) is put on everything that is made new.

**Ontologies.** Each ontology is a package; an ontology brings its types, their attributes (columns, queries by indexed attributes), its entities as templates of the *New* menu and
its relations, grouped in menus (people: personal, family, professional, organizations; relations end only at the types that fit).

## Requirements and limits

* Haiku R1/beta6 on x86_64 (the VM of the CI is `r1beta6`); BFS volumes (relations and indices are attributes).
* The preview has been tried on one machine by one person: expect rough edges, and report them at https://github.com/sen-laboratories.
* Plugins that handle the same file are not ordered by a rule yet (the PDF is read by the extractor, Paperboy only identifies and enriches).

## What will change

SEN is moving to a cleaner model of types (`semantic-types.md`): the format of a file stays its MIME type, what a file *means* becomes one identifier (`META:TYPE`) of a
registry with a tree and mappings to other vocabularies, the same on macOS. The types of the ontologies are renamed then, and the MIME database of SEN's types is cleaned; the
files and their relations are kept for a migration only once a release has data that needs it.
