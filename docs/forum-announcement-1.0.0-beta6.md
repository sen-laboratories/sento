<!--
Draft of the post for the Haiku Community forum (discuss.haiku-os.org). Not published yet. Suggested place: a reply in the topic of the Beta 1/2/3/4/5 announcements
(same topic, "Third-Party"). The images are the ones of the release notes (tag v1.0.0-beta.6). Drafts are not committed.
-->

# SEN 1.0 Beta 6: Places and Spatial Relations

Beta 5 was about making SEN yours. Beta 6 is about **where things are**. Still for **Haiku R1/beta6 nightlies (x86_64)**, and still storing everything natively in file attributes.

**Places and locations.** A new ontology, **SEN Spatial**, adds **Place** (*Karlskirche*, *Vienna*) and **Location** (just where something is). Choose *Enrich* and **SUESS**, the new place enricher, looks the place up at OpenStreetMap and Wikidata: other names, kind, position, bounding box, outline, address, links, and a **picture** with its license and credit. A file that names a city or country is searched in that country only (*Westbahnhof* in *Wien* no longer finds Budapest), and a metro station is told from the train station of the same name. No API key needed; your photos' GPS positions are only used if you switch that on.

![Which place is yours?](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/enrich-location.png)

**A map in Shoji.** Places open in Shoji as a form with a **map** and a pin. Pick the style with the secondary mouse button: OpenTopoMap (relief), OpenStreetMap, Humanitarian or CyclOSM, all free public servers without a key; the default is a setting.

![A place in Shoji](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/place-karlskirche-shoji.png)

**Spatial relations.** *Open related › Spatial* offers **within / contains, covered by / covers, touch, overlap, equal** (the RCC-8 relations, worked out when you open them from the outlines of your places with GEOS and from Wikidata's "located in" chain, never stored, never stale) and **nearby**: the shops, cafés, sights and stops that OpenStreetMap knows around a place, with what each is and its distance in metres, each one a Place in your Library (the Overpass server, radius and how long answers are kept are settings).

![Nearby places](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/nearby-places.png)

**Relation groups as folders in Akita.** Choose the group itself (*Open related › Spatial*) and a window opens with a **folder for each relation**, empty until you open it, so nothing is asked that you do not look at. Drag a folder to your Home folder or the Desktop to **keep it**: it fetches again when you open it. The menus open at once with a spinner (*Fetching relations…*) while a plugin works.

![The folders of the spatial relations of a place](https://raw.githubusercontent.com/sen-laboratories/sento/v1.0.0-beta.6/docs/images/beta6/spatial-relations-folder.png)

**Under the hood:** a channel for each plugin in the server and dynamic relations resolved by their navigator plugin, relation folder icons from the ontology, the installer removes types that an ontology no longer ships, a shared HTTP client, and new API headers in sento (`SenSocketHttp.h`, `SenMapStyles.h`, settings forms with `enabled_when`).

**Upgrade** (stop SEN while packages are updated):

```sh
sen stop
pkgman update
sen start       # and, to reload Akita:  akita restart
pkgman install sen_ontology_spatial sen_plugin_place_enricher sen_plugin_spatial_navigator   # the new parts (or sen_plugins)
```

New here? `pkgman add-repo https://kiri.sen-labs.org/x86_64`, then `pkgman install sen_core sen_plugins akita sen_filetypes shoji` and the ontologies you like. Requires a Haiku **nightly from hrev60299_1 on**.

**Good to know:** a Location with only a name and no city or country is searched worldwide (a location service is planned for the next beta); Overpass can take half a minute for a busy area; places that Wikidata does not know have no picture. BFS volumes only; still a beta, probably the last before the release candidates.

*Map data © OpenStreetMap contributors (ODbL); descriptions from Wikidata (CC0); pictures from Wikimedia Commons under their own licenses.*

**Links:** [Release notes](https://github.com/sen-laboratories/sento/releases/tag/v1.0.0-beta.6) · [Source code](https://github.com/sen-laboratories) · Package repository `https://kiri.sen-labs.org/x86_64` · Support SEN Labs on [Liberapay](https://liberapay.com/sen-labs/).

Reports and feedback are always welcome. What would you like to see on the map next?
