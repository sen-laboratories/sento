<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN 1.0.0 Beta 5 "Sofia" — *Settings, Library and Movies*

> The most valuable thing a tool can do is to stay out of the way, and to be easy to adjust when it does not.

Beta 4 was about the people behind your files. Beta 5 is about **making SEN yours** and **keeping things in order**: there is a **settings window** for SEN and for every plugin, what plugins make new goes into a **Library** instead of
beside your files, a new plugin, **Akari**, enriches **movies** and makes people of their crew and cast, the **graph is live**, and a number of rough edges of beta 4 are gone.

**SEN (Semantic Extensions Native)** on **Haiku R1/beta6 nightlies (x86_64), from hrev60299 on**, as with beta 4. If you have beta 4 installed, `pkgman update` brings you here (see *Upgrading* below).

> [!IMPORTANT]
> **SEN follows the Haiku nightlies**, as before: the packages of Akita and SEN FileTypes require `haiku >= r1~beta6_hrev60299_1` and set no upper limit, so `pkgman` never holds your Haiku back.
> `abi-check.sh` in the [akita](https://github.com/sen-laboratories/akita) repository compares them with the newest nightly every day; a new build follows when a nightly breaks Akita.

## One place for the settings

Choose **Settings…** in the menu of the SEN item in the Deskbar. The window is a **tree**: **SEN › Core** for the server and, under **Plugins**, one node for each kind of plugin (*Enrich*, *Identify*…) with the plugins that have something to set. The form on the right is the
one that **Shoji** makes for any structured data (the window shows it as a replicant), so settings look and behave like everything else in SEN, and a plugin only has to *describe* its settings; it does not draw a window.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/settings-core.png" alt="The SEN settings window: SEN, Core" width="760">

* **Core** has the settings that were in the small window of beta 4 (the SEN key, Akita instead of Tracker, the FileTypes of SEN) with short names and a tip each, the **name of the Library folder**, and one new setting: *New objects belong to the current context* (below).
* **Plugins** has the settings that are **the same for all plugins**, set once instead of in every plugin: *Extract entities into* (**Files** makes a person file of every author, actor… **Attributes** writes only their names), *Ask which one if several are found*
  (the chooser for a book, a movie, a paper) and *Max. number of entities to extract* (a slider from *None* to *All*: *None* at its left end, *All* at its right end means no limit).

  <img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/settings-plugins.png" alt="The settings of all plugins: where entities go, ask if several are found, how many" width="760">

* **A plugin** with settings of its own has a node of its own. **Paperboy** has its e-mail address and the OpenAlex key (both are optional, they make the servers friendlier), the sources to ask, the renaming of the file after the paper and the name pattern; **Akari** has the TMDB key and the language.
  A plugin says whether it has settings (the attribute `SEN:plugin:hasSettings`), so plugins that have none are not listed. **BERT** has none of its own any more: what it did with people now follows the shared settings.
* The settings are **checked by the plugin**, which can refuse a value and say why (the window shows the reason). A plugin reads its settings again for **every request**, so a change takes effect at once, without restarting anything.
* Secrets (the TMDB key) are **password fields**: you see dots, not the key.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/settings-paperboy.png" alt="The settings of Paperboy" width="760">

## A Library for what SEN makes

Until beta 4 the people of a paper or a book were made **next to the file** they came from, and your Downloads folder filled with people. Now they go into a **Library**:

* The default Library is **`~/Library`** (you can name it otherwise in *SEN › Core*; an existing folder is not moved). It has **one folder for each type of entity, named in the plural**: *People*, *Books*, *Movies*… (the ontologies say how a type is called in the plural).
* **Every context has its own**: the Library of the context *Research* is `~/Library/Research`, with the same folders. (A folder `Contexts` that a development version made is moved up once.)
* An existing person is **found again by name and key** wherever it is (in the Library, or next to the file where beta 4 put it) and updated in place. The Library only decides where **new** ones are made.
* On the command line: `sen --library person` prints the path of the folder for a type (`--create` makes it, `--context <name>` asks for another context).

**New objects belong to the current context.** What you or a plugin makes new (a file from the *New* menu, a relation, the people of an enrichment) now gets an **Association** to the context that is checked in the menu of SEN, as if you had made the context sticky with the Shift key. This is the new
default; **SEN › Core › *New objects belong to the current context*** switches it off, and then only what you made sticky yourself is applied. The default context, which is everything anyway, is never applied. In *Open related…* a context has its objects as **Association** relations, and the Library folder of the context has them as files.

## Akari: movies, their crew and their cast

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/akari-chooser.png" alt="Akari asks which movie is meant" width="600">

**Akari** (*light* in Japanese; the projector in its logo throws light on a file and a graph of people appears) enriches **video files** from [The Movie Database](https://www.themoviedb.org) (TMDB). Choose **Enrich** on a movie in Akita.

* **The movie.** Title and year come from the attributes of the file, else from its name: `Blade Runner 2049 (2017).mp4` and `The.Matrix.1999.1080p.BluRay.x264-YIFY.mkv` are found as what they are. If TMDB finds **several movies**, you choose (the best first, rated by title, year and how well known the movie is; you can switch the question off in the settings).
  The file gets the semantic type **Movie** (its type for Haiku stays what it was), title, original title, release date, **duration**, description, **genres**, languages, countries, links (TMDB, IMDb, Wikidata, Letterboxd) and the **poster as its thumbnail**. What the file has already is kept; *Enrich (Overwrite)* (Shift) replaces it.
* **The people.** The director, the crew (writers, producers, composer, cinematography, editing) and the cast become **person files** in the Library (`~/Library/People`), with birthday, biography, links and **photo**, each with a **Contribution** to the movie: the **role**, for an actor the **character**, and the place in the credits.
  A person with two jobs has two Contributions. A person that exists is updated and not made twice. In the window of the relations such a person is listed once for each Contribution, and **only then** the entries are named by their role:
  *Ang Lee (Director)* and *Ang Lee (Producer)*; a person with one Contribution is named as before.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/akari-contributions.png" alt="The Contribution relations of a movie in Akita: role, character, place in the credits and provenance" width="760">

* **You need a key**, TMDB gives one for free (*Settings › API* on themoviedb.org). Both the API key and the *API Read Access Token* are accepted. Put it into *Settings › Plugins › Enrich › SEN Akari*. The plugin says so if the key is missing, and does nothing else.
* The ontology of movies (**SEN Movies**) follows **schema.org** now (`schema:duration`, `schema:genre`, `schema:director`, `schema:actor`…), and **Movie has an icon** (a video cassette).
* **Genres, countries and links are lists** and are written as a list: with the separator that the attribute says (a comma), so Shoji shows each genre as an entry. Links that an early version wrote with a semicolon are still read.

> This product uses the TMDB API but is not endorsed or certified by TMDB. The data and the images are TMDB's.

**Paperboy** and **BERT** make their people in the Library as well, follow the shared settings, and need the *People* ontology (it is a requirement of their packages now).

## Shoji

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/shoji-movie.png" alt="Shoji showing a movie" width="520">

* **A form for any message.** Shoji can show **every value of any BMessage in one outline and edit it in place** (the message inspector), and the same form is used for settings. It knows **sliders with labels** at the ends, **password fields** and now **durations**.
* **Durations are real times.** `PT2H16M` (the ISO 8601 form that schema.org uses) is shown and typed as **`02:16:00`**; what you type is checked and stored as ISO 8601 again.
* **Dates:** the date control has the room for its arrows now, and the arrows are the ones that **list views** use.
* **Read-only fields look like it.** A field that cannot be changed is really read-only, with its **label as readable as the others** (the field is greyed, not the label). This is a rule for all templates.
* The **enlarged picture** (a click on the picture of a form) closes with its window.
* *Associations:* a type counts as having an application if the system opens it with one.

## The graph is live

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/graph-movie.png" alt="The graph centred on a movie: its crew and cast" width="760">

* **It follows the files.** The graph watches the files of its objects (their SEN attributes and names) and a live query follows **new and removed** objects. What changed is read again after a moment of quiet (so an enrichment that writes twenty people does not make it flicker), and **zoom, place and selection stay**.
* **A file with no relations yet** that you asked for is found as soon as it gets them. If it has none, the window says **"<name> has no relations."** (and not that it is no object of SEN).
* The file in focus is found **by its id**, read from the file, also after a rename.

## Smaller, but you will notice

* **Restart Akita** is an item of the Deskbar menu (next to *Restart*), when Akita is installed. It is the same as `akita restart` and applies what a new Akita brings.
* **Enrichment says that it has begun.** Akari, Book Enricher and Paperboy show a notification when the lookup starts ("Enriching movie"), not only when it is done; the result replaces it in the same place.

  <img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/akari-notify-start.png" alt="Akari: Enriching movie" width="300"> <img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.5/docs/images/beta5/akari-notify-done.png" alt="Akari: Movie enriched" width="300">

* **Installing ontologies shows a notification** with the progress and a message when it is done.
* **Thumbnails in every size.** A poster or cover now shows as the icon in *all* sizes of the icon view, not only in the one that was selected when the file was enriched: the thumbnail is scaled to the size of the icon (**Akita** has the fix in Tracker's code; senryu 6b0b271e31), and plugins write the creation time of the thumbnail *before* the picture, so the icon is refreshed when it changes.
* **Every relation says who made it.** The new relation property **Provenance** (`prov:wasAttributedTo`, from PROV-O) is the signature of the program that made the relation (a plugin, Akita...), set by the server from the sender of the request. It is a column in the relation windows. A relation from before gets it when it is made again (an *Enrich* of the file does that); one that has it keeps its first creator.
* **A related file that is gone is shown as such.** In a relation window it keeps its entry with the **broken-link icon** that Haiku uses for a broken symbolic link (it is named after what the relation knew of it), and the *Open related* menus list it as a disabled entry, so a dangling relation is not mistaken for no relation.
* **New files get the sticky classifications at once**, and the *Contexts* folder of the Library is gone (see above).
* BERT no longer asks for a setting that only it had; the *SEN key* setting has short names with a tip.

## Under the hood

* **The settings protocol** (spec: `docs/spec-settings-and-message-template.md`): path keys for nested messages (`SenSettingsPath.h`, `SenMessagePath.h`), a form schema (with `min_label` and `max_label` for sliders), values as text for points, rectangles and colours, `kSettings` to the plugin and `kSettingsReply` back, `SenSettingsForm.h` for the host side. `sen_settings` is an application of its own that the server starts.
* **Ontologies** (`0.1.0-32`): a type has its **plural** (a slot of the root class *Entity*, written as the resource `SEN:PLURAL` when the type is installed), the plugin type has the attribute `SEN:plugin:hasSettings`, *Movies* follows schema.org and has the Contribution relation. The installer runs once more after the update so that the plurals are in place.
* **For developers** (sento): `SenPluginSettings.h` (`sen::plugins::LoadSharedSettings()`), `SenLibrary.h` (`sen::library::FolderFor(type, &folder)`), `SenStripeDialog.h`. The developer guide is updated.

## Packages

31 packages in the repository **Kiri**, among them:

* `sen_core`: `1.0.0~beta5-8`, `sen_oni`: `1.0.0~beta5-3`, `akita`: `1.0.0~beta5-3`, `shoji`: `1.0.0~beta5-7` (`sen_filetypes` stays at `1.0.0~beta4-1`: nothing changed there).
* The seven ontology packages are at `0.1.0-32`.
* All plugin packages are `~beta5`, and `sen_plugins` (`1.0.0~beta5-2`) requires them all. **New:** `sen_plugin_movie_enricher` (Akari).

## Upgrading from Beta 4

Stop SEN while the packages are replaced and start it again afterwards:

```sh
sen stop
pkgman update
sen start       # and, to have the new Akita:  akita restart
```

The first start with beta 5 installs the ontologies again (the notification tells when it is done). The people that earlier versions made next to your files are found and updated where they are; only new ones go into the Library. New here? See *Quick Install* in the notes of beta 3:
`pkgman add-repo https://kiri.sen-labs.org/x86_64`, then `pkgman install sen_core sen_plugins akita sen_filetypes shoji` and the ontologies you like.

## Known limitations

* **Akari needs a TMDB key** (free). Without it nothing is enriched, and the plugin says why. TMDB's data are in many languages; the one that you ask for in the settings is used where TMDB has it, and the original title is kept.
* **The graph** watches up to 1500 files and reads up to 2000 objects; with more, it says so and does not follow the rest live.
* **Semantic Scholar and DBLP** in Paperboy are still not tested live (as in beta 4); OpenAlex and arXiv do the work.
* BFS volumes only, as before. A Haiku older than hrev60299_1 cannot be used with this beta (Akita would not load there).

## What comes next

Planned besides bug fixes and the feedback on beta 5:
* templates for more types,
* the first release candidates for the 1.0 train.

In parallel SEN 2.0 "Gnosis" (the unified registry of semantic types, an Akita that is an information browser with Shoji and the graph built in) moves ahead.

## Your support is essential

Please consider hitting the "Support" button in the About dialog and contribute to the ongoing development of SEN, or simply donate right from here to [SEN Labs Liberapay](https://liberapay.com/sen-labs/).

## Feedback & Community

* **Issue Tracker & Source:** [https://github.com/sen-laboratories](https://github.com/sen-laboratories)
* **Package Repository:** `https://kiri.sen-labs.org/x86_64`
* **Community Discussion:** Haiku Community Forums
