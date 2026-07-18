# Serdde Engineering Rules

Serdde is a format-neutral serialization framework written in Dudu. JSON is a
backend, not the framework's type model. Keep the implementation usable as a
normal Dudu package and do not depend on compiler branches that recognize this
library by name.

## Structure

- Keep production source files focused and normally between 300 and 500 lines.
- Split parsers, writers, value modeling, errors, derive macros, and format
  adapters by responsibility.
- Keep the public import surface small. Internal modules may be numerous.
- Put fast deterministic tests in `tests/`; keep benchmarks and heavyweight
  compatibility probes out of the default edit loop.
- Examples are executable documentation and must use the public API.

## Implementation

- Prefer ordinary Dudu code, generics, macros, and package dependencies.
- C and C++ interop is allowed where it is the public interoperability feature
  being demonstrated, not as a hidden replacement for Serdde behavior.
- Preserve structured errors through every layer. Do not discard value paths,
  source offsets, JSON lines, or columns.
- Generated derive code must be inspectable and type checked like handwritten
  Dudu code.
- Do not special-case imported libraries in the Dudu compiler.
- If Serdde exposes a general compiler defect, first add a neutral fixture to
  Dudu, then fix the general language behavior.
- Avoid compatibility aliases and legacy syntax. Neither project has a stable
  release that requires preserving mistakes.

## Validation

- Run the narrowest relevant checks while editing.
- Before a milestone commit, run the Serdde test target, examples, generated
  code inspection, and the relevant Dudu regression tests.
- Verify path and Git dependency consumption from a clean external project.
- Benchmark release builds and report parser, writer, allocation, and total
  time separately where practical.
- Keep compiler and editor validation local. Remote jobs are release checks,
  not the development loop.

## Git

- Commit stable green milestones with factual messages.
- Push both Serdde and any required general Dudu fixes at reasonable points.
- Do not commit build directories, generated binaries, dependency caches, or
  benchmark output.
