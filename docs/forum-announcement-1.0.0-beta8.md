# SEN 1.0 Beta 8 "Sofia": Time and Events

Beta 6 brought places, Beta 7 people and your own whereabouts. Beta 8 adds **time**: a new Temporal ontology with **Events** and **Calendars**, a **calendar extractor** for `.ics` files, a **temporal navigator**, date and time controls in **Shoji**, and a much nicer way to edit attributes in **Akita**'s list view. Still for **Haiku R1/beta6 nightlies (x86_64)** (from `hrev60299_1` on), and still storing everything natively in file attributes.

**Events, Calendars and the Temporal ontology.** An Event has a start, an end, a duration, a type, a place, an organizer, attendees, a status, an online-meeting link and a repeat rule; a Calendar is the umbrella of the events of one calendar file.
* **Real time attributes, also fuzzy ones**: the start and the end are time attributes (so columns, sorting and queries just work), with the text of a not-exact time (*about May 1994*, EDTF `1994-05~`) kept beside them. Any file may carry a start and an end.
* **Event types**: from *Meeting*, *Birthday* and *Wedding* to the new **Holiday, Public holiday, National day, Religious holiday, Festival** and **Observance**, all mapped to Wikidata. Any other text is fine, too.

**Calendar extractor.** Choose *Enrich* on an `.ics` or `.vcs` file (Google, Outlook, Apple, Thunderbird...):
* A forgiving reader for iCalendar and the old vCalendar 1.0, including repeat rules, exceptions and online-meeting links (Zoom, Meet, Teams, Webex, Jitsi...). Nothing leaves your machine.
* The file becomes an **Event** (or a **Calendar** if it holds several), is **named by its date and time** (`2026-11-12_0930_Planning.ics`, by a pattern you can change) and gets its description as a **Markdown file** beside it.
* **Relations**: Attendee (with role and response; each a Person in your Library), Takes place at, Series (the instance before and after), Description, and Events at a calendar. *Copy to Library* keeps an item of a calendar as an event of its own.

**Temporal navigator.** *Open related › Temporal* at an event shows how it lies in time to the others: **Between** (before and after), **Meets**, **Overlaps**, **During**, **Equals**, the relations of Allen's interval algebra, worked out when you open them and never stored.

**Shoji.** An Event form, a **date and time control** (calendar drop-down, time of day, *All day*), the rule that an end is never before its start, and a **Repeats** popup (every day, week, month, year, or **Custom…** with weekdays and an end) that keeps the technical repeat rule in step. In every form, **Enter** accepts an edit and **Escape** puts the stored value back; lists of common values follow what you type.

**Akita.**
* **Click to edit, as in a spreadsheet**: the whole cell of an editable attribute is the place to click, it is outlined at once and the edit starts faster. **Enter** accepts, **Escape** reverts.
* **Lists of values that follow your typing**: first the values that begin with your text, else those that contain it, with the matching characters in bold; a small arrow opens the whole list.
* **The right control for the kind of attribute**: yes/no attributes are check boxes (greyed where read-only), times and days have a calendar that drops down from the field, and Find asks for times with *before*, *after* and *is*.
* **Update notice**: when `pkgman update` installs a new Akita, a running Akita offers *Restart*.
* **n/a** in the list marks an attribute that does not belong to the file's type, and a click on an event of a calendar opens that event.
* Relations show the icon of their folder, items of a relation that are entities can be copied into your Library, and *Open contained* is greyed out where there is nothing contained.

**Places.** The spatial navigator asks a **list of Overpass servers** in turn (drag to reorder, test each one, API keys where needed), a **Get key…** button opens the page where a free key is made, and Spaces puts the city, region and country of a file before the name.

**Contexts.** **Context › New…** in the Deskbar menu makes a new context, switches to it and opens it in Akita with the name ready to type.

**Upgrade** (stop SEN while packages are updated):

```sh
sen stop
pkgman update
sen start       # Akita offers a restart itself
pkgman install sen_plugin_calendar_extractor sen_plugin_temporal_navigator sen_ontology_temporal
```
*(Updating `sen_plugins` pulls in the two plugins automatically.)*

New here? `pkgman add-repo https://kiri.sen-labs.org/x86_64`, then `pkgman install sen_core sen_plugins akita sen_filetypes shoji basho` and the ontologies you like. Requires a Haiku **nightly from hrev60299_1 on**.

**Good to know:** fuzzy times are typed as text; the date and time controls are for exact ones. Repeat rules the reader does not know give no Series relation. BFS volumes only.

**Links:** [Release notes](https://github.com/sen-laboratories/sento/releases/tag/v1.0.0-beta.8) · [Source code](https://github.com/sen-laboratories) · Package repository `https://kiri.sen-labs.org/x86_64` · Support SEN Labs on [Liberapay](https://liberapay.com/sen-labs/).

Reports and feedback are always welcome. What is the next date you want your desktop to know about?
