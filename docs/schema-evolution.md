# Schema Evolution

Serdde's default object policy is:

- missing fields are errors unless they have a default;
- unknown fields are ignored;
- serialized field names match Dudu field names;
- enum variants use external tagging.

These defaults allow readers to accept fields added by a newer writer while
still rejecting incomplete records.

## Rename A Field

Use `rename` for the current wire name and `alias` for names accepted from old
documents:

```python
@derive(Serde)
class Player:
    @Serde(rename="displayName", alias=["name", "display_name"])
    display_name: str
```

Writers emit `displayName`. Readers accept all three names and reject a
document that supplies more than one of them.

## Add A Field

Add a default when old documents do not contain the new field:

```python
@derive(Serde)
class Player:
    id: u64

    @Serde(default=True)
    enabled: bool
```

Use `default_with` when construction needs a function:

```python
def default_region() -> str:
    return "unknown"


@Serde(default_with="default_region")
region: str
```

## Remove A Field

`skip_deserializing` ignores input and initializes from a default.
`skip_serializing` stops new writers from emitting a field while readers still
accept it. `skip` applies both rules.

Directional changes should be deployed in stages when old and new programs
run at the same time.

## Strict Schemas

Use `deny_unknown_fields=True` when an unexpected key must be rejected:

```python
@derive(Serde)
@Serde(deny_unknown_fields=True)
class Request:
    id: u64
```

This is useful at validation boundaries. It reduces forward compatibility, so
it should be selected deliberately.

## Enum Representations

The default externally tagged form is explicit and unambiguous. Internal
tagging uses `tag`, adjacent tagging uses both `tag` and `content`, and
`untagged=True` tries variants in declaration order.

Variant `rename` and `alias` support the same migration pattern as fields.
Untagged schemas should keep variants structurally distinct because ordering
becomes part of decoding behavior.

## Flattening

`flatten=True` merges a nested object into its parent. Flattening is useful for
schema composition but creates one shared key space. Serdde rejects duplicate
keys during encoding and invalid combinations during macro expansion.
