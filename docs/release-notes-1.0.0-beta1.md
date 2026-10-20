<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN 1.0.0 Beta 1 — *The Relational Desktop*

> *"The human mind does not work by indexing, but by association."* — Vannevar Bush, 1945

Thirty-five years after the seminal *Semantic File Systems* paper at SOSP '91, and rooted firmly in the file-system lineage of BeOS and Haiku, **SEN (Semantic Extensions Native)** enters its first public beta on **Haiku R1/beta6 (x86_64)**.

Traditional operating systems treat folders as physical filing boxes: an entity is bound to a single parent path (`path = identity = location`). If a document belongs to three projects, you are forced into an artificial choice or left to juggle fragile symlinks.

**SEN replaces physical containment with dynamic relational projections.** 
Your files remain exactly what they are—standard files on standard BFS volumes with native attributes. But instead of living isolated in rigid trees, files are now linked to each other and to abstract entities (people, books, papers, software modules, ideas) as a native, live-updating knowledge graph.

This is a **feature-complete beta**. The core primitives, daemons, Tracker replacement, and plugins have been dogfooded daily over the past several weeks. While underlying schemas and type nomenclatures will be refined before the final 1.0 release, the relational desktop is here, fully functional and ready to test.

---

## Quick Install & Upgrade

### Fresh Installation

```sh
# 1. Add the official SEN Labs repository (once)
pkgman add-repo https://kiri.sen-labs.org/x86_64

# 2. Install the core system, plugins, Tracker integration, and filetypes
pkgman install sen_core sen_plugins akita sen_filetypes

# 3. Optional: Install additional domain ontologies
pkgman install sen_ontology_people sen_ontology_books sen_ontology_papers \
               sen_ontology_movies sen_ontology_music sen_ontology_sourcecode
```

* **Starting SEN:** The core server starts automatically on your next login (or run `sen start` immediately). You will see the **SEN Knot** icon appear in your Deskbar.
* **Activating Akita:** Akita replaces standard Tracker on next login. You can switch immediately without logging out:
  * `akita start` — switch to Akita immediately.
  * `akita disable` — safely revert back to standard Haiku Tracker.
  * `akita restart` — restart the Akita process.

> [!IMPORTANT]
> **Upgrading from the Preview:**  
> The package manager sorts `1.0.0~beta1` *before* `1.0.0~preview1`. As a result, `pkgman update` will **not** automatically detect this beta. To upgrade, remove your preview packages first and install the beta afresh:
> ```sh
> pkgman uninstall sen_core sen_plugins akita sen_filetypes
> pkgman install sen_core sen_plugins akita sen_filetypes
> ```
> *Don't worry:* Your existing relations and metadata will **not** be lost. Relations are stored directly as native BFS extended attributes on your files.

---

## Architectural Overview

SEN is designed around clean, decoupled components that work in harmony with Haiku's native kit architecture:

| Package | Role & Capabilities |
| :--- | :--- |
| **`sen_core`** | **The Backbone:** Background server managing relations, classifications, active contexts, kernel Node Monitor subscriptions, and the `sen` CLI. Houses the Deskbar Knot. |
| **`akita`** | **The Relational Tracker:** A customized fork/extension of Haiku Tracker that renders relations as live folders, enables relational drag-and-drop, and manages context menus. |
| **`sen_oni`** | **Ontology Installer:** Daemon and utility that monitors package activations, dynamically registering MIME types, icons, and BFS indices. |
| **`sen_plugins`** | **SENSEI Engine:** Extensible pipeline that reads document ASTs and metadata (PDF TOCs, Markdown frontmatter, C/Rust/Go/Python `#include` trees) into live relations. |
| **`sen_filetypes`** | Extended FileTypes editor with searchable indexed attributes and an inspector for ontology hierarchies. |
| **`sen_ontology_*`** | Pluggable domain models (Core, Books, Papers, People, Source Code, Movies, Music) as modular Haiku packages. |

---

## Core Primitives: How SEN Works

### 1. Folders as Dynamic Relational Projections
In SEN, opening a relation does not open a directory on disk; it opens a **relational projection window** in Akita:
* **`Open related...`:** Opens a unified window displaying every relation anchored to the current file (both user-defined associations and implicit structures, such as a PDF's table of contents).
* **Target-as-File:** Each relation is rendered as an entry named after its target entity, with relational metadata (relationship type, role, timestamps, annotations) displayed across Tracker columns.
* **Direct Manipulation:**
  * **Add a relation:** Drag any file into an open relation window (creates a generic reference).
  * **Relate on the fly:** Hold `Command` while dragging one file onto another. Hold `Shift + Command` to be prompted for a custom relation label.
  * **Edit relation details:** Edit attribute columns directly inline.
  * **Sever a relation:** Deleting an item inside a relation window removes the association, not the underlying target file.
  * **Deep Linking:** **Dragging a file out of a relation window creates a deep link copy** that preserves the exact relational context (including page numbers or document sections) while leaving the original relation intact.
  * **Cascade Safety:** Deleting a file that holds active relations prompts you first, giving you full control over cascade cleanup.

### 2. Classifications & Contexts
* **Associative Tagging:** Use *Associate with...* to attach topics, concepts, or collections to any entity. Clicking a classification opens a live BFS query containing all participating files.
* **Sticky Contexts (Shift + Click):** Setting an active context via the Deskbar Knot (e.g., `Project: SEN 1.0`) makes it **sticky**. Every file, note, or entity created during that session is automatically tagged with that context, eliminating manual filing entirely.

### 3. The SENSEI Plugin Architecture & Paperboy
SEN doesn’t rely purely on manual filing—it understands file structure through background SENSEI plugins:
* **Code Graphs (`sen_plugin_sourcecode`):** Automatically maps dependency and `#include` trees across **C/C++, Python, Go, Rust, Java, JavaScript, and TypeScript**, surfacing connected modules in dedicated relation menus.
* **Paperboy (`sen_plugin_papers`):** A dedicated researcher's companion. Drop a scientific PDF into Haiku; Paperboy parses its DOI, fetches metadata and open-access mirrors from **OpenAlex** and **Unpaywall**, and extracts its bibliography. Cited papers are surfaced as native *citation* relations (complete with publication years) that link directly to local copies on your machine if you have them.

### 4. Native Domain Ontologies
Ontologies in SEN are lightweight `.hpkg` packages. When installed, `sen_oni` registers their MIME signatures, attribute indices, and *New...* template items into Haiku's Tracker.
* Ontologies enforce **semantic integrity**: a *movie review* relation only permits connections to movies/videos; a *bibliographic quote* relation only targets texts.
* Domains can be mixed and matched freely: track professional associations with `sen_ontology_people`, source dependencies with `_sourcecode`, and academic citations with `_papers`.

---

## System Requirements & Operational Limits

* **Operating System:** Haiku R1/beta6 on `x86_64`.
* **File System:** Native **BFS volumes only**. SEN stores all graph edges, indices, and metadata inside native BFS extended attributes (`attr_t`).
  * *Volume Bounds:* Moving a file across volume boundaries using terminal commands (`mv`) rather than Tracker drag-and-drop may strip extended attributes if the target volume is non-BFS.
* **Plugin Priority:** Currently, plugin pipeline precedence is determined by file format (e.g., the standard PDF extractor extracts text, while Paperboy enriches metadata). A user-configurable ordering system is planned.
* **Computed vs. Materialized Relations:** Relations computed on-demand by plugins (such as source code `#include` trees) appear inside their respective dedicated submenus rather than the root relation window.

---

## What Will Change Before 1.0 Final

This beta marks the freeze of our functional feature set, but type systems and UI ergonomics will undergo one final architectural polish:

1. **Unified Semantic Type Registry (`semantic-types.md`):**  
   We are decoupling file formats from semantic meaning. A file's MIME type will remain its physical container format (`application/pdf`, `text/plain`), while its semantic role will be tracked via a unified registry identifier (`META:TYPE`). This ensures seamless cross-platform parity with our experimental macOS bridge.
2. **Type Renaming & Migration:**  
   Ontology type names will be streamlined. Once the final schema lands in the next milestone, automatic migration scripts will be provided for your existing attributes.
3. **Akita Rework:**  
   The next release will merge deep architectural enhancements into Akita, making relational navigation even more fluid across spatial desktop views.

---

## Feedback & Community

SEN is an open research project built out of deep appreciation for alternative computing models and the Haiku platform. Report issues, share workflows, and inspect the code:

* **Issue Tracker & Source:** [https://github.com/sen-laboratories](https://github.com/sen-laboratories)
* **Repository Mirror:** `https://kiri.sen-labs.org/x86_64`
* **Community Discussion:** Haiku Community Forums & r/haikuOS
