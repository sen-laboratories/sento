<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->
# Semantic types: one scheme for Haiku and macOS (draft spec)

Status: **draft for review**, nothing of it is implemented. It replaces the use of the MIME database for what SEN means by a type (entities, relations, classifications,
the scientific paper...). Decisions so far (2026-10-16): identifiers are UTI-style, the types live in a SEN registry of their own, and this document comes first.

## 1. Why

* The MIME database has two levels (`supertype/subtype`) and one entry per type. SEN needs a **tree** (a friend is a personal relationship, which is a relationship)
  and **many types for one file format** (a PDF is a PDF, and a scientific paper, and maybe a book). Every ontology that adds types adds entries to the MIME database
  of the user, which the system, the Tracker and FileTypes then show to people who did not ask for them (`document/scientific-paper` next to `application/pdf`).
* SEN for macOS has Uniform Type Identifiers (UTIs): reverse-DNS names with **conformance** (a type conforms to one or more parents). If both systems use the same
  identifiers and the same conformance, an ontology is written once and files, queries and relations mean the same on both.

## 2. Two layers

| Layer | Haiku | macOS | Says | Who owns it |
|-------|-------|-------|------|-------------|
| **Format** | `BEOS:TYPE` (MIME) | the UTI of the file (`public.pdf`) | how to open it, what its bytes are | the system, the file format |
| **Meaning** | `META:TYPE` (attribute, already there, indexed) | an extended attribute with the same name | what it is *for* SEN: a scientific paper, a friend relationship, a person | the ontologies of SEN |

A file has both. `BEOS:TYPE` stays coarse and is not changed to say what a file means: a scientific paper is `application/pdf` (or `application/epub+zip`) with
`META:TYPE = org.sen-labs.paper`. Files that exist only for SEN (a person, a label, the file of a relation in a relation folder) have a coarse `BEOS:TYPE` of the
supertype of their kind: `entity/...`, `classification/...`, `relation/personal`, `relation/organisational` (the few that the file manager needs for icons and
"open with"), and the real type in `META:TYPE`.

`META:TYPE` holds one identifier, the most specific one that is known. Everything it conforms to follows from the registry (section 4), it is not written to the file.

## 3. Identifiers

* `org.sen-labs.<name>` for what SEN defines; dots separate the parts of a name only, **the tree is not in the name** (conformance says it): `org.sen-labs.relation.friend`
  conforms to `org.sen-labs.relation.personal`, but nothing may be derived from the spelling. Names are lower case, `[a-z0-9-]`, parts separated by dots.
* What exists elsewhere is used as it is: `public.pdf`, `public.document`, `public.plain-text`, `public.source-code` (macOS names; the registry maps them to the
  MIME types on Haiku, section 6). An ontology of another author uses its own domain (`com.example.invoice`).
* Roots (examples): `org.sen-labs.entity`, `org.sen-labs.relation`, `org.sen-labs.classification`, `org.sen-labs.document`.

Examples (draft; the trees are for review):

```
org.sen-labs.relation                         (BEOS:TYPE of the file of a relation: relation)
  org.sen-labs.relation.personal              relation/personal
    org.sen-labs.relation.knows
      org.sen-labs.relation.acquaintance
      org.sen-labs.relation.friend
        org.sen-labs.relation.close-friend
      org.sen-labs.relation.partner
    org.sen-labs.relation.family
      org.sen-labs.relation.parent            (parent of / child of)
      org.sen-labs.relation.sibling
      org.sen-labs.relation.relative
  org.sen-labs.relation.work                  relation/personal  (between persons, at work)
      org.sen-labs.relation.colleague
      org.sen-labs.relation.superior          (superior of / subordinate of)
      org.sen-labs.relation.mentor
      org.sen-labs.relation.collaborator
      org.sen-labs.relation.client
  org.sen-labs.relation.organisational        relation/organisational (between a person and an organization)
      org.sen-labs.relation.employment
      org.sen-labs.relation.membership

org.sen-labs.entity
  org.sen-labs.person        (BEOS:TYPE entity/person; the files of the People application of Haiku are application/x-person until they are migrated)
  org.sen-labs.organization
  org.sen-labs.book          (conforms to public.document; BEOS:TYPE of the file: application/epub+zip, application/pdf...)
public.document
  org.sen-labs.paper         (a scientific paper: BEOS:TYPE application/pdf, application/epub+zip; META:TYPE org.sen-labs.paper)
```

## 4. The registry

One place that says what each identifier is, written once in the LinkML schema of the ontology (the schema is the single source, as it is for the MIME types now) and
generated for each system:

* **Haiku:** a file per ontology in `data/sen/types/<ontology>` of the package (system or user, also non-packaged), read by the SEN server and by the programs that
  need it through a header-only library in sento (`SenTypes.h`, like `SenLookup.h`): no installation step into a database, a package that comes or goes changes
  the registry (the server notices it like the ontologies today). The format is one flattened `BMessage` per ontology (the types as nested messages), compiled by the
  generator; the schema stays the readable form.
* **macOS:** `UTExportedTypeDeclarations` of the app (Info.plist), generated from the same schema.

A type has: `id`, `conformsTo` (identifiers, at least one except for roots), `title` (the short name that menus show), `description`, `icon`, `attributes` (what the
Attributes menu and the columns offer: name, public name, type, flags: as the attribute info of a MIME type now), the BEOS:TYPE that its files get (`format`, optional:
for files that exist for SEN only), the MIME types that it can be found on (`formats`, optional: `application/pdf` for the paper) and, for relations, the config that
the `SEN:REL:CONFIG` resource has now (labels in both directions, `bidir`, `dynamic`, `contained`, source and target types, group).

Lookups the library offers: `Find(id)`, `ConformsTo(id, ancestor)`, `Ancestors(id)`, `Descendants(id)`, `Attributes(id)` (own and inherited along the
conformance, nearest first; a name that is defined twice is defined once), `Icon(id)` (the nearest in the chain), `RelationTypes()` (the descendants of
`org.sen-labs.relation`), `ForFormat(mimeType)` (the types that can be found on files of that format).

The MIME database keeps only what the system needs: the coarse supertypes and what files of SEN get as `BEOS:TYPE` (`entity/person`, `relation/personal`...), with
a short description and an icon; **no entry for a meaning**.

## 5. What it changes

* **Relations.** A relation type is an identifier below `org.sen-labs.relation`; what is now `relation/x-vnd.sen-labs.relation.person.friend` becomes
  `org.sen-labs.relation.friend`. The attribute that stores a relation is `SEN:REL:<identifier>`; the menus (New related, Open related, drop) list the
  `RelationTypes()` of the registry, grouped by the **conformance tree** instead of the `group` annotation (the group "Personal" is the parent
  `org.sen-labs.relation.personal`; its title and icon are those of that type; a deeper tree gives nested submenus, with a limit of depth that the menu shows). The
  config of a relation (labels...) comes from the registry, not from the MIME type.
* **Queries by meaning.** `META:TYPE` is indexed. "All relations of the kind family" is a query for the descendants of `org.sen-labs.relation.family` that the server
  expands from the registry (`META:TYPE == a || META:TYPE == b ...`, as the ids of `SEN:TO` are queried now); a descendants list longer than the limit of a query is
  chunked like `SEN:TO`.
* **The Attributes menu of Akita** (already shows the attributes of the `META:TYPE` of a file) reads the registry instead of the MIME database, along the whole
  conformance chain; FileTypes of SEN gets a view of the registry (the tree, the attributes) since FileTypes of Haiku only knows the MIME database.
* **Plugins.** A plugin declares the types that it handles by identifier and/or format (as it declares MIME types now). Which plugin is asked first when several handle a
  file is decided by the **most specific type** (a plugin for `org.sen-labs.paper` before one for `public.pdf`) and then by an explicit order; this is the rule that is
  still to be designed (open question 3).
* **Templates and "New".** The entities that an ontology brings are templates of the types with a `format` (files that exist for SEN): the template carries `BEOS:TYPE`
  and `META:TYPE`.
* **Classification.** Labels, topics... are types below `org.sen-labs.classification` (BEOS:TYPE `classification/...` as now for the file).

## 6. Mapping of other systems

A table in the registry maps the identifiers of other systems to the format layer: `public.pdf` = `application/pdf`, `public.plain-text` = `text/plain`,
`public.source-code` = `text/x-source-code` and so on, so that an ontology written with UTIs (for macOS) finds its files on Haiku and the other way round. The
file extensions of a type stay with the format (MIME database, `UTTypeTagSpecification` on macOS).

## 7. Migration

A tool (`sen_oni --migrate`, later part of `sen_oni --update`) with a table old type -> new:

1. a file with `BEOS:TYPE = entity/x-vnd.sen-labs.entity.person` gets `META:TYPE = org.sen-labs.person` and `BEOS:TYPE = entity/person`; a file with
   `document/scientific-paper` as `BEOS:TYPE`/`META:TYPE` gets `META:TYPE = org.sen-labs.paper` and the BEOS:TYPE of its format (guessed from the content if it is not
   a format); the Haiku People files (`application/x-person`) become persons (their attributes are kept; the old type is deprecated: the new type conforms to a
   registry entry that names it, so both are found until the files are migrated);
2. the attributes `SEN:REL:relation/...` and the relation entries in `SEN:TO`/`SEN:META` keep their ids; the relation **type** inside a stored relation is the
   key of the attribute, which is renamed (`SEN:REL:x-vnd.sen-labs.relation.person.friend` -> `SEN:REL:org.sen-labs.relation.friend`) on the file and its opposite
   side; the transaction logic of the server (AttrSnapshot) is reused;
3. the MIME types of the ontologies that are replaced are uninstalled, those of the format layer stay.

Until migrated, the registry lists the old MIME types as aliases of the new identifiers (`aliases`), so that a file that is not migrated yet is still understood.

## 8. Open questions

1. **Trees.** The trees above are a first proposal (work relations a child of personal or of their own root? Where do `knows`, `acquaintance` belong?). To be decided
   by reviewing the people ontology as the first one.
2. **BEOS:TYPE of files of SEN.** `relation/personal` and `relation/organisational` as coarse types for the file of a relation in a relation folder (icons, columns), `entity/person`...
   or a single `application/x-vnd.sen-labs.entity` for everything that exists for SEN only? The first gives icons and default columns for free.
3. **Order of plugins** for one file (the rule above), including `extract` and `identify`: which one writes the `META:TYPE` when several can.
4. **Depth** of the submenus of relations (3 levels?) and what a type with a long list of children does in a menu (the idea of the user for the overflow).
5. **Aliases of other vocabularies** on the types (`schema:ScholarlyArticle`, `foaf:Person`) for export and for the web: in the registry as `exact_mappings` of the schema.
6. The **version** of the registry format and of the types (a type of the user that conforms to a type that changed).

## 9. Steps

1. Review of this spec, trees of the people ontology and the paper (this document).
2. `SenTypes.h` (reader of the registry, tests with the registry of the people and papers ontologies), generator output (`data/sen/types/<ontology>`), `Find`, `ConformsTo`,
   `Attributes`, `RelationTypes` first.
3. Server: relation types and configs from the registry (the MIME database as a fallback for old ontologies); queries by descendants; the migration table.
4. Akita: menus (tree of relation types), the Attributes menu, icons; FileTypes of SEN: view of the registry.
5. Ontologies: people and papers first, then the others; `sen_oni --migrate`.
6. macOS: the same schema generates the UTI declarations.
