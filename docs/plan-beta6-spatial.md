<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->
# Plan for beta 6: places, locations and spatial relations (SEN Spatial)

Beta 6 is probably the last beta before the release candidates. It adds one last ontology, **SEN Spatial** (`sen_spatial`), for geographic entities and the relations
between them, and **SUESS**, the plugin that enriches places (*Semantic Urban & Environmental Spaces for SEN*, after Eduard Suess, who described the biosphere in
*The Face of the Earth*).

## What it is

**Place** (`entity/x-vnd.sen-labs.entity.place`, `schema:Place`) is a human, social entity: a named thing with an identity, a meaning or a function ("TU Wien",
"Café Central", "Vienna"). It stays the same place when its boundary shifts or when nobody knows its coordinates.

**Location** (`entity/x-vnd.sen-labs.entity.location`, `dcterms:Location`) is the spatial reference: where something is on Earth (coordinates, bounding box, WKT
polygon, postal address), without more identity than that: the position of a photo, a waypoint, a meeting point.

The attributes of the footprint are the same in both (a Place has them when they are known), so that a column or a query works for both:

| What | Attribute |
|---|---|
| Name, other names, description | `dc:title`, `schema:alternateName`, `dc:description` |
| Kind of place | `schema:additionalType` (the tag of OpenStreetMap as IRI, for an administrative boundary the kind: city, state, country) |
| Links | `schema:sameAs` (Wikidata, OpenStreetMap, Wikipedia, GeoNames), `schema:url` |
| Picture | the thumbnail; `schema:image`, `schema:license`, `schema:creditText` |
| Position | `schema:latitude`, `schema:longitude`, `schema:elevation` (doubles) |
| Bounding box | `SEN:boxSouth`, `SEN:boxNorth`, `SEN:boxWest`, `SEN:boxEast` (four doubles so that a query finds the places in a region) |
| Boundary | `geo:asWKT` (simplified, to tell where places are in relation to each other, not to draw a map) |
| Address | `schema:address`, `schema:postalCode`, `schema:addressLocality`, `schema:addressRegion`, `schema:addressCountry` |

The schema is `sen-oni/schema/sen-spatial.yaml` (done, see *Status*).

## Relations

A file or entity is **located near** a place or location (first called *located at*; renamed because the lookup of a position finds the place that is there and the places around it, and there can be several) (`dcterms:spatial`; stored; a photo, a paper, an event). The relations *between* places and locations are RCC-8
(Region Connection Calculus) in the names of everyday use proposed in the workshop paper on LLMs and spatial reasoning (`viterbi-web.usc.edu/~sabek/pdf/25_workshop_llm_spatial_benchmark.pdf`; not read in full, the names are those of the research notes), with the OGC GeoSPARQL vocabulary as `class_uri`:

| Relation | Inverse | RCC-8 | GeoSPARQL |
|---|---|---|---|
| within | contains | NTPP / NTPPi | `rcc8ntpp` |
| covered by | covers | TPP / TPPi | `rcc8tpp` |
| touches | touches | EC | `rcc8ec` |
| overlaps | overlaps | PO | `rcc8po` |
| equal to | equal to | EQ | `rcc8eq` |
| *disjoint* | | DC | not a relation: nearly every two places are disjoint; computed when asked |

Each inverse pair is one relation type with `label` and `inverse_label`, as the *Parent* relation of people. `within` is the strict NTPP and not `geo:sfWithin`
(which includes the border-touching case, that is `covered by`). They are **not** the *Contains* relation of the core ontology (parts of a document); they are in the
group *Spatial* and have MIME types of their own (`relation/x-vnd.sen-labs.relation.spatial.*`).

A further dynamic relation, **point of interest** (inverse *in the area of*, `schema:containsPlace`, `remote`), lists the named things that OpenStreetMap knows in the area of a place (Overpass API, `nwr[name](south,west,north,east); out center;`, at least 500 m and
at most 3 km across, roads and borders left out, the 100 nearest); a place file is made in the Library for each one that is not there yet (not enriched).

RCC-8 is defined for regions: a point (a café) can be within, equal to or touching a region, not overlap it. The navigator checks the type of the geometry.

### The topological relations are dynamic

They are **not stored**. When the user chooses *within*, *contains*, *touches*... at a place, the **spatial navigator** (a plugin like the personal relation navigator,
`application/x-vnd.sen-plugin.sen-labs.spatial-navigator`) works the relation out and shows the result. That is why

* enriching a place needs the lookup of the place itself and nothing else (no parents, no neighbours, no depth setting),
* the relations never go stale when a boundary is corrected,
* nothing is created that the user did not ask for.

How the navigator finds the targets:

* **within** (a place is selected, the parents are asked for): the chain "located in" (`P131`) of the Wikidata item of the place, which comes as a flat list of ids, up to the
  country. The parents that exist as files in the Library are the targets; the others are listed as places that can be made (a click makes them with the enricher). The
  chain is cached so that the menu opens at once the second time (where: see the open questions).
* **contains** (the inverse): the places that exist locally and lie inside this one: the candidates are found with a query on the bounding box (`SEN:boxSouth` is indexed),
  and the footprints decide (see below).
* **touches, overlaps, equal to, covered by, covers**: the same candidates, decided by the footprints.

The decision from the footprints is done with `libgeos_c` (GEOS, the C API only): `GEOSRelate()` returns the DE-9IM matrix of two geometries, and eight patterns map it to the
RCC-8 relation (the mapping was tried on Haiku with synthetic squares and with the real polygons of Stephansdom and Wien, all correct). A place that has only a point uses the
point; a place without any footprint has no spatial relations.

**Dependency:** `lib:libgeos_c` is the only new one. HaikuPorts has `geos` / `geos_devel` 3.14.0 (the package `geos` provides `lib:libgeos_c`; its own `lib:libgeos` comes
with it, 1.6 MB for both). The programs link only the C API (`geos_c.h`).

## SUESS, the enricher

A plugin of the kind *Enrich*, folder `sensei/plugins/place-enricher`, built like Akari on `BaseEnricher`; places are made in the Library of the context
(`~/Library/Places`).

* **Lookup by name** at OpenStreetMap **Nominatim** (`search`, `format=jsonv2`, `extratags`, `namedetails`, `addressdetails`, `polygon_text` with `polygon_threshold`), with the
  candidate chooser when more than one is found (the best first). The title can come from the attribute `dc:title` or the name of the file.
* **Lookup by position** (reverse geocoding of a file that has coordinates, e.g. from a photo): **opt-in**, a setting of the plugin (default off), because the position of the
  user's file goes to a third party.
* **What the result gives:** name and other names, position, bounding box, simplified boundary, the type (`category`/`type`, or `addresstype` for administrative boundaries) as
  `schema:additionalType`, the address, the web site, and the links (OpenStreetMap, Wikidata, Wikipedia from `extratags`) in `schema:sameAs`.
* **The thumbnail** (a small picture that stands for the place): from **Wikidata**, if Nominatim gave the Wikidata id. For administrative places the coat of arms (`P94`), else
  the flag (`P41`), else a locator map (`P242`), else the photo (`P18`); for buildings and other points the photo, else the logo (`P154`). The size comes from Wikimedia Commons
  (`iiurlwidth`); its license, author and the need to name the author (`extmetadata`) go into `schema:license` and `schema:creditText` (a CC BY-SA photo must be credited).
* **Calls:** one to Nominatim, and if there is a Wikidata id, one to Wikidata, one to Commons and one for the picture. No other calls at enrichment.
* **Rules of the services:** the public Nominatim allows at most one request per second, wants an identifying `User-Agent`, results to be cached, no autocomplete and no personal
  data; the data are ODbL (OpenStreetMap contributors must be named: in the About text, the README and the credit of the plugin, like the notice of Akari for TMDB). The
  same-session cache and a pause between requests are part of the plugin. Wikimedia wants an identifying `User-Agent` too.
* **Settings** (Settings window, *Plugins › Enrich › SEN SUESS*): language for names (default: the system language), lookup by position (off), the chooser as in the other plugins
  (shared setting).

## Not in beta 6

GPX / GeoJSON / KML extractors (and with them the MIME types `application/gpx+xml`, `application/geo+json`), EXIF positions of photos, Photon or GeoNames as other services,
storing the parents of a place, a depth for the parents, a map view.

## Work packages

| # | What | Status |
|---|---|---|
| 1 | `sen-oni`: `schema/sen-spatial.yaml`, `generate.sh`, README, generated rdefs and `SenOntoSpatial.h`, package `sen_ontology_spatial` | schema, generator and rdefs done (checked, 28 tests pass, all rdefs compile with `rc`); the package is built by `package.sh` (all folders of `ontologies/`) |
| 2 | Icons: Place has the astronaut with the flag (`schema/icons/x-vnd.sen-labs.entity.place.hex`), Location and the spatial relations the globe (`x-vnd.sen-labs.entity.location.hex`); both rendered with `BIconUtils` in the VM; MIT, by Arti (Haikunaut) and 3dEyes (GCompris), credited in `sen-oni/ontologies/spatial/README.md`; the Library folder icon is DSL, see `sen-core/data/icons/library-folder.COPYING` | done |
| 3 | `sensei/plugins/place-enricher` (SUESS): Nominatim, Wikidata/Commons thumbnail, settings, README, tests with recorded responses | open |
| 4 | `sensei/plugins/spatial-navigator`: the dynamic relations, `libgeos_c`, the DE-9IM mapping (a unit test on the Mac with fixtures), the cache of the chain | open |
| 5 | Check how the server handles a dynamic relation that is offered at a type (`RelationHandler.cpp`: dynamic relations are never created at a file) and how a navigator returns targets that do not exist yet | open |
| 6 | Akita: the group *Spatial* in the menus, the thumbnail of a place; Shoji form of Place and Location (derived from the ontology?) | open |
| 7 | Docs: user guide chapter, plugin README, notices (OpenStreetMap/ODbL, Wikimedia licenses), release notes, announcement | open |
| 8 | Packages for Kiri: `sen_ontology_spatial`, `sen_plugin_place_enricher`, `sen_plugin_spatial_navigator` (+ in `sen_plugins`), `requires lib:libgeos_c`; version beta 6 in the rdefs; tags; merge into `main` as before | open |

## Open questions

* **Dynamic only:** a user cannot state "my home is within Vienna" by hand if the relation is never stored. If that is wanted, a stored variant (or storing an answer that the
  user confirmed) would be needed. The relation *Located At* is stored.
* **Footprint on the Place:** the plan puts the footprint attributes on both entities. The alternative is a Place that has only a relation to a separate Location file; it
  needs two files per enriched place.
* **Where the cache of the "located in" chain lives** (a file in `~/config/cache/sen`, or attributes of the place).
* **Names for the labels** (`touches`, `overlaps`, `equal to`) as the menus show them.
