# Serdde

Serdde is a format-neutral serialization framework for Dudu. It follows the
useful parts of Rust Serde's data model and derive workflow while keeping a
Python-shaped Dudu API.

```python
from serdde import Serde
from serdde.json import dumps
from serdde.json import loads


@derive(Serde)
class Player:
    id: u64
    name: str


encoded = dumps(Player(id=7, name="Ada"))
decoded = loads[Player](encoded.value)
```

The serialization model is independent of JSON. JSON and DSON use the same
typed `Value` tree and derived conversion code.

## Status

Serdde is under active implementation against the Dudu alpha toolchain. The
public behavior and completion requirements are tracked in
[`docs/architecture.md`](docs/architecture.md) and
[`docs/configuration.md`](docs/configuration.md).

## Development

Use a current Dudu checkout:

```bash
dudu test
dudu run tests
duc expand examples/derive.dd --show-origins
```

The repository does not require compiler knowledge of Serdde. Derives use the
public `dudu.ast` macro package and generated code is ordinary checked Dudu.

## License

Apache-2.0 OR MIT.
