<!--
Draft of the post for the Haiku Community forum (discuss.haiku-os.org). Not published yet. Suggested place: a new topic in "Third-Party" (like the announcement
of 2023: https://discuss.haiku-os.org/t/announcement-sen-personal-knowledge-graphs-book-featuring-haiku/13582), with a link from the thread about the website
(https://discuss.haiku-os.org/t/sen-finally-gets-the-website-it-deserves-rfc/16819). The images are the ones of the release notes (tag v1.0.0-beta.1).
-->

# SEN 1.0 Beta 1: files as a knowledge graph on BFS, now installable with `pkgman`

Some of you followed the first announcement of SEN in 2023. Today the first public beta is here, for **Haiku R1/beta6 (x86_64)**.

**What it is:** SEN (Semantic Extensions Native) relates files to each other and to things (people, books, papers, ideas) and keeps all of it in BFS attributes of your files: no database, no sidecar files. Relations are shown by Akita, our Tracker with a few additions, as folders: a relation is a file named after its target, its properties are the columns, and the usual actions work (drag a file in to add a relation, delete a file to remove it, edit a column to change it). Drag a file out of a relation window and you get a deep link to the target (a page in a PDF, a section) while the relation stays.

![Relations of a PDF](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.1/docs/images/beta1/relations-pdf.png)

**What is in the beta**

* the SEN server with a menu in the Deskbar (contexts, classifications, plugins, ontologies, info and a consistency check)
* *Open related...* (all relations of a file in one window, also what a PDF contains: its outline) and *New related...* / *Associate with...*
* ontologies as packages (people, books, movies, music, papers, source code), with entities in the *New* menu, columns and queries for their attributes
* plugins that find relations and enrich files: PDF and Markdown outlines, the `#include`s of C/C++, Python, Java, Go, Rust, JavaScript and TypeScript, book metadata from OpenLibrary, papers from OpenAlex and Unpaywall with their citations
* a FileTypes that shows the attributes of the types of the ontologies

![The SEN menu](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.1/docs/images/beta1/menu-sen.png)

**Install**

```sh
pkgman add-repo https://kiri.sen-labs.org/x86_64
pkgman install sen_core sen_plugins akita sen_filetypes
pkgman install sen_ontology_people sen_ontology_books sen_ontology_papers   # and the others, as you like
```

The server starts with the next login (`sen start` starts it now); Akita replaces Tracker at the next login (`akita start` / `akita disable` / `akita restart`). If you tried the preview: `1.0.0~beta1` sorts before `1.0.0~preview1` for pkgman, so remove the preview packages first (the exact commands are in the release notes); your relations are attributes of your files and stay.

**Good to know:** it is a beta, used for some weeks by one person: expect rough edges. It works on BFS volumes only. The names of the types of the ontologies are going to change with 2.0 (the types become identifiers of a registry, the same ones as on the macOS version that is being worked on), which is why this is a beta and not a 1.0.

**Links:** [release notes](https://github.com/sen-laboratories/sento/releases/tag/v1.0.0-beta.1) · [source](https://github.com/sen-laboratories) · package repository `https://kiri.sen-labs.org/x86_64`

I am happy about every report, from "it does not install" to "I would use it for...". If you want to try it, tell me what you tried to relate first.
