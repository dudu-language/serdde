# Wire Formats

JSON, CBOR, and DSON implement the same typed serializer/deserializer protocol.
They do not share an intermediate `Value` tree.

## JSON

JSON is the text interchange format. Typed reads use yyjson behind a private
reader boundary; typed writes use a direct writer.

```python
from serdde.json import dumps
from serdde.json import loads

encoded = dumps(player)
decoded = loads[Player](encoded.value)
```

The backend rejects:

- malformed syntax and trailing input;
- duplicate object keys;
- invalid UTF-8 and malformed escapes;
- non-finite floating-point output;
- integers outside the requested Dudu type;
- nesting deeper than the configured limit.

JSON cannot distinguish signed from unsigned integer spelling. Decoding checks
the exact destination range.

## CBOR

CBOR is the compact binary interchange format. It follows RFC 8949 and uses
deterministic output.

```python
from serdde.cbor import dumps
from serdde.cbor import loads

bytes = dumps(player)
decoded = loads[Player](bytes.value)
```

The returned `str` contains arbitrary bytes and should not be printed as UTF-8.

Default classes encode as maps keyed by serialized field names. The decoder
accepts fields in any order. Declaration order is not part of the wire schema.
Preferred integer and length encodings are used, map keys follow deterministic
ordering, and indefinite-length output is not emitted.

Compact schema mode uses explicit stable IDs:

```python
@derive(Serde)
@Serde(compact=True)
class Player:
    @Serde(id=1)
    name: str

    @Serde(id=2)
    hp: i32
```

IDs must be unique non-negative integers. They are not inferred from field
order, names, hashes, native offsets, or ABI layout. Reordering these
declarations leaves deterministic bytes unchanged.

CBOR fixtures include malformed input, deterministic golden bytes, schema
evolution, aliases, unknown fields, duplicate IDs/names, RFC-style vectors,
and bidirectional Rust `ciborium` interoperability.

## DSON

DSON is deterministic typed text for exact tests and debugging:

```text
null
bool(true)
i64(-2)
u64(2)
f64(2.5)
str(3:Ada)
seq(3:[i64(1),u64(2),f64(3.0)])
obj(2:{str(2:id)=u64(7),str(4:name)=str(3:Ada)})
```

String lengths are byte counts. Sequence and object lengths are item counts.
The parser rejects count mismatches, duplicate keys, malformed numbers,
invalid UTF-8, trailing input, and excessive nesting.

DSON is not intended as a general ecosystem standard. It makes wire categories
visible and verifies format-neutral derive behavior without binary inspection.

## Dynamic Data

Dynamic parsing and writing require explicit modules:

```python
from serdde.json_value import dumps_value
from serdde.json_value import loads_value
from serdde.value import Value

tree: Result[Value, SerddeError] = loads_value(source)
encoded = dumps_value(tree.value)
```

Equivalent APIs are in `serdde.cbor_value` and `serdde.dson_value`. Paying for
a `Value` tree is therefore visible at the import and call site.

## Reusable Output

Each typed backend provides `dumps_into(value, destination)`. The destination
string is cleared and reused. Adapter calls use `dumps_into_with`.
