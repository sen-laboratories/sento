<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN user guide

## Relating files with Command+drag

Hold the **Command** key (the Apple key on a Mac keyboard, Alt on a PC one) while you drag files onto another file or folder, and let go: instead of copying, moving or opening anything, a menu
with the relations that can be created between the files opens at the pointer. Choose one and SEN creates it (in both directions, for
most relations), or press Esc or click outside the menu to cancel. The files stay where they are.

- **A label:** hold **Shift** while you choose the relation in the menu, and a small window asks for a label for the relation (it can be
  left empty). Cancel (or Esc) creates nothing. The opposite direction keeps the label of its own that the relation defines, if it has one.
- **Several files:** select the files to relate and drag them. If you drop them on a file that is part of a selection, all the files of
  that selection are the target: the relation is created between every dragged file and every target file.
- **Which relations are offered** depends on the kinds of the files: only relations that fit all files on both sides are shown. A label
  (classification) and a file are related by an association, two files by the other relations.
- Files in the Trash and the Desktop folder itself have no relations. Dropping files on themselves does nothing.
- If a relation could not be created (for example because it is read-only), a message says how many failed and why.

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
