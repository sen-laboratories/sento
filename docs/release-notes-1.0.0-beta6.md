<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->

# SEN 1.0.0 Beta 6 "Sofia" — *Places and Spatial Relations*

> Everything happens somewhere.

Beta 5 was about making SEN yours and organizing your knowledge. Beta 6 introduces the final ontology for the 1.0 line: **SEN Spatial**, bringing first-class support for **places, locations, and spatial relationships**. A new enrichment plugin, **SUESS**, populates places from OpenStreetMap and Wikidata with rich metadata and photography; an intelligent **spatial navigator** dynamically evaluates topological relations (*within*, *touches*, *overlaps*) and discovers **nearby** points of interest; **Shoji** renders places with embedded, styled **maps**; and **Akita** introduces dynamic relation group folders that load on demand.

**SEN (Semantic Extensions Native)** runs on **Haiku R1/beta6 nightlies (x86_64), from hrev60299 on**. If you have beta 5 installed, `pkgman update` brings you here (see *Upgrading from Beta 5* below).

> [!IMPORTANT]
> **SEN follows the Haiku nightlies**: packages for Akita and SEN FileTypes require `haiku >= r1~beta6_hrev60299_1` and set no upper version limit. An automated ABI check (`abi-check.sh` in the [akita](https://github.com/sen-laboratories/akita) repository) verifies compatibility against every new nightly.

## Places and locations

Beta 6 distinguishes between two fundamental geographic concepts:
* A **Place** represents an entity with a name and cultural identity (such as *Karlskirche*, *Vienna*, or *Café Central*).
* A **Location** represents a pure coordinate or geographic point without inherent identity (such as the GPS position of a photo or a temporary meeting point).

Both can be created directly from Akita's **New › Spatial** menu. They share a unified schema (title, aliases, place kind, coordinates, bounding box, boundary outline, street address, web links, and imagery) based on `schema.org`, Dublin Core, and GeoSPARQL vocabularies. As with everything in SEN, all geographic data is stored natively in file attributes—searchable via queries and displayable as Tracker columns.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/new-spatial-file.png" alt="New › Spatial › Location, Place" width="640">

## SUESS: the place enricher

Select a Place or Location in Akita and choose **Enrich**. **SUESS** (*Semantic Urban & Environmental Spaces for SEN*, named in honor of Austrian geologist Eduard Suess) resolves geographic entities using [OpenStreetMap](https://www.openstreetmap.org) (Nominatim) by name, street address, or coordinates:

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/enrich-location.png" alt="OpenStreetMap found several places. Which one is yours?" width="640">

* **Rich place metadata**: Writes place classification, coordinates, bounding box, simplified polygon outlines, postal address, links (OpenStreetMap, Wikidata, Wikipedia), and localized descriptions. If Wikidata recognizes the place, SUESS fetches photography or heraldic crests complete with license and attribution metadata.
* **Geographically scoped search**: If a file already specifies a city, region, or country, queries are automatically constrained to that territory (searching for *Westbahnhof* in *Wien* will never suggest Budapest). Coordinates present on the file are used to rank matches locally without broadcasting raw positions.
* **Smart disambiguation**: When multiple candidates share a name, the candidate chooser provides context—distinguishing subway stations from railway terminals by displaying transport lines (e.g. *U6*) and transit operators (*Wiener Linien*).
* **Respecting privacy**: GPS coordinates embedded in photos are only looked up when *Use GPS location for lookup* is explicitly enabled.
* **Polite public querying**: Operates with built-in rate-limiting (maximum 1 request per second), persistent response caching (1 week), and transparent User-Agent identification in accordance with community usage policies—no API keys required.

> Data © OpenStreetMap contributors (ODbL). Descriptions from Wikidata (CC0); pictures from Wikimedia Commons under their respective licenses.

## Places in Shoji, with an interactive map

Shoji displays Places and Locations with an integrated, high-resolution **map view** alongside standard attributes, showing the location's bounding area centered on a pin:

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/place-karlskirche-shoji.png" alt="A place in Shoji: the form, the picture and the map" width="560">

* **Multiple map styles**: Right-click the map to choose between diverse cartographic styles, served from keyless public tile providers and cached locally for two weeks:
  * **OpenTopoMap**: Detailed topographic relief contours.
  * **OpenStreetMap**: Classic standard cartography.
  * **Humanitarian**: High-contrast, warm presentation.
  * **CyclOSM**: Cycling infrastructure and path layouts.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/shoji-place-westbahnhof.png" alt="The menu of map styles in Shoji" width="560">

* **Configurable defaults**: Set your preferred default style in *Settings › Plugins › Navigate › SEN Spatial Navigator › Map style*. Context menu selections apply to the current view with an option to revert to default.

## Dynamic spatial relations

The **Spatial** group in Akita's *Open related…* menu introduces dynamic, compute-on-demand relationships:

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/open-related-spatial.png" alt="Open related › Spatial: Covered By, Equal, Nearby, Overlap, Touch, Within" width="460">

* **Topological relationships (RCC-8)**: Supports *Within / Contains*, *Covered by / Covers*, *Touch*, *Overlap*, and *Equal* based on Region Connection Calculus. These relations are **computed dynamically rather than stored statically**: the spatial navigator evaluates them on demand using the GEOS geometry engine against local outlines, combined with Wikidata administrative containment hierarchies (e.g. *St. Stephen's Cathedral* is within *Innere Stadt*, *Vienna*, and *Austria*). They are always fresh and never become obsolete when boundaries are edited.
* **Nearby points of interest**: Lists points of interest surrounding a location—cafés, sights, transit stops, parks, and shops—queried dynamically via the Overpass API. Results include the place category and precise distance in meters, prioritizing prominent cultural landmarks over retail shops and filtering out closed or disused venues. Nearby places are automatically created in your Library (`~/Library/Places`), with the top results enriched with photography in the background.
* **Persistent proximity links**: A persistent **Located near** relation is created when SUESS links a Location or geotagged photo to its recognized Place.
* **Configurable navigation settings**: Customize Overpass server mirrors, search radii, maximum results, response cache duration, and default map styles in the settings window.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/nearby-places.png" alt="The places nearby, in the menu of the relation" width="640">

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/geo-settings.png" alt="The settings of the spatial navigator: radius, most things, how long answers are kept, map style, Overpass server" width="560">

## Akita: relation groups as folders

Selecting an entire relation group—such as *Open related › Spatial*—opens a Tracker window containing a dedicated folder for each relation in the group (*Nearby*, *Within*, *Covered By*…), marked with an identifying globe icon:

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/spatial-relations-folder.png" alt="The folders of the spatial relations of a place" width="520">

* **On-demand fetching**: Computed and remote relations load asynchronously only when their folder is opened, displaying a non-blocking *Fetching relations…* indicator until populated. Nothing is queried over the network unless you explicitly inspect it.
* **Persistent kept relation folders**: Drag any relation folder (such as *Nearby*) to your Desktop or Home directory. The copied folder retains its query parameters and automatically refreshes its contents whenever opened.

<img src="https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/kept-nearby-folder.png" alt="A kept folder of what is nearby, with the distance and what each thing is" width="560">

* **Responsive menus**: Relation target menus open instantly with an inline spinner while background lookups proceed.
* **Clean name disambiguation**: Items in relation folders sharing the same display name are distinguished by their filename rather than arbitrary numeric suffixes.
* **Themed relation folder icons**: Relation folders now display the distinctive folder icon defined by their ontology (`SEN:ICON:FOLDER`).

## Desktop integration & system polish

* **Server restart shortcuts**: The Deskbar menu now includes direct commands to *Restart SEN Server* and *Restart Akita* alongside *Settings…*.
* **Dedicated context library icons**: Library folders for individual contexts now feature dedicated styling (*Library Folder* by Matthew McClintock).
* **Automated type retirement**: The ontology installer now cleanly deregisters obsolete MIME types and templates when an updated ontology removes them, without affecting your existing user files.
* **Shared network stack**: Akari, BERT, and Paperboy adopt the shared, resilient HTTP socket client for improved reliability with external APIs.

## Under the hood

* **Asynchronous plugin channels**: The server architecture now provides dedicated worker threads, loopers, and queues per plugin, preventing slow or remote lookups from blocking other operations.
* **Dynamic relation protocol**: Implemented the `kResolve` and `kLibraries` protocol enabling navigator plugins to compute relations on the fly with timeout protection (up to 90 seconds).
* **C++ developer additions (sento)**:
  * `SenSocketHttp.h`: Lightweight socket-based HTTP client featuring strict redirect authorization isolation and configurable timeouts.
  * `SenMapStyles.h`: Standard definitions for map tile providers and attribution.
  * `SenSettingsForm.h`: Added conditional form dependencies (`enabled_when` supporting logical AND/OR expressions) and visual separator items.
  * Dynamic relation flags: `SEN:remote` and RCC-8 classification property `SEN:REL:Rcc8`.
* **Ontology updates (`0.1.0-39`)**: Introduced the Spatial ontology, updated Core and People ontologies to revision 33, added provenance attribution IRIs (such as Wikidata P131), and standardized title attributes.

## Packages

34 packages in the repository **Kiri**, among them:

* `sen_core`: `1.0.0~beta6-3`, `sen_oni`: `1.0.0~beta6-2`, `akita`: `1.0.0~beta6-13`, `shoji`: `1.0.0~beta6-7` (`sen_filetypes` remains at `1.0.0~beta4-1`).
* **New packages**: `sen_ontology_spatial` (`0.1.0-39`), `sen_plugin_place_enricher` (SUESS, `0.1.0~beta6-5`), and `sen_plugin_spatial_navigator` (`0.1.0~beta6-13`, linking against `geos`).
* **Updated plugins**: Akari, BERT, and Paperboy (`0.1.0~beta6-1`), bundled under `sen_plugins` (`1.0.0~beta6-1`).

## Upgrading from Beta 5

Stop SEN while packages are updated, then restart the services:

```sh
sen stop
pkgman update
sen start       # and, to reload Akita:  akita restart
pkgman install sen_ontology_spatial sen_plugin_place_enricher sen_plugin_spatial_navigator
```
*(Alternatively, installing `sen_plugins` pulls in all available plugins.)*

First-time installation? See the *Quick Install* instructions in the beta 3 notes:
`pkgman add-repo https://kiri.sen-labs.org/x86_64`, then run `pkgman install sen_core sen_plugins akita sen_filetypes shoji` along with your preferred ontologies.

## Known limitations

* **Unconstrained searches are worldwide**: Searching for a location by name alone without specifying a city or country queries worldwide, returning the most prominent global match first. An integrated local positioning service is planned for the next release.
* **Re-enrichment behavior**: *Enrich (Overwrite)* refreshes data for the existing OpenStreetMap reference rather than starting a fresh search; to query a different location, enrich a new Place or Location file.
* **Imagery availability**: Photography depends on Wikidata entries having linked Wikimedia Commons images; places mapped solely in OpenStreetMap do not include pictures.
* **Overpass API response times**: Public Overpass servers can take up to 30 seconds in dense urban areas. You can configure alternative public mirrors or a private instance in settings.
* **Filesystem requirements**: BFS volumes only. Requires Haiku nightlies `hrev60299_1` or newer.

## What comes next

Beta 6 represents the final major feature beta before the SEN 1.0 release candidates. Upcoming focus includes:
* Local location awareness to automatically prioritize nearby results during unconstrained name searches.
* GPX and GeoJSON import extractors, along with automated geotag extraction for photos.
* Ongoing bug fixes and performance tuning based on beta 6 feedback.

In parallel, architectural work continues on SEN 2.0 "Gnosis".

## Credits

Map data © [OpenStreetMap contributors](https://www.openstreetmap.org/copyright) (ODbL). Map styles: [OpenTopoMap](https://opentopomap.org) (CC-BY-SA), the Humanitarian OpenStreetMap Team style hosted by OpenStreetMap France, and [CyclOSM](https://www.cyclosm.org). Descriptions from [Wikidata](https://www.wikidata.org) (CC0); imagery from [Wikimedia Commons](https://commons.wikimedia.org) under respective licenses. Icons: *Maps* by zuMi (Place), *Haikunaut* by Arti (SUESS), *GCompris* by 3dEyes (MIT, via [HVIF Store](https://hvif-store.art/)), and *Library Folder* by Matthew McClintock (Design Science License). Computational geometry powered by [GEOS](https://libgeos.org).

## Your support is essential

Please consider clicking the "Support" button in the About dialog to contribute to ongoing development, or donate directly through [SEN Labs Liberapay](https://liberapay.com/sen-labs/).

## Feedback & Community

* **Issue Tracker & Source:** [https://github.com/sen-laboratories](https://github.com/sen-laboratories)
* **Package Repository:** `https://kiri.sen-labs.org/x86_64`
* **Community Discussion:** Haiku Community Forums
