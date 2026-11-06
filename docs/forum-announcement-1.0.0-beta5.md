<!--
Draft of the post for the Haiku Community forum (discuss.haiku-os.org). Not published yet. Suggested place: a reply in the topic of the Beta 1/2/3/4 announcements
(same topic, "Third-Party"). The images are the ones of the release notes (tag v1.0.0-beta.5). Drafts are not committed.
-->

# SEN 1.0 Beta 5: Settings, a Library and Movies

Beta 4 brought the people behind your files into focus. Beta 5 is about **making SEN yours**, **keeping things in order**, and **movies**. Still for **Haiku R1/beta6 nightlies (x86_64)**, and still storing everything natively in file attributes.

**One central settings window.** Choose *Settings…* from the SEN Deskbar menu to open a unified preferences center: **Core** for server and system options, **Plugins** for shared enrichment rules (extracting to files vs. attributes, interactive disambiguation prompts, and extraction limits), plus dedicated sections for plugins with custom options (Paperboy, Akari). The forms are hosted natively by Shoji as embedded replicants, so plugins never have to draw custom windows, and settings take effect immediately.

![The SEN settings window](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/settings-plugins.png)

**A Library for what SEN makes.** Extracted entities (authors, actors, directors) no longer clutter the folder containing your source files: they are organized into **`~/Library`**, with structured folders per entity type (*People*, *Books*, *Movies*), and dedicated sub-libraries for each context (`~/Library/<context>`). Furthermore, newly created objects now **automatically join the active context** by default (configurable in Core settings).

**Akari enriches movies.** Select a video file in Akita and choose *Enrich*: Akari identifies the title and year, lets you pick from scored candidates if multiple titles match, and pulls rich metadata from The Movie Database (TMDB). The file receives the schema.org-compliant **Movie** type, duration, genres, web links, and poster artwork as its thumbnail. Key crew and cast become **Person files** with headshots and bios in `~/Library/People`, connected through detailed **Contribution** relations tracking their specific roles and characters.

![The Contribution relations of a movie, with role and provenance](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/akari-contributions.png)

**Live graph & Shoji polish.** The relationship graph is now **live and reactive**: it watches files and listens to live queries, automatically reflecting modifications, additions, and deletions while preserving your zoom level, pan position, and selection. **Shoji** gains a universal BMessage inspector and form host, formats ISO 8601 durations as human-readable time (`02:16:00`), masks password fields, and refines date and slider controls.

![The graph centred on a movie](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/graph-movie.png)

**Desktop integration & system polish:**
* **Restart Akita** directly from the SEN Deskbar menu after package updates.
* **Real-time enrichment feedback**: Desktop notifications appear as soon as a lookup begins and update in place when done.
* **Tracker thumbnail scaling**: Poster and cover thumbnails now scale cleanly across all icon sizes.
* **Missing relation indicators**: Targets that have moved or been deleted are displayed with Haiku's standard broken-link icon rather than silently disappearing.
* **Relation provenance**: Relations record their creator (`prov:wasAttributedTo`), visible as a dedicated column in relation views.

**Upgrade** (stop SEN while packages are updated):

```sh
sen stop
pkgman update
sen start       # and, to reload Akita:  akita restart
```

New here? `pkgman add-repo https://kiri.sen-labs.org/x86_64`, then `pkgman install sen_core sen_plugins akita sen_filetypes shoji` and the ontologies you like. Requires a Haiku **nightly from hrev60299_1 on**.

**Good to know:** Akari requires a free TMDB API key or Read Access Token (configured in settings). Semantic Scholar and DBLP in Paperboy remain experimental; OpenAlex and arXiv handle primary paper lookups. BFS volumes only; still a beta.

**Links:** [Release notes](https://github.com/sen-laboratories/sento/releases/tag/v1.0.0-beta.5) · [Source code](https://github.com/sen-laboratories) · Package repository `https://kiri.sen-labs.org/x86_64` · Support SEN Labs on [Liberapay](https://liberapay.com/sen-labs/).

Reports and feedback are always welcome. Which file type would you like to see an enricher for next?
