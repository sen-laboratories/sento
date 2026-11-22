# SEN 1.0 Beta 8 "Sofia": Time and Events

Beta 6 brought places and spatial relations, and Beta 7 connected them to people with privacy-first ambient location awareness. Beta 8 completes the foundational picture with **time and events**: introducing the **Temporal ontology** (`Event` and `Calendar` entities), an offline **Calendar Extractor** for `.ics` files, a dynamic **Temporal Navigator** powered by Allen's interval algebra, date and time controls in **Shoji**, and spreadsheet-style in-place list editing in **Akita**.

As always, SEN is built natively for **Haiku R1/beta6 nightlies (x86_64)** (from `hrev60299_1` onward), storing all semantic metadata directly inside native BFS file attributes.

---

### Time, Events, and the Temporal Ontology

The new **Temporal ontology** turns your desktop into an integrated, searchable schedule:
* **Dual Exact and Fuzzy Time (EDTF)**: Timestamps (`schema:startDate`, `schema:endDate`) are stored as native 64-bit integers for instant BFS indexing, Tracker queries, and accurate column sorting across historical dates (including pre-1970 and post-2038). Imprecise dates (*"around May 1994"*, *"2020s"*) are captured alongside using the standard Extended Date/Time Format (EDTF, `1994-05~`, `202X`), with earliest and latest bounds calculated automatically. Any file on your system can carry start and end attributes.
* **Comprehensive Metadata**: Events track title, description, start/end times, duration, location, organizer, attendees (with attendance status), online meeting links, and recurrence rules.
* **Curated Event Types**: Search-as-you-type autocomplete offers common event types (Meeting, Birthday, Conference, etc.), now expanded with formal calendar designations: **Holiday, Public holiday, National day, Religious holiday, Festival**, and **Observance**—all mapped to Wikidata entities.
* **Native Vector Icons**: Custom HVIF vector icons for Events and Calendars.

---

### Local-First Calendar Extractor

Select any `.ics` or legacy `.vcs` file (exported from Google Calendar, Outlook, Apple Calendar, Thunderbird, Nextcloud, etc.) in Akita and choose **Enrich**:
* **100% Offline & Resilient**: A standalone C++ parser (backed by 230+ tests) handles folded lines, byte order marks, quoted-printable encoding, legacy vCalendar 1.0 syntax, and complex recurrence rules (daily to yearly, intervals, counts, and exceptions) completely locally. Nothing touches the network.
* **Automatic Organization**: Files become **Event** entities (or **Calendar** containers for multi-event files), are automatically renamed using customizable patterns (e.g. `2026-11-12_0930_Planning.ics`), and save detailed descriptions as companion **Markdown files** (`.md`).
* **Deep Relations**:
  * **Attendees**: Linked to `Person` entities in your Library by email, noting their roles and attendance status (with yourself identified via the configurable *Myself* setting).
  * **Takes place at**: Linked to physical `Place` entities in your Library.
  * **Online Meetings**: Automatically extracts meeting links and identifies services (Zoom, Google Meet, Teams, Webex, Jitsi, GoToMeeting, Whereby, Skype).
  * **Series**: Links preceding and upcoming occurrences calculated from recurrence rules.
* **Materialise Events**: Dragging individual events out of a calendar's relation view or choosing *Copy to Library* instantly creates standalone, fully enriched event files.

---

### The Temporal Navigator

Choose *Open related… › Temporal* on any file with a timestamp to explore chronological relationships through **Allen's Interval Algebra**:
* **Dynamic Chronological Relations**: Evaluates **Between** (preceding/succeeding), **Meets**, **Overlaps**, **During**, and **Equals** (plus their inverses) on the fly across all mounted BFS volumes. Relations are calculated dynamically on demand and never stored statically.
* **Fuzzy Interval Safety**: Imprecise dates evaluate across their broadest possible bounds to prevent false positive classifications.

---

### Akita: Spreadsheet-Style List Editing & Desktop Polish

Akita (SEN's native Tracker fork) gains major usability refinements:
* **Spreadsheet-Style In-Place Editing**: Clicking anywhere in an editable cell immediately highlights the cell and begins inline editing, while fast double-clicks still open files. Pressing **Enter** commits changes; **Escape** cleanly reverts to the original value.
* **Type-Aware Controls**:
  * **Checkboxes**: Boolean attributes render clean, aligned checkboxes with proper read-only states.
  * **Calendar Dropdown**: Date and time cells feature an integrated calendar dropdown for picking dates, times, and toggling "All day". Freeform typing (`2026-11-12 09:30`, `1994-05~`) is fully validated.
  * **Autocomplete Popups**: Attributes with defined vocabularies (e.g. `Event Type`) present a live autocomplete dropdown as you type, with prefix matches prioritized and matching characters in **bold**.
* **Visual Applicability (`n/a`)**: When an attribute column does not apply to a file's semantic type, Akita displays a dimmed, non-editable **n/a** instead of an ambiguous blank cell.
* **Native Identity & Vector Icons**: Deskbar now officially displays **Akita**, accompanied by hand-crafted pixel icons (16/32px) and native HVIF vector artwork.
* **Seamless Update Notifications**: Running Akita instances automatically detect package updates after `pkgman update` and prompt to restart the desktop cleanly.
* **Find by Time**: Query files with *before*, *after*, and *is* using the calendar picker, running directly against native 64-bit integer BFS indexes.

---

### Shoji: Forms, Dates, and Recurrence

* **Dedicated Event Form**: Displays all event fields with direct-launch links for places and meeting URLs.
* **Date & Time Controls**: Locale-aware date formatting, popup calendar, time input, and a unified **All day** toggle. Intelligent boundary validation ensures end dates never precede start dates.
* **Advanced Recurrence Editor**: Common presets (Daily, Weekdays, Weekly, Monthly, Yearly) plus a **Custom…** cycle dialog for custom intervals (every N days/weeks/months/years), specific weekdays (starting on your locale's first day of the week), and stop conditions (never, after N occurrences, or until a date). Plain language syncs automatically with standard iCalendar `RRULE` strings (`SEN:recurrence`).
* **Settings Engine Upgrades**: Autocomplete popups, master/detail table bindings (`follows_table`), and non-blocking action buttons with "Waiting for reply…" states.
* **Localization Ready**: All UI strings are cataloged and ready for translation via Haiku's Locale Kit.

---

### Places: Prioritized Overpass Mirrors

The Spatial Navigator now supports a prioritized, configurable list of OpenStreetMap **Overpass API servers**:
* Drag and drop to reorder fallback mirrors. If a busy server times out, queries automatically fall back to the next available mirror.
* Pre-configured public servers plus support for authenticated mirrors (like overpass.ch), complete with an inline **Get key…** shortcut to provider sign-up pages.
* Live server test buttons display real-time HTTP status indicators.
* Spaces prioritizes city, region, and country attributes from existing file metadata, while failed tile lookups are cached to avoid redundant network hits.

---

### Instant Contexts

The SEN Deskbar menu now places **New…** at the top of the **Context** submenu. Selecting it creates a new context, activates it immediately, and opens its folder in Akita with the title field ready for typing. Renaming an active context keeps it active seamlessly.

---

### Upgrading from Beta 7

Stop SEN services while updating packages, then restart them:

```sh
sen stop
pkgman update
sen start       # Akita will prompt to restart automatically if running
pkgman install sen_plugin_calendar_extractor sen_plugin_temporal_navigator sen_ontology_temporal
```
*(Updating `sen_plugins` pulls in the calendar extractor and temporal navigator automatically.)*

New to SEN? Run:
```sh
pkgman add-repo https://kiri.sen-labs.org/x86_64
pkgman install sen_core sen_plugins akita sen_filetypes shoji basho
```
along with your desired ontologies. Requires a Haiku **nightly from hrev60299_1 onward** on BFS volumes.

---

**Links:** [Release Notes](https://github.com/sen-laboratories/sento/releases/tag/v1.0.0-beta.8) · [Source Code](https://github.com/sen-laboratories) · Package Repository `https://kiri.sen-labs.org/x86_64` · Support SEN Labs on [Liberapay](https://liberapay.com/sen-labs/).

We'd love to hear your thoughts and feedback! What calendar or scheduling workflows would you like to see next on Haiku?
