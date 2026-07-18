# Changelog

All notable changes to Serdde are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and releases use
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Format-neutral `Value` representation with ordered objects.
- Structured errors with value paths and source locations.
- Strict JSON parsing and deterministic JSON writing.
- Checked primitive conversion and generic handwritten codec contracts.
- Public typed derives for classes with nested derived fields, lists, sets,
  string-keyed dictionaries, `Option`, `Result`, and arbitrary-rank fixed
  arrays. Generated failures retain field and index paths.
