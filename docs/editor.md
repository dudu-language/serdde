# Editor Support

Serdde uses Dudu's normal language server and macro-origin model. No Serdde
editor extension is required.

The fixture `examples/editor_fixture.dd` covers:

- go-to-definition on `Serde` and configuration attributes;
- hover on derived classes and named enum payload fields;
- semantic highlighting for derive decorators;
- diagnostics on invalid helper attributes;
- generated member visibility after expansion;
- references and completion on normal source fields.

Open the repository or an external project with the Dudu VS Code extension and
use that fixture for manual checks. Validate the source independently with:

```sh
dudu check examples/editor_fixture.dd
duc expand examples/editor_fixture.dd --show-origins
```

Generated methods are not written into the source file. `duc expand` is the
explicit view for inspecting them. Runtime examples remain ordinary Dudu files
and can be stepped through after C++ emission with the project's native
debugger setup.
