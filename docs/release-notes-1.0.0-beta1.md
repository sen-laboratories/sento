<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN 1.0.0 Beta 1 "Sofia" — *The Knowledge Desktop*

> *"The human mind [...] operates by association."* — Vannevar Bush, "As We May Think", 1945

Thirty-five years after the seminal *Semantic File Systems* paper at SOSP '91, and rooted firmly in the file-system lineage of BeOS and Haiku, **SEN (Semantic Extensions Native)** enters its first public beta on **Haiku R1/beta6 (x86_64)**.

Traditional operating systems treat folders as physical filing boxes: an entity is bound to a single parent path (`path = identity = location`). If a document belongs to three projects, you are forced into an artificial choice or left to juggle fragile symlinks.

**SEN replaces physical containment with dynamic relational projections.**  

Your files remain exactly what they are — standard files on standard BFS volumes with native attributes. But instead of living isolated in rigid trees, files are now linked to each other and to abstract entities (people, books, papers, software modules, ideas) as a native, live-updating knowledge graph.

Self-relations allow you to navigate "inside" a document, following internal references and outlines, like in a PDF document:

<img src="https://raw.githubusercontent.com/sen-laboratories/sen-core/v1.0.0-beta.1/images/screenshots/document-self-relations.png" alt="Document self relations" width="760">

This 1.0 beta is **feature-complete and self-contained**. It represents my vision of BFS-native relational computing. At the same time, this is only the first step on a bigger journey, which will continue with SEN 2.0 "Gnosis", which is currently being worked on at a much quicker pace, now that the foundation has been laid.

SEN uses the native MIME based FileTypes for representing semantic Entities and Relations, which are packaged, versioned and managed with native Haiku packagefs as Ontologies.

<img src="https://raw.githubusercontent.com/sen-laboratories/sen-core/v1.0.0-beta.1/images/screenshots/sen-ontology-menu.png" alt="SEN Ontologies" width="420">

## Plugins

SEN implements plugins as standard Haiku applications that are discovered dynamically by their application signature (`application/x-vnd.sen-plugin.*`), the plugin type in `META:TYPE` and feature flags like `extract`, `enrich`, `navigate`, etc.

<img src="https://raw.githubusercontent.com/sen-laboratories/sen-core/v1.0.0-beta.1/images/screenshots/sen-plugins.png" alt="SEN plugins" width="680">

This allows for an easy to extend, loosely coupled, native system of plugins to handle relation extraction, navigation and semantic enrichment (Book files via OpenLibrary metadata, papers via OpenAlex, etc.), where metadata is consistently stored *only* in standard extended attributes using well established terminology from schema.org, Dublin Core, FOAF, etc.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.1/docs/images/beta1/document-relations.png" alt="Document relations" width="720">

This works for Entities as well as Relations, which can be opened just like any other folder. These "materialized" relations are represented as normal files with the specific Relation FileType, so relation properties from the file's attributes are intuitively displayed as properties in Tracker's Detail columns.

These properties can even be edited for writable Relations, just like other file properties like Name, and the relation is updated on-the-fly.

---

## Quick Install & Activation

```sh
# 1. Add the official SEN Labs repository (once)
pkgman add-repo https://kiri.sen-labs.org/x86_64

# 2. Install core, plugins, Akita (Tracker), and SEN FileTypes
pkgman install sen_core sen_plugins akita sen_filetypes

# 3. Optional: Install domain ontologies
pkgman install sen_ontology_people sen_ontology_books sen_ontology_papers \
               sen_ontology_movies sen_ontology_music sen_ontology_sourcecode
```

* **Starting SEN:** The core server starts on next login (or run `sen start` now) and installs the **SEN Knot** into your Deskbar tray.
* **Activating Akita:** Akita replaces stock Tracker on next login. You can toggle it live at any time:
  * `akita start` — switch to Akita immediately.
  * `akita disable` — safely revert to stock Haiku Tracker.
  * `akita restart` — restart the Akita process.

> [!IMPORTANT]
> **Upgrading from the Preview:**
> 
> (For those curious souls who found the SEN Labs Kiri repository already, you know who you are;-)
>
> The package manager sorts `1.0.0~beta1` *before* `1.0.0~preview1`. As a result, `pkgman update` will **not** automatically detect this beta. To upgrade, remove your preview packages first and install the beta afresh:
> ```sh
> pkgman uninstall sen_core sen_oni sen_plugins akita sen_filetypes \
>     sen_plugin_book_enricher sen_plugin_markdown_extractor sen_plugin_mime_extractor sen_plugin_paperboy \
>     sen_plugin_pdf_extractor sen_plugin_pdf_navigator sen_plugin_reference_navigator \
>     sen_plugin_relation_navigator sen_plugin_sourcecode_extractor sen_plugin_text_navigator
> pkgman install sen_core sen_plugins akita sen_filetypes
> ```
> The ontology packages are updated by the usual `pkgman update`.
> *Your data is safe:* Relations and metadata are stored directly as native BFS extended attributes on your files and remain completely untouched.

---

## The Deskbar Knot: Your System HUD

The **SEN Knot** in the Deskbar tray serves as the central cockpit for relational computing on Haiku:

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.1/docs/images/beta1/menu-sen.png" alt="SEN Deskbar Knot" width="200">

```
[ SEN Deskbar Knot ]
 ├── Active Context Switcher  ──► Select context / [Shift+Click] for Sticky
 ├── Classifications          ──► Hierarchical browser for Topics, Labels, Concepts
 ├── Ontologies               ──► Browse registered types, attributes & templates
 ├── Plugins (SENSEI)         ──► Live query of installed plugins, versions & About
 ├── Info & Check             ──► Server status, plugins, types with their file counts, consistency check
 └── Settings                 ──► System integration preferences
```

* **Context Switcher:** Switch your working context on the fly. **Shift + Clicking** any context or classification makes it **sticky**—every new file, note, or document created anywhere in the system is automatically tagged with that context until unset.
* **Browsable Classifications & Ontologies:** Instantly explore your personal knowledge graph by navigating through cascading menus of types and labels without opening a single file-search window.
* **Plugin Overview:** See all installed SENSEI plugins with their version and features in a query window, and open the About window of each one.
* **System Info & Integrity Check:** Inspect the running state of `sen_core`, the SEN objects on each volume, the installed plugins and the registered types with their number of files, and run a consistency check (missing indices, duplicate IDs, types that are not installed).
* **SEN Settings Panel:** Configure system-level defaults with one click:
  * **Make Akita the default file manager** over stock Tracker.
  * **Prefer SEN-flavored FileTypes** (with searchable attribute indices and ontology inspectors) over stock Haiku FileTypes.

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

### 2. SENSEI Plugins

SEN understands document structure natively through background SENSEI plugins: (SENSEI stands for "Semantic Extraction, Navigation, Search, Enrichment and Identification")

* **BERT, the Book Enrichment Tool** (`sen_plugin_book_enricher`): searches OpenLibrary for metadata as provided by the file's attributes, and retrieves the full metadata including the book cover, which is stored as a thumbnail image.
* **Code Graphs (`sen_plugin_sourcecode_extractor`):** Automatically maps dependency and `#include` trees across **C/C++, Python, Go, Rust, Java, JavaScript, and TypeScript**, surfacing connected modules in dedicated relation menus.
* **Paperboy (`sen_plugin_paperboy`):** A dedicated researcher's companion. Drop a scientific PDF into Haiku; Paperboy parses its DOI, fetches metadata and open-access mirrors from **OpenAlex** and **Unpaywall**, and extracts its bibliography. Cited papers are surfaced as native *citation* relations (complete with publication years) that link directly to local copies on your machine if you have them.
* and many more...

### 3. Native Domain Ontologies

Ontologies in SEN are lightweight `.hpkg` packages. When installed, `sen_oni` registers their MIME signatures, attribute indices, and *New...* template items into Haiku's Tracker.

* Ontologies enforce **semantic integrity**: a *movie reference* relation only permits connections to movies and videos; a *book quote* relation only targets books (as entities, PDF or EPUB files).
* Domains can be mixed and matched freely: track professional associations with `sen_ontology_people`, source dependencies with `_sourcecode`, and academic citations with `_papers`.

---

## System Requirements & Operational Limits

* **Operating System:** Haiku R1/beta6 on `x86_64`.
* **File System:** Native **BFS volumes only**. SEN stores all graph edges, indices, and metadata inside native BFS extended attributes.
  * *Volume Bounds:* Moving a file across volume boundaries using terminal commands (`mv`) rather than Tracker drag-and-drop may strip extended attributes if the target volume is non-BFS.
* **Plugin Priority:** Currently, plugin pipeline precedence is determined by file format (e.g., the standard PDF extractor extracts text, while Paperboy enriches metadata). A user-configurable ordering system is planned.
* **Computed vs. Materialized Relations:** What a document contains (the outline of a PDF) is extracted by a plugin and shown in the root relation window. Relations that plugins compute on demand otherwise, such as the `#include`s of source code files, appear in their dedicated submenus only.

---

## The Roadmap: From 1.0 "Sofia" to 2.0 "Gnosis"

SEN 1.0 is designed to be a complete, stable, self-contained realization of BFS relational computing. Looking further ahead, **SEN 2.0** will introduce our next major architectural leap:

* **Unified Semantic Type Registry (`semantic-types.md`):**  
  In 2.0, we will fully decouple file container formats from semantic meaning. A file's MIME type will reflect its byte format (`application/pdf`, `text/markdown`), while its ontological meaning will be governed by a unified identifier (`META:TYPE`) mapped across an extensible taxonomy tree.
* **Cross-Platform Parity:**  
  The 2.0 type engine will share identical semantics with our experimental macOS implementation, enabling true cross-platform relational consistency.
* **Akita 2.0:**  
  A larger rework of Akita, going its own way beyond the stock Tracker, including some kind of graph view alongside the list and icon views.
* **Migration:**  
  Relations and metadata are native BFS attributes of your files. Where a new release changes them, a migration is provided between official releases (not for previews and betas).

---

## Your support is essential

While standing on the shoulders of giants, with BeOS and Haiku as a solid foundation - the best environment for SEN! - I have invested a significant amount of resources into bringing my passion project to life.

Please consider hitting that "Support" button in the About dialog and contribute to the ongoing development of SEN!

## Feedback & Community

SEN is an open research project built out of deep appreciation for alternative computing models and the Haiku platform. Report issues, share workflows, and inspect the code:

* **Issue Tracker & Source:** [https://github.com/sen-laboratories](https://github.com/sen-laboratories)
* **Package Repository:** `https://kiri.sen-labs.org/x86_64`
* **Community Discussion:** Haiku Community Forums & r/haikuOS
