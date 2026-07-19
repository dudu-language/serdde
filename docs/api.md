# API

Serdde has a typed protocol layer, three wire backends, and an optional dynamic
`Value` backend.

## Derives

```python
from serdde.macros import Deserialize
from serdde.macros import Serde
from serdde.macros import Serialize
```

`@derive(Serialize)` generates methods equivalent to:

```python
def serdde_field_count(self: &const[Self]) -> usize:
    ...

def serdde_serialize[Writer](
    self: &const[Self],
    output: &Writer,
) -> Result[bool, SerddeError]:
    ...
```

`@derive(Deserialize)` generates a static method:

```python
def serdde_deserialize[Reader](
    input: &const[Reader],
) -> Result[Self, SerddeError]:
    ...
```

`@derive(Serde)` generates both directions. Writers and readers are generic,
so one expansion serves JSON, CBOR, DSON, `Value`, protocol probes, and
third-party backends.

## Typed Wire APIs

JSON:

```python
from serdde.json import dumps
from serdde.json import dumps_into
from serdde.json import loads

text = dumps(player)
ready = loads[Player](text.value)

buffer = ""
dumps_into(player, buffer)
```

CBOR and DSON have the same typed API under `serdde.cbor` and `serdde.dson`.
CBOR returns binary bytes stored in `str`; it is not UTF-8 text.

`dumps_into` clears and reuses the destination string's allocation. It does not
append to the previous contents.

## Adapters

An adapter is format-neutral:

```python
class PairSerde:
    def serialize[Writer](
        value: &const[std.pair[std.string, i32]],
        output: &Writer,
    ) -> Result[bool, SerddeError]:
        ...

    def deserialize[Reader](
        input: &const[Reader],
    ) -> Result[std.pair[std.string, i32], SerddeError]:
        ...
```

Apply it to a field:

```python
@derive(Serde)
class Entry:
    @Serde(adapter="PairSerde")
    pair: std.pair[std.string, i32]
```

Apply it to a standalone value:

```python
from serdde.json import dumps_with
from serdde.json import loads_with

text = dumps_with[std.pair[std.string, i32], PairSerde](pair)
pair = loads_with[std.pair[std.string, i32], PairSerde](text.value)
```

`dumps_into_with` is the reusable-buffer form. These three adapter calls also
exist in `serdde.cbor` and `serdde.dson`.

## Dynamic Value

Dynamic trees are explicit:

```python
from serdde.json_value import dumps_value
from serdde.json_value import loads_value
from serdde.value import Value

tree: Result[Value, SerddeError] = loads_value(source)
text = dumps_value(tree.value)
```

Use `serdde.cbor_value` and `serdde.dson_value` for the other wire formats.

Typed conversion without a wire format is provided by `serdde.convert`:

```python
from serdde.convert import from_value
from serdde.convert import to_value

tree = to_value(player)
player = from_value[Player](tree.value)
```

`to_value_with` and `from_value_with` apply an adapter to this explicit dynamic
backend.

`Value` represents null, bool, signed integer, unsigned integer, float, UTF-8
string, sequence, or ordered string-keyed object. Object insertion order is
retained for dynamic text output.

## Errors

Import `SerddeError`, `ErrorKind`, `PathSegment`, and `PathKind` from
`serdde.error`.

```python
match loads[Player](source):
    case Err(error):
        print(error.path_string() + ": " + error.message)
```

`path_string()` renders paths such as `$.players[2].name`. Errors also carry a
format name and, where available, one-based line/column plus byte offset.
Integer overflow, kind mismatch, missing fields, duplicate aliases, unknown
fields under strict policy, invalid UTF-8, malformed input, and failed
untagged variants use structured errors rather than native-library messages.

## Handwritten Codecs

A Dudu type can implement the generated protocol methods directly. This is
appropriate when the wire representation differs from the in-memory model.
An adapter is preferable when the type is external or when more than one wire
representation is useful.
