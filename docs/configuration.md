# Derive Configuration

Serdde helper attributes use the imported derive name.

```python
@derive(Serde)
@Serde(rename_all="camelCase", deny_unknown_fields=True)
class Player:
    player_id: u64

    @Serde(rename="displayName", alias=["name", "display_name"])
    display_name: str
```

## Container Options

| Option | Meaning |
| --- | --- |
| `rename_all` | Rename fields or variants using `snake_case`, `camelCase`, `PascalCase`, `kebab-case`, or `SCREAMING_SNAKE_CASE`. |
| `deny_unknown_fields` | Reject object keys not consumed by a field or flatten target. |
| `tag` | Use internal enum tagging with this key. |
| `content` | Together with `tag`, use adjacent enum tagging. |
| `untagged` | Try enum variants in declaration order without a tag. |

External enum tagging is the default.

## Field And Variant Options

| Option | Meaning |
| --- | --- |
| `rename` | Change the serialized name. |
| `alias` | Accept one or more old names while deserializing. |
| `default` | Use the field's declared default, the type default, or a named function when absent. |
| `skip` | Omit the field in both directions and use its default while decoding. |
| `skip_serializing` | Omit only while encoding. |
| `skip_deserializing` | Ignore input and use the default while decoding. |
| `skip_if` | Omit while encoding when the named predicate returns true. |
| `flatten` | Merge a nested object into its containing object. |
| `adapter` | Use the named adapter class's static `serialize` and `deserialize` methods for this field. |

Invalid combinations are compile-time macro diagnostics. Examples include
`flatten` on a non-object codec, `skip` without a usable default, multiple enum
tagging modes, duplicate serialized names, and aliases that collide.

## External Types

Imported C++ and third-party Dudu types do not need global protocol
registrations. An explicit adapter can own their wire representation:

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


@derive(Serde)
class Entry:
    @Serde(adapter="PairSerde")
    pair: std.pair[std.string, i32]
```

Standalone wire values use a format's `dumps_with[T, Adapter]`,
`loads_with[T, Adapter]`, and `dumps_into_with[T, Adapter]`. Explicit dynamic
conversion uses `to_value_with[T, Adapter]` and
`from_value_with[T, Adapter]`. Adapter lookup is explicit, so two packages can
provide different representations for one imported type without an
import-order-dependent global conformance rule.

## Schema Evolution

The default policy ignores unknown fields and requires fields without defaults.
Aliases allow old names during migrations. Renaming affects output and the
primary accepted input name. Defaults allow fields to be added without breaking
older documents.
