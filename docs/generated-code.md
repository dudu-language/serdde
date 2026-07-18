# Generated Code

Serdde derives are ordinary Dudu macros. The compiler does not contain a
Serdde registry, built-in serializer, or package-name check.

Inspect an expansion with:

```sh
duc expand examples/basic.dd --show-origins
```

Filter to one derive name when a source file uses several macros:

```sh
duc expand examples/basic.dd --macro Serde --show-origins
```

The output includes the macro invocation, macro definition, source
declaration, and generated methods. Generated methods use typed local values,
normal loops, normal overload resolution, and `Result` propagation.

For a class field, the generated serializer:

1. converts the field to `Value`;
2. prepends the field path if conversion fails;
3. inserts the converted value into an ordered object.

The generated deserializer performs the reverse operation, checks aliases and
defaults, and constructs the class with named arguments.

Macro diagnostics point at source declarations and attributes. Compiler
diagnostics from generated declarations retain macro-origin notes. Generated
names are hygienic and tested against user fields that resemble Serdde's own
temporary names.

Generated code is part of normal semantic analysis and C++ emission. It does
not use source-string output or a hidden dynamic dispatch boundary.
