# Changelog

All notable changes to Serdde are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and releases use
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Format-neutral `Value` representation with ordered objects.
- Structured errors with value paths and source locations.
- Strict JSON parsing and deterministic JSON writing.
- Checked primitive conversion and generic handwritten codec contracts.
- Public typed derives for classes with nested derived fields, lists, sets,
  string-keyed dictionaries, `Option`, `Result`, and arbitrary-rank fixed
  arrays. Generated failures retain field and index paths.
- Class derives now support `rename`, `rename_all`, aliases, defaults and named
  default functions, directional skips, skip predicates, flattening, duplicate
  input detection, and optional rejection of unknown fields.
- Enum derives now support unit and named-payload variants with external,
  internal, adjacent, and untagged representations. Variant aliases,
  directional skips, unknown-field policy, deterministic tag order, and
  collision-safe generated bindings are covered by executable fixtures.
