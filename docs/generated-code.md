# Generated Code

Serdde derives are ordinary Dudu macros. The compiler contains no Serdde
registry, built-in serializer, or package-name check.

Inspect an expansion with:

```sh
duc expand examples/basic.dd --show-origins
```

Filter to one derive name when a source file uses several macros:

```sh
duc expand examples/basic.dd --macro Serde --show-origins
```

The output includes the macro invocation, macro definition, source
declaration, and generated methods.

## Serialization

For a class, the derive emits:

1. a field-count method that accounts for skips and flattened objects;
2. normal-order field serialization;
3. deterministic-order field serialization;
4. an object method that selects the backend's required ordering.

Each active field is written directly through `serialize_to`. A field adapter
calls its generic `serialize[Writer]` method. Errors gain the serialized field
name while returning.

No generated method names `Value`, `ValueKind`, `to_value`, `from_value`, or
`serdde.convert`. `scripts/test_generated_direct.sh` enforces this boundary.

## Deserialization

The generated method:

1. checks the encoded kind;
2. resolves primary names and aliases;
3. rejects duplicate aliases for one field;
4. applies defaults or reports a missing field;
5. decodes each present field directly into its declared type;
6. checks unknown fields when requested;
7. constructs the destination once.

Flattened fields receive the parent reader and route their own keys. Untagged
enums retry immutable bounded child readers; they do not construct a dynamic
tree.

## Diagnostics

Macro diagnostics point at source declarations and attributes. Compiler
diagnostics from generated declarations retain macro-origin notes. Generated
names are hygienic and tested against user fields resembling Serdde's internal
names.

Generated declarations participate in ordinary semantic analysis, LSP
navigation, and C++ emission. Derives emit AST nodes, not source strings.
