<!--
Draft of the post for the Haiku Community forum (discuss.haiku-os.org). Not published yet. Suggested place: a reply in the topic of the Beta 1 announcement (same
topic, "Third-Party"), or a new topic that links to it. The image is the one of the release notes (tag v1.0.0-beta.2).
-->

# SEN 1.0 Beta 2: see the graph of your files

Beta 1 made the knowledge graph of your files something you can *walk* through. Beta 2 lets you *see* it. Still for **Haiku R1/beta6 (x86_64)**, still all in the attributes of your files.

**The graph view.** Every file with a SEN id is a dot, every relation a line; things that belong together stand together (an ontology with its types, a book with its author, a paper with the papers it cites). Colours show the types, the size shows how many relations an object has.

![The graph of SEN in dark mode](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.2/docs/images/beta2/graph-dark.png)

* **Open it** with the SEN key (Command) + click on the knot in the Deskbar, or *Graph...* in its menu. In Akita, Command + double click on a file opens the graph centred on that file.
* Select a dot to see its neighbours, double-click it to open the file; click a line to select a relation, double-click it to open the relations of that type in Akita. Wheel to zoom, drag to move, F to fit.
* Light or dark follows your system colours until you choose (and it remembers). *Save as SVG* saves the window as you see it, *Save selection* saves one object with its direct connections: a clean picture of one thing and what it is related to.

**Also in this beta**

* Ontologies are folders now: opening one in Akita shows what it defines and needs, with a folder-and-bookmark icon, instead of starting FileTypes with some type of it.
* Fixes found while living with beta 1: the attributes in the Ontologies menu of the Deskbar, choosing a target in *Open related...* (it ended in an error dialog), Citation is only offered between books, movies, persons, documents and papers, quieter logs, and Alt+Q in a SEN window no longer makes the knot disappear.
* Packaging: `sen_plugins` can no longer pull in an older plugin from the repository; builds of all plugins and of Akita are one step.

**Upgrade** (the knot in the Deskbar is loaded from the server, so stop SEN while the packages are replaced):

```sh
sen stop
pkgman update
sen start       # and, for the new Akita:  akita restart
```

New here? `pkgman add-repo https://kiri.sen-labs.org/x86_64`, then `pkgman install sen_core sen_plugins akita sen_filetypes` and the ontologies you like (the release notes have the full list).

**One important thing:** Akita follows the Haiku nightlies. It links against private classes of Haiku, and in the nightly **hrev60299_3** `BPrivate::JobQueue` changed (an Akita built before it does not load, and you would be without a file manager). The packages of Akita and FileTypes now require `haiku >= r1~beta6_hrev60299_1` and set no upper limit, so `pkgman` does not hold your Haiku back; `pkgman update` brings the current Akita `1.0.0~beta3-1` together with the Haiku update. A check against the newest nightly runs every day, and a new build follows when one breaks Akita.

**Good to know:** still a beta, BFS volumes only, and the graph shows up to 2000 objects.

**Links:** [release notes](https://github.com/sen-laboratories/sento/releases/tag/v1.0.0-beta.2) · [source](https://github.com/sen-laboratories) · package repository `https://kiri.sen-labs.org/x86_64` · if you like where this is going, you can support SEN Labs on [Liberapay](https://liberapay.com/sen-labs/).

Reports are welcome, from "it does not install" to "I would use it for...". If you try the graph, tell me what it showed you that you did not expect.
