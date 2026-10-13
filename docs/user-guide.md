<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN user guide

## The SEN item in the Deskbar

The SEN server puts its knot into the Deskbar when it starts. **Clicking** it opens a window with all the objects that SEN knows: the files that have
a SEN:ID, as a flat list. The menu (secondary mouse button) has:

- **Context**: the contexts that are set up (the folders of `~/config/settings/sen/contexts`, with their icon), the current one marked: choosing one makes it the current one.
  A new context ("New related... Context" in the file manager) is a folder there, with the type of a context.
- one menu for every kind of classification (**Labels**, **Topics**, **Projects**...): choosing the menu itself opens the list of them, and choosing one of
  its entries shows the files that it classifies. (As in the menus of the file manager, every menu can be chosen itself.)
- **Info**: the overview of SEN as a tree: the objects on every volume, the types and relations that are installed with the number of files of each.
- **Check**: looks for inconsistencies (the same SEN:ID on several files, files of types that are not installed, volumes without an index).
- **Ontologies** (above Settings, they are configuration): the ontologies that are installed, each with its icon. An ontology opens down to its types (the entities, relations,
  classifications...), and a type to its attributes: choosing a type or an attribute shows it in FileTypes (the one of SEN, package `sen_filetypes`, if it is installed
  and not switched off in the Settings), choosing an ontology shows it in its folder. Choosing *Ontologies* itself opens the folder `~/config/settings/sen/ontologies`, which shows the version, description, author and schema of each ontology as its columns (in Akita), and *Update* has the ontologies of the packages
  installed now, which is otherwise done when the server starts and when a package comes or goes.
- **Settings**: the key of the SEN mode of dragging (Command, Option, Control or Shift), the use of Akita instead of Tracker (this takes
  effect at the next login: it is a file of the user for the launch daemon, which the command `akita` writes; `akita start` and `akita stop` switch at once) and the use of the FileTypes of SEN instead of the one of Haiku.
- **About**, **Restart** and **Quit**. Restart quits the server and starts it again (it is the way to apply what a new version of the server brings). Quitting asks first: without the
  server, copies of files keep the SEN:ID of the original, which breaks the lookup by ID. It stays off until you start it again (`sen start`, or the next login).

The same information is on the command line: `sen info`, `sen types` and `sen check` (see the README of sen-core).

## Relating files with Command+drag

Hold the **Command** key (the Apple key on a Mac keyboard, Alt on a PC one; another key can be chosen in the Settings of the SEN item in the Deskbar) while you drag files onto another file or folder, and let go: instead of copying, moving or opening anything, a menu
with the relations that can be created between the files opens at the pointer. Choose one and SEN creates it (in both directions, for
most relations), or press Esc or click outside the menu to cancel. The files stay where they are.

- **A label:** hold **Shift** while you drop the files (it is remembered, so you can let go and pick from the menu) or while you choose the
  relation in the menu, and a small window asks for a label for the relation (it can be
  left empty). Cancel (or Esc) creates nothing. The opposite direction keeps the label of its own that the relation defines, if it has one.
- **Several files:** select the files to relate and drag them. If you drop them on a file that is part of a selection, all the files of
  that selection are the target: the relation is created between every dragged file and every target file.
- **Which relations are offered** depends on the kinds of the files: only relations that fit all files on both sides are shown. A label
  (classification) and a file are related by an association, two files by the other relations.
- Files in the Trash and the Desktop folder itself have no relations. Dropping files on themselves does nothing.
- If a relation could not be created (for example because it is read-only), a message says how many failed and why.

## Deleting files that have relations

When you delete a file (Move to Trash, or Delete) that has relations, Akita says which files it is related to, and asks whether you really want to delete it. If you do,
the relations of the file are removed on both sides together with it, so that no other file keeps a link to a file that is gone. (Relations that point to files that are
already gone are removed without asking. The relations of the ontologies are read-only and stay. Labels and the like that are put on a file go with the file anyway.)

## Sticky classifications

What you make while a classification is **sticky** is put into it automatically: a new file (*New* in a folder, a new file for a relation) is labelled with it, and a relation
that you make lists it, so that you can filter by it later. In the menu of the SEN item, **Shift+click** a label, topic, collection... to make it sticky (it gets a check mark)
and Shift+click it again to let it go; a plain click shows its files as before. The **current context** (choose it in the Context menu) sticks by being the current one, the
global context does not (it would be everything).

## New objects from the ontologies

An ontology that you install brings its entities (Book, Person...) to the **New** menu of Akita: they are templates in the folder *Tracker New Templates*, a submenu for each
ontology. You can make an object of a new type at once; templates that you delete or change are left alone, and removing the ontology removes only those that it made.

## Relations in folders

A relation connects a file with another file: a book with its author, a PDF bookmark with a page, an ontology with the types it provides.
You see a relation as a **folder of files**, like any folder: choose *Open related...* in the context menu of a file, pick the kind of relation, and
a window opens that has one file for each relation.

### A relation file stands for the file it points to

The files in such a window are **placeholders** for the files at the end of the relations. They show the details of the relation in their columns
(the label, the page of a reference, ...), but they *represent the other file*:

* **Double-click** opens the file that the relation points to, not the placeholder. The details of the relation come along: a reference to page 12 of a PDF opens the PDF at page 12.
* **Open related...** and **Open contained...** on a placeholder show what is related to, or contained in, the file that it points to.
* **Changing a column** of the placeholder (the label, for example) changes the relation itself.
* **Deleting** a placeholder removes the relation, not the file it points to. Dropping a file into the window adds a relation to it.

Some relations are read-only, e.g. what an ontology provides: you can look at them, but not change or remove them.

### Folders inside a relation window

A **folder** in a relation window is a relation of its own (a relation with several parts, an *n-ary* relation, e.g. a chapter with its sections). What you do with the folder
you do with that relation.

Soon: *New related...* on a relation, or on such a folder, will add another part to it (a binary relation becomes a relation of three files, and so on).

### Choosing the columns

In the **Attributes** menu, each kind of relation has its own list of columns. A relation window shows all of them at first. In the menu of the column titles (right click),
Shift+click keeps the menu open so that you can choose several in a row, and choosing the menu of a kind of relation selects all its columns.
