<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2026 SEN Labs e.U.
-->
# Semantic types: one scheme for Haiku and macOS (draft spec)

Status: **agreed in principle, nothing of it is implemented**. It replaces the use of the MIME database for what SEN means by a type (entities, relations,
classifications, the scientific paper...). Decisions (2026-10-16): identifiers are UTI-style, the types live in a SEN registry of their own, the LinkML schema is the single
source of truth, mappings to other vocabularies are SSSOM, the first type to be done is the scientific paper. See section 8 for what is decided and what is open.

## 1. Why

* The MIME database has two levels (`supertype/subtype`) and one entry per type. SEN needs a **tree** (a friend is a personal relationship, which is a relationship)
  and **many types for one file format** (a PDF is a PDF, and a scientific paper, and maybe a book). Every ontology that adds types adds entries to the MIME database
  of the user, which the system, the Tracker and FileTypes then show to people who did not ask for them (`document/scientific-paper` next to `application/pdf`).
* SEN for macOS has Uniform Type Identifiers (UTIs): reverse-DNS names with **conformance** (a type conforms to one or more parents). If both systems use the same
  identifiers and the same conformance, an ontology is written once and files, queries and relations mean the same on both.

## 2. Two layers

* **Layer 1: Syntax (format)**: the technical nature of a file, what its bytes are and how a program opens it.
* **Layer 2: Semantics (meaning)**: what the file embodies, what it is *for* in the life of the person: a scientific paper, a person, a friendship.

| Layer | Haiku | macOS | Owned by |
|-------|-------|-------|----------|
| **1 Syntax** | `BEOS:TYPE` (MIME type): `application/pdf` | the UTI of the file: `public.pdf` | the system, the file formats |
| **2 Semantics** | `META:TYPE` (attribute, indexed): `org.sen-labs.paper` | an extended attribute of the same name | the ontologies of SEN |

A file has both. A scientific paper is `application/pdf` (or `application/epub+zip`) in layer 1 and `org.sen-labs.paper` in layer 2; the PDF viewer works with the first, SEN
with the second. `META:TYPE` holds one identifier, the most specific one that is known; what it conforms to follows from the registry (section 4) and is not written to the file.

**MIME types stay where they are enough.** Layer 1 is MIME, and so is what the system and the file manager need of files that exist only for SEN: the supertypes
`entity/`, `classification/`, `relation/` and `meta/` and the few coarse types below them that give such a file an icon, columns and an application (`relation/personal`,
`relation/organisational`, `entity/person` while it is a file of its own). Code that only needs to know "is this a relation / a classification / an entity?" keeps checking the
supertype of `BEOS:TYPE` as it does now, with no registry. What is **not** put in the MIME database any more is a *meaning*: there is no `document/scientific-paper` entry, and
no entry for each kind of friendship.

**What stays native on Haiku, and what is new.** SEN is a *Semantic Extension* of the system, so the system keeps its own parts: files, BFS attributes and indexes (also for
`META:TYPE`), MIME types for formats and file types, queries, Tracker and the Deskbar, packages, and the preferred application of a format. What SEN adds is the layer of
meaning and a registry for it. The registry is a read-only file that a package brings, not a second database that has to be kept in step with the MIME database; and nothing
in the system depends on it: a Haiku without SEN sees ordinary files, with an attribute `META:TYPE` that it does not know.

## 3. Identifiers

* `org.sen-labs.<name>` for what SEN defines (`org.sen-labs.paper`, `org.sen-labs.person`, `org.sen-labs.relation.friend`); parts are separated by dots, the names are lower case
  `[a-z0-9-]`. **The tree is not in the name**: conformance says it, nothing may be derived from the spelling.
* **Terms of an established vocabulary** (schema.org, FOAF, REL, SKOS, Dublin Core) are not given an identifier in somebody else's namespace: a name such as `org.schema.ScholarlyArticle`
  would be read as the official UTI of schema.org, which it is not. They are SEN's own identifiers that carry the established prefix, `org.sen-labs.<prefix>.<Term>`
  (`org.sen-labs.schema.ScholarlyArticle`, `org.sen-labs.foaf.Person`, `org.sen-labs.rel.friendOf`), and **say what they mean by a mapping** to the term itself (section 6): the
  prefix is the one that the attribute names use already (`schema:`, `foaf:`, `dc:`). SEN's own ontologies use their name instead of a prefix (`org.sen-labs.people.close-friend`).
  A SEN term is used where the established one is not fine enough; where one is, the established term is the type and no own one is made.
* What exists elsewhere in the system is used as it is: `public.pdf`, `public.document`... (macOS names; the registry maps them to the MIME types on Haiku, section 6).
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

## 6. Mappings

How a SEN type and a term of another vocabulary or system are related is data, not code, and is written as **SSSOM** (Simple Standard for Sharing Ontological Mappings): a table of
`subject_id`, `predicate_id`, `object_id`, `mapping_justification` (why: lexical match, manual curation...), `confidence`, and the provenance of the mapping. The predicates are
those of SKOS (`skos:exactMatch`, `closeMatch`, `broadMatch`, `narrowMatch`, `relatedMatch`), so that "how close" is part of the mapping. It is used for

* the formats: `public.pdf` -> `application/pdf`, `public.plain-text` -> `text/plain`, `public.source-code` -> `text/x-source-code` (so that an ontology written with UTIs
  finds its files on Haiku and the other way round);
* the established vocabularies: `org.sen-labs.schema.ScholarlyArticle` -> `https://schema.org/ScholarlyArticle` (exact), `org.sen-labs.people.close-friend` -> `rel:closeFriendOf`
  (exact), `org.sen-labs.people.superior` -> `rel:employerOf` (close);
* the types and attributes of ontologies of other authors that are merged with ours (the idea of the `sumo` repository: collect, unify, apply with the provenance of each mapping);
* the changes of a type over time: when an ontology renames or splits a type, the old identifier is mapped to the new with `skos:exactMatch` / `broadMatch`, which is the
  only versioning of types that is needed (open question 6 of the first draft).

The registry carries the mappings of an ontology (generated from the SSSOM file next to the schema); the file extensions of a type stay with the format (MIME database on Haiku,
`UTTypeTagSpecification` on macOS).

## 7. Migration

None for files that exist only as tests. The definitions are migrated (the schemas, the ontologies); the MIME database of a user is cleaned of everything that came from SEN, its
ontologies and its plugins (the installer knows what it installed: the types of the ontology placeholders), and the new packages bring the registry instead. A tool that retypes
existing files (`BEOS:TYPE` of the old entities to `entity/person` + `META:TYPE`, the renaming of the `SEN:REL:` attributes) is only written when a release is out that people
have data in. Until then the Haiku People files (`application/x-person`) are the old type of a person: the registry names it as a broader match of `org.sen-labs.person` so that
both are found.

## 8. Decisions and open questions

Decided (2026-10-16):

2. **BEOS:TYPE of files that exist for SEN only:** one coarse type for each kind (`relation/personal`, `relation/organisational`, `entity/person`...): icons and default columns for
   free.
3. **Plugins with the same best match:** the most specific `META:TYPE` / format match wins. If two or more are equally specific, the user is asked once and the answer is
   stored, for that file or for the type, like the preferred application of a format.
4. **Depth of menus:** not limited for now (a submenu is the established paradigm on Haiku; macOS can look at it later).
5. **Aliases and mappings of other vocabularies:** SSSOM, section 6.
6. **Versions of types:** no scheme of its own, the mappings (`skos:exactMatch`, `broadMatch`...) say how an old type relates to the new one.

Open:

1. **The trees of the people ontology**, which should stay close to the established ontologies of people (FOAF, REL) and fit the registry without much work: to be discussed with the
   first people types, after the paper.

## 9. Steps

1. **Scientific paper** as the first type of this design: no MIME type, `META:TYPE = org.sen-labs.paper` (with the mapping to `schema:ScholarlyArticle`) on the PDF and EPUB files
   that Paperboy identified; the schema of the papers ontology as the single source, with the attributes it has now.
2. `SenTypes.h` (reader of the registry; tests with the registry of the papers ontology), the generator output (`data/sen/types/<ontology>` and the SSSOM mappings): `Find`,
   `ConformsTo`, `Attributes`, `RelationTypes` first.
3. Server: relation types and configs from the registry; queries by descendants; the choice of plugins (specific match, asking the user).
4. Akita: the Attributes menu from the registry (along the conformance chain), then the menus of relations as a tree; FileTypes of SEN: a view of the registry.
5. Ontologies: people (after the review of the trees), then the others; the cleaning of the MIME database of SEN's types.
6. macOS: the same schema generates the UTI declarations.

### Status (2026-10-17)

* Step 1 (paper): schema, generator, `types/org.sen-labs.{paper,document}`, SSSOM mappings, `sen_oni` (types are compiled for the registry, the MIME types that an ontology no
  longer provides are removed), Paperboy writes `org.sen-labs.paper`.
* Step 2: `SenTypes.h` with `Find`, `Ancestors`, `ConformsTo`, `Descendants`, `ForFormat`, `OfOntology`, `Attributes`, mappings (`sen-core/tests/vm/types.sh`).
  `RelationTypes` is not there yet.
* Step 4: the SEN menu of the ontologies lists the types of the registry with their attributes; Akita's Attributes menu has a menu for the `META:TYPE` of the files that is in
  the registry. FileTypes does not show the registry yet; the entries are information only.
