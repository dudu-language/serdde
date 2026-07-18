# API

Serdde has three layers: typed conversion, a format-neutral `Value`, and wire
formats.

## Derives

Import derive macros from `serdde.macros`:

```python
from serdde.macros import Deserialize
from serdde.macros import Serde
from serdde.macros import Serialize
```

`@derive(Serialize)` generates:

```python
def serdde_serialize(self: &const[Self]) -> Result[Value, SerddeError]
```

`@derive(Deserialize)` generates a static method:

```python
def serdde_deserialize(value: &const[Value]) -> Result[Self, SerddeError]
```

`@derive(Serde)` generates both methods.

## Typed Conversion

Import from `serdde.convert`:

```python
from serdde.convert import from_value
from serdde.convert import from_value_with
from serdde.convert import to_value
from serdde.convert import to_value_with
```

```python
encoded: Result[Value, SerddeError] = to_value(player)
decoded: Result[Player, SerddeError] = from_value[Player](encoded.value)
```

The `_with` forms take an explicit adapter type for a value that cannot define
its own codec:

```python
encoded = to_value_with[std.pair[std.string, i32], PairSerde](pair)
decoded = from_value_with[std.pair[std.string, i32], PairSerde](encoded.value)
```

## JSON

Import from `serdde.json`:

```python
from serdde.json import dumps
from serdde.json import dumps_value
from serdde.json import loads
```

```python
text: Result[str, SerddeError] = dumps(player)
player: Result[Player, SerddeError] = loads[Player](text.value)
tree: Result[Value, SerddeError] = loads(text.value)
```

JSON rejects duplicate object keys, invalid UTF-8, non-finite floats,
out-of-range integers, malformed escapes, and nesting deeper than 128 values.
Output is compact and deterministic for an unchanged `Value` tree.

## DSON

`serdde.dson` has the same `loads`, `dumps`, and `dumps_value` shape as JSON.
DSON preserves signed integers, unsigned integers, and floats as distinct
`ValueKind` values. See [DSON](dson.md).

## Value

Import `Value` and `ValueKind` from `serdde.value`. A `Value` contains one of:

- null;
- bool;
- signed integer;
- unsigned integer;
- float;
- UTF-8 string;
- sequence;
- ordered string-keyed object.

Object lookup uses `has_field` and `field`. `set_field` preserves insertion
order for deterministic format output.

## Errors

Import `SerddeError`, `ErrorKind`, `PathSegment`, and `PathKind` from
`serdde.error`.

```python
match loads[Player](source):
    case Err(error):
        print(error.path_string() + ": " + error.message)
```

`path_string()` renders paths such as `$.players[2].name`. Parser failures also
provide one-based `line` and `column`, a byte `offset`, and the `format` name.

## Handwritten Codecs

A Dudu type may define `serdde_serialize` and static `serdde_deserialize`
directly instead of using a derive. The signatures must match the generated
methods. This is useful when the serialized representation differs materially
from the in-memory representation.
