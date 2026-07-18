# Changelog

All notable changes to Serdde are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and releases use
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Format-neutral `Value` representation with ordered objects.
- Structured errors with value paths and source locations.
- Strict JSON parsing and deterministic JSON writing.
- Typed JSON `loads[T]` and `dumps[T]`, exact numeric boundaries, UTF-8
  validation, nesting limits, and source-aware conversion errors.
- DSON, a deterministic typed text format that preserves every `ValueKind` and
  proves derived codecs are independent of JSON.
- Explicit field and top-level adapters for imported and third-party types,
  validated with an imported C++ `std.pair`.
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
- Generic conversion entry points now support primitives, generic derived
  classes, nested generic classes, lists, sets, string-keyed dictionaries,
  `Option`, and both alternatives of `Result` through one ordinary Dudu
  overload family. Nested conversion failures retain field and index paths.
- Generated deserializers allocate hygienic input bindings around user field
  names, including classes with fields named `value` or `input`.
- Recursive classes and payload enums roundtrip without registration tables or
  handwritten codec functions.
- Anonymous `variant[...]` fields serialize as an index plus payload, including
  nested collection alternatives. Duplicate alternatives are rejected at
  macro expansion with a source diagnostic.
- Fixed-array derive coverage exercises ranks two through four through the same
  recursive codec generator.
- Executable examples cover basic JSON use, configuration, multiple formats,
  editor integration, and explicit adapters for imported C++ types.
- Package documentation covers the API, schema evolution, generated code,
  editor behavior, DSON, architecture, configuration, and benchmarks.
