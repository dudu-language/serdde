# Serdde Architecture

Serdde separates typed conversion from wire formats. The current implementation
uses `Value` as its common conversion layer. The target architecture replaces
that mandatory tree with direct serializer and deserializer protocols; the
complete migration is specified in [Direct Wire And Binary Plan](direct-wire-plan.md).

## Layers

1. `serdde.value` defines the recursive, format-neutral `Value` model.
2. Derived and handwritten codecs convert Dudu values to and from `Value`.
3. Format modules parse and write `Value` without knowing user types.
4. Convenience APIs compose a typed codec with a format.

This is the current implementation structure, not the permanent dependency
direction. After the direct-wire migration, generated codecs target structured
format-neutral protocols. JSON, CBOR, DSON, and `Value` implement those
protocols as peer backends. `Value` remains available for callers that
intentionally need dynamic data, but typed `dumps` and `loads` do not traverse
it.

JSON is not embedded in generated derives. Adding a format does not require a
new derive.

## Public API

`@derive(Serialize)` adds:

```python
def serdde_serialize(self: &const[Self]) -> Result[Value, SerddeError]
```

`@derive(Deserialize)` adds:

```python
def serdde_deserialize(value: &const[Value]) -> Result[Self, SerddeError]
```

The method has no `self` parameter and is therefore a static Dudu method.
`@derive(Serde)` adds both methods.

The generic entry points use ordinary Dudu use-site requirements:

```python
def to_value[T](value: &const[T]) -> Result[Value, SerddeError]:
    return value.serdde_serialize()


def from_value[T](value: &const[Value]) -> Result[T, SerddeError]:
    return T.serdde_deserialize(value)
```

Concrete generic instantiations are checked by Dudu. A type without the
required method receives a Dudu diagnostic at the call site.

## Generated Code

Derives generate typed code for each field and variant. Primitive conversion
uses explicit library functions. User-defined values call their derived or
handwritten methods. Container conversion emits ordinary loops and recursively
applies the same rules to element types.

This avoids:

- runtime reflection;
- compiler special cases for Serdde;
- C++ return-type overload tricks;
- a dynamically typed `any` boundary;
- format-specific generated code.

Generated code is inspectable with `duc expand` and participates in normal
diagnostics, LSP navigation, and C++ emission.

## Data Model

`Value` has these kinds:

- null;
- bool;
- signed integer;
- unsigned integer;
- float;
- UTF-8 string;
- sequence;
- ordered string-keyed object.

The model preserves integer sign and width category well enough for checked
typed conversion. A format may reject values it cannot represent.

Objects preserve insertion order for deterministic output. Lookup uses a map;
ordering uses a parallel key list. Duplicate keys are rejected by parsers.

## Errors

Every error has:

- a stable `ErrorKind`;
- a message;
- a typed path made of field and index segments;
- optional byte offset, line, and column;
- optional format name and source cause.

Nested conversion prepends path segments while an error returns to the caller.
JSON syntax errors report one-based line and column positions.

## Supported Type Families

The completed derives cover:

- scalar Dudu types and strings;
- classes and generic classes;
- unit and named-payload enums;
- nested and recursive models;
- `list`, `dict`, `set`, and fixed arrays;
- `Option`, `Result`, and `variant`;
- fields containing another derived or handwritten Serdde type.

Raw pointers and references are not serialized implicitly. A type may provide a
handwritten codec when ownership or external identity has a domain-specific
meaning.

## Formats

JSON is standards-compliant and tested against external conformance data.
DSON is a deterministic typed text format used to prove that derives are not
coupled to JSON. DSON preserves every `Value` kind exactly.

## Native Boundary

Serdde is implemented in Dudu. Native libraries may be used for optional
adapters and benchmark comparisons, but the compiler must not recognize the
library name or its types. A discovered language gap is reproduced with a
neutral Dudu fixture and fixed generally in Dudu.

External types use explicit adapter classes. Field adapters are selected with
`@Serde(adapter="...")`; standalone values use `to_value_with` and
`from_value_with`. This proved sufficient for imported C++ templates including
`std.pair` without adding traits, compiler registries, orphan rules, or
Serdde-specific lookup to Dudu.
