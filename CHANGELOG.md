# Changelog

All notable changes to Serdde are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and releases use
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Format-neutral direct serializer and deserializer protocols. Typed JSON,
  CBOR, and DSON operations no longer construct an intermediate `Value` tree.
- Explicit dynamic wire modules: `serdde.json_value`, `serdde.cbor_value`, and
  `serdde.dson_value`.
- Direct standalone adapter APIs for every wire format, including reusable
  output buffers.
- Deterministic RFC 8949 CBOR with name-keyed maps, explicit compact field IDs,
  schema-evolution fixtures, golden bytes, and Rust `ciborium`
  interoperability.
- yyjson-backed typed JSON reading and a direct JSON writer behind private
  backend boundaries.
- Reproducible JSON and CBOR benchmarks covering Serdde direct, the explicit
  `Value` baseline, Rust Serde, yyjson, simdjson On-Demand, RapidJSON SAX,
  nlohmann/json, and Glaze, including latency, throughput, allocation, memory,
  wire-size, artifact-size, and compilation measurements.
- Clean path and pinned-Git package consumers plus a standalone CMake build
  gate that exercise transitive native source and include configuration.

- Optional format-neutral `Value` representation with ordered objects.
- Structured errors with value paths and source locations.
- Strict dynamic JSON parsing and deterministic dynamic JSON writing.
- Typed JSON `loads[T]` and `dumps[T]`, exact numeric boundaries, UTF-8
  validation, nesting limits, and source-aware conversion errors.
- DSON, a deterministic typed text format that preserves every `ValueKind` in
  dynamic mode and directly implements typed protocols.
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
