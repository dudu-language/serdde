# Direct Wire Architecture And Delivery Record

This document defines the completed direct-wire architecture and retains the
delivery requirements used to audit it. Serdde moved from a mandatory
recursive `Value` tree to direct typed serialization and deserialization,
added a measured native JSON backend, and added deterministic CBOR without
coupling derives to a format.

The work is complete only when normal typed JSON, CBOR, and DSON operations no
longer allocate or traverse an intermediate `Value` tree. `Value` remains a
public dynamic-data feature implemented as one ordinary backend.

## Goals

- Preserve the `@derive(Serialize)`, `@derive(Deserialize)`, and
  `@derive(Serde)` user experience.
- Serialize typed values directly into a format writer.
- Deserialize format input directly into the destination type.
- Use an optimized native JSON engine where measurements justify it.
- Add interoperable, deterministic CBOR with stable schema-evolution rules.
- Preserve Serdde's structured paths, source locations, configuration, and
  adapters.
- Keep generated code inspectable Dudu with no Serdde-specific compiler logic.
- Make the direct path measurably competitive with established native
  libraries in runtime, allocations, and memory use.

## Non-Goals

- Serializing C++ object memory, padding, vtables, pointers, or ABI layout.
- Claiming zero allocation when the destination owns strings or containers.
- Making JSON or CBOR concepts part of the Dudu compiler.
- Hiding unsupported behavior behind a `Value` fallback.
- Preserving an internal generated-code API that has not been released.
- Implementing source-borrowed strings without an explicit input-lifetime
  model.

## Required Architecture

The dependency direction becomes:

```text
derived or handwritten typed codec
              |
              v
serializer / deserializer protocol
       |          |          |
       v          v          v
     JSON        CBOR       DSON
       |
       +-----------------------------+
                                     v
                              optional Value backend
```

Formats consume generated typed codecs. Generated codecs do not construct
format values and do not name JSON, CBOR, DSON, or a native parsing library.

`to_value` and `from_value` use `ValueSerializer` and `ValueDeserializer`.
They are not the implementation beneath `dumps` and `loads`.

## Protocol Surface

The concrete spelling may tighten during implementation, but the structured
operations must cover:

- null, bool, char, signed integers, unsigned integers, and floats;
- owned strings and validated UTF-8;
- sequence begin, element, and end;
- object begin, named field, unknown-field skip, and end;
- enum unit variants and named-payload variants;
- definite collection lengths where known;
- serializer errors and deserializer errors with source positions;
- decoder checkpoints or bounded value replay for untagged variants;
- adapters that serialize through the protocol instead of returning `Value`;
- recursive and generic codecs using ordinary Dudu generic dispatch.

Generated serialization should have the conceptual form:

```python
def serdde_serialize[S](self: &const[Self], output: &S) -> Result[None, SerddeError]:
    output.begin_object(2)
    output.field("id")
    serialize(self.id, output)
    output.field("name")
    serialize(self.name, output)
    output.end_object()
```

Generated deserialization should consume fields in any input order, track
which required fields were seen, reject duplicate aliases for one field, skip
or reject unknown fields according to policy, and construct the destination
once its required data is available.

The implementation must use static generic dispatch where possible. It must
not add virtual calls to every scalar or field operation merely to make the
protocol format-neutral.

## Allocation Contract

For typed `dumps(value)`:

- no recursive `Value` objects;
- no per-field heap allocation;
- no object-key dictionary;
- one growable output buffer is acceptable;
- callers may provide a reusable or pre-sized output buffer;
- temporary storage for deterministic key ordering must be avoided for
  generated objects whose keys are already statically known.

For typed `loads[T](source)`:

- no Serdde `Value` tree;
- no per-token heap allocation;
- no second owned copy of the complete input;
- allocations required by destination `str`, `list`, `dict`, and `set` values
  are acceptable and measured;
- strings containing escapes may require materialization;
- format-engine arena allocation is measured separately and accepted only
  when it materially improves the complete operation.

True borrowed decoding is a separate API because the returned object would
depend on the source buffer's lifetime. It must not be implied by `loads[T]`.

## JSON Backend

The JSON backend must be selected by measured complete typed operations, not
parser-only marketing numbers.

Evaluate at least:

- simdjson On-Demand for direct typed reading;
- RapidJSON SAX Reader and Writer as a streaming reference;
- yyjson as a compact C integration and high-performance DOM baseline;
- Glaze as a direct-to-object performance reference, accounting for its C++
  version requirements and compile-time cost.

The selected production path must:

- build through normal Dudu/CMake dependency configuration;
- work on supported Linux and macOS targets;
- keep native types behind a small backend boundary;
- preserve exact integer range checking and non-finite-float policy;
- reject malformed UTF-8, duplicate keys, trailing input, and excessive
  nesting;
- provide byte offset, line, and column when the native engine exposes enough
  information, deriving them efficiently otherwise;
- support direct writing without a native DOM;
- keep Serdde field paths when native errors are converted.

If the fastest reader and writer come from different libraries, using both is
acceptable when the dependency and compile-time cost remains reasonable. A
small direct writer using proven escaping and number-formatting primitives is
also acceptable when benchmarks beat a heavier dependency.

## Difficult Features

These features are part of the direct architecture, not deferred exceptions.

### Flattening

Generated flattened objects share one parent key space. Macro expansion must
compute or generate field routing that detects collisions and dispatches keys
without first building a generic object. Runtime skip predicates must produce
the correct emitted object length for definite-length formats.

### Untagged Enums

The deserializer protocol must support a bounded checkpoint/replay operation
for the current encoded value, or an equivalent raw-slice retry mechanism.
Trying variants must not require constructing a `Value`. Failure reports the
most useful candidate errors without unbounded reparsing.

### Adapters

Adapters receive a serializer or deserializer and participate in the direct
path. Direct adapters receive the active writer or reader. An explicit adapter
that intentionally requests a dynamic `Value` is allowed and pays that cost
only for its own subtree.

### Conditional Fields

For JSON, field count is not encoded. For deterministic CBOR, generated code
must compute the final map length before writing it or use a backend mechanism
that produces an equivalent definite-length map. Skip predicates and flattened
objects must be included in this count.

## CBOR Wire Contract

CBOR support follows RFC 8949 and has two explicit object representations.

### Stable Default

- Classes encode as maps keyed by serialized field names.
- Decoders accept fields in any order.
- Source declaration order is not part of the wire contract.
- Unknown, missing, renamed, aliased, skipped, and flattened fields follow the
  same policy as JSON.
- Enum representations follow Serdde configuration rather than native memory
  layout.
- Deterministic encoding is the default for emitted bytes.
- Preferred integer and length encodings are used.
- Map keys follow RFC 8949 deterministic ordering.
- Indefinite-length values are not emitted in deterministic mode.

### Compact Schema Mode

Compact maps may use explicit stable integer IDs:

```python
@derive(Serde)
class Player:
    @Serde(id=1)
    name: str

    @Serde(id=2)
    hp: i32
```

IDs are validated for uniqueness during macro expansion. They are never
derived from declaration position, hashes with unspecified stability, native
offsets, or compiler output. Removed IDs are not silently reused.

Positional array encoding is not a default object representation. If added as
an explicit protocol option, it requires a declared schema/version and is
documented as incompatible with field reordering.

### CBOR Backend Selection

Evaluate maintained C/C++ streaming implementations such as QCBOR and
TinyCBOR, plus a narrowly scoped direct implementation if existing libraries
cannot preserve Serdde paths or deterministic behavior efficiently. Do not use
a native DOM merely to replace the Serdde `Value` tree with another mandatory
tree.

Validate against RFC 8949 examples, external test vectors, and at least one
independent implementation. Deterministic golden bytes are checked into tests.

## Work Plan

### 1. Lock Existing Semantics

- Add behavior fixtures for every scalar and supported container family.
- Cover nested/generic/recursive classes and payload enums.
- Cover all enum representations, aliases, defaults, skips, flattening,
  adapters, unknown fields, duplicate keys, and malformed inputs.
- Record current public JSON output where it is intentionally stable.
- Add allocation and peak-memory instrumentation to the benchmark harness.
- Keep heavyweight native comparisons outside the default edit loop.

### 2. Define Direct Protocols

- Add focused serializer, deserializer, sequence, object, and enum protocol
  modules.
- Define ownership, error propagation, nesting, checkpoint, and skip behavior.
- Implement scalar and collection generic dispatch.
- Add a protocol conformance backend that records events for deterministic
  unit testing.
- Keep production files within the repository size guidance.

### 3. Invert `Value`

- Implement `ValueSerializer` and `ValueDeserializer` using the direct
  protocols.
- Reimplement `to_value`, `from_value`, and `_with` APIs on those backends.
- Confirm all existing dynamic-tree tests pass before changing wire formats.
- Remove typed-codec dependencies on `Value`, `ValueKind`, and conversion
  helpers.

### 4. Regenerate Derives

- Change class and enum derives to emit protocol-based methods.
- Migrate primitives, containers, arrays, `Option`, `Result`, and `variant`.
- Implement direct flatten routing, conditional-field counts, and untagged
  retry behavior.
- Migrate field and top-level adapters.
- Verify `duc expand`, diagnostics, LSP navigation, and generated-source
  readability.
- Delete obsolete tree-specific macro helpers once no caller remains.

### 5. Implement Direct JSON

- Build reproducible candidate benchmarks using equivalent typed records.
- Select and document the native reader/writer architecture.
- Integrate dependencies through normal CMake/package configuration.
- Implement direct `dumps`, `loads`, reusable-buffer, and writer APIs.
- Preserve explicit `dumps_value` and `loads_value` operations in each
  format's `*_value` module.
- Run JSON conformance and malformed-input suites.
- Delete the old typed parse-to-Value-to-type and
  type-to-Value-to-write paths.

### 6. Implement Direct DSON

- Move DSON readers and writers onto the same protocols.
- Preserve exact signed, unsigned, and floating categories.
- Keep DSON useful as a deterministic debugging and conformance format.
- Ensure it no longer proves format neutrality by paying for a mandatory tree;
  it proves it by implementing the common protocol.

### 7. Implement Deterministic CBOR

- Add `serdde.cbor` typed and dynamic APIs.
- Implement all scalar, sequence, object, enum, and recursive forms.
- Implement stable string-key maps and explicit integer-ID compact maps.
- Add canonical/deterministic output and schema-evolution fixtures.
- Test field reorder, field addition/removal, aliases, unknown fields, missing
  fields, and source recompilation stability.
- Validate interoperability and golden bytes externally.

### 8. Harden Errors

- Preserve field/index/variant paths across every backend.
- Preserve JSON and CBOR byte offsets and useful source positions.
- Report integer overflow, wrong kinds, duplicate fields, invalid UTF-8,
  unknown fields, and failed untagged candidates precisely.
- Ensure native library names and template errors do not leak through the
  public API when a Serdde diagnostic can explain the problem.

### 9. Establish Performance Gates

- Benchmark small records, deeply nested records, large strings, large arrays,
  maps, enums, and malformed input.
- Report encode/decode throughput, latency, allocations, peak RSS, output size,
  generated C++ size, and compile time.
- Compare direct Serdde JSON with its selected native engine, nlohmann/json,
  Glaze, and Rust Serde with `serde_json` using equivalent fixtures.
- Compare Serdde CBOR with a maintained Serde-compatible CBOR backend and at
  least one independent native implementation.
- Retain all comparison sources and record toolchain, dependency, optimization,
  and machine details so results are reproducible.
- Report cold and warm compilation cost and binary size alongside runtime,
  allocation, memory, and wire-size measurements.
- Require zero `Value` allocations on normal typed operations.
- Investigate regressions rather than weakening benchmark fixtures.

### 10. Finish Public Documentation

- Rewrite architecture and API docs around direct protocols.
- Document dynamic `Value` as an explicit choice and cost.
- Document JSON backend dependencies and portability.
- Document CBOR deterministic and compact schema rules.
- Add direct JSON, dynamic JSON, CBOR, schema evolution, adapters, and reusable
  buffer examples.
- Update README, changelog, generated-code docs, and benchmark results.

### 11. Validate As A Package

- Consume Serdde through path and pinned Git dependencies from clean projects.
- Run `scripts/test_package_consumers.sh` for the clean path consumer and
  `scripts/test_package_consumers.sh --git-ref REV` for an exact published
  commit or tag.
- Run default tests, examples, compile-fail fixtures, generated-code checks,
  conformance suites, and release benchmarks locally.
- Build and execute the root `CMakeLists.txt` from a clean temporary build
  directory with `scripts/test_cmake.sh`.
- Run relevant Dudu regressions for any general compiler fixes.
- Commit and push stable green milestones rather than one unreviewable final
  rewrite.

## Completion Criteria

- Typed JSON, CBOR, and DSON do not construct `Value` trees.
- `Value` remains available through explicit dynamic APIs.
- Existing derive behavior and schema controls work on every applicable
  backend.
- JSON uses a measured native-optimized path and direct writer.
- CBOR output is interoperable and deterministic across recompiles.
- Field declaration reordering does not change decoded meaning.
- Compact field IDs are explicit and stable.
- Structured diagnostics retain useful paths and source positions.
- Benchmarks report allocations and memory, not only elapsed time.
- Equivalent Rust Serde JSON and CBOR benchmarks are checked in and reproducible.
- No Serdde or native-library special cases exist in the Dudu compiler.
- Documentation describes the implemented architecture directly.
