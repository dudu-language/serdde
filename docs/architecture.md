# Serdde Architecture

Serdde derives target format-neutral serializer and deserializer protocols.
Formats and dynamic trees are peer backends.

```text
derived or handwritten typed codec
              |
              v
serializer / deserializer protocol
       |          |          |          |
       v          v          v          v
     JSON        CBOR       DSON      Value
```

Typed JSON, CBOR, and DSON calls do not invoke `to_value`, `from_value`, a
`Value` parser, or a `Value` writer. The typed format modules contain no
`Value` imports. Generated-source tests reject a derive expansion that names
the dynamic backend.

## Protocol

`serdde.protocol` provides generic operations for:

- null, bool, char, signed and unsigned integers, floats, and strings;
- lists, sets, fixed arrays, and string-keyed dictionaries;
- `Option`, `Result`, and `variant`;
- sequence and object boundaries;
- fields, field IDs, aliases, defaults, and unknown-field policy;
- classes, recursive classes, and all enum representations;
- direct field and standalone adapters.

Dispatch is static generic dispatch. Scalars and fields do not pay for a
virtual interface. A backend reader presents immutable child readers, so an
untagged enum can retry a bounded encoded subtree without materializing a
dynamic tree.

## Generated Code

`@derive(Serialize)` emits field counting, normal-order field writing,
canonical-order field writing, and object serialization. Conditional fields
and flattened objects contribute to the final definite object size before a
CBOR map is opened.

`@derive(Deserialize)` emits field lookup, duplicate-alias detection, checked
typed decoding, defaults, unknown-field handling, and one final constructor
call. Nested errors add field or index path segments while returning.

The macro imports only neutral defaults, errors, protocols, and variant
support. It does not import `serdde.convert` or a format implementation.

Generated declarations are ordinary Dudu AST nodes. They participate in
semantic analysis, source diagnostics, LSP navigation, and C++ emission. There
is no source-string macro output or compiler registry.

## JSON

The JSON reader uses yyjson behind `JsonReader`. yyjson validates and indexes
the document; Serdde then decodes destination fields directly. Immediate
object entries are indexed once, avoiding repeated linked-list traversal.

The JSON writer is a direct writer over a caller-owned or internal string. It
does not construct a native or Serdde DOM. It validates UTF-8, escapes strings,
uses locale-independent numeric conversion, and rejects non-finite floats.

Backend selection uses the complete typed benchmark matrix. yyjson is the
production reader because its compact C integration, checked numeric access,
and indexed object model performed well without exposing backend types. The
production writer stays as a small inline native boundary because wrapping a
streaming writer behind one out-of-line call per protocol event erased much of
that writer's standalone advantage. The direct writer was retained only after
measuring complete typed Serdde operations. RapidJSON SAX, simdjson On-Demand,
yyjson, Glaze, and nlohmann/json remain checked-in comparison implementations,
so this decision can be repeated rather than assumed.

Public errors convert native parse failures into `SerddeError` with JSON byte
offset, line, and column. Backend-specific types do not appear in public Dudu
signatures.

## CBOR

The CBOR backend is a narrowly scoped RFC 8949 reader/writer. Its indexed wire
document is not a public dynamic data model and is not copied into `Value` for
typed operations.

Default class representation is a deterministic map keyed by serialized field
names. Keys use RFC 8949 deterministic ordering. Compact classes use explicit
non-negative integer field IDs. Declaration position, hashes, C++ layout, and
ABI details never define the wire schema.

The implementation is validated with golden RFC-style vectors and bidirectional
interoperability against Rust `ciborium`.

## DSON

DSON is deterministic typed text. It visibly distinguishes signed integers,
unsigned integers, and floats, which makes it useful for debugging protocol
behavior. Its reader and writer implement the same direct protocol as JSON and
CBOR.

## Value

`serdde.value` is an optional recursive dynamic model. `ValueWriter` and
`ValueReader` implement the ordinary protocol. `to_value` and `from_value` are
therefore explicit backend calls, not the machinery behind typed wire APIs.

Dynamic wire modules are also explicit:

- `serdde.json_value`;
- `serdde.cbor_value`;
- `serdde.dson_value`.

## Allocation Contract

Typed encoding allocates no recursive `Value`, object-key dictionary, or
per-field dynamic node. An internal or caller-provided output string may grow.

Typed decoding allocates destination-owned strings and containers as required.
JSON's native parser arena and backend wire indexes are measured separately;
there is no second owned copy of the complete input and no Serdde `Value`
tree. Borrowed destination strings are not implied by `loads[T]`.

## Native Boundary

Native libraries stay behind reader/writer headers and normal Dudu import/build
configuration. Serdde-specific behavior is never added to Dudu. Compiler bugs
found while implementing Serdde receive neutral compiler fixtures and general
fixes.

The root `CMakeLists.txt`, clean path/Git consumers, and Dudu manifest all use
the same native inputs: the private Serdde headers and vendored yyjson C source.
This verifies that the backend works both from the repository and through
transitive package configuration.
