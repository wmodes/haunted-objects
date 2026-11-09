# Changelog

All notable changes to this project are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Git repository, `.gitignore` (ignores `CLAUDE.md`, `.pio/`, `secrets.h`, `.env`, `.DS_Store`).
- Design Document with concept, behavior (ambient with interludes), hardware, tooling, open
  questions, and the project lexicon.
- README, CHANGELOG, and Dev README.
- `detectors/` PlatformIO project (classic ESP32, `esp32dev`, Arduino framework, platform pinned
  to espressif32 8.0.0), with its own README.
- Design decision: layered, typed config and secrets (shared defaults, per-piece overrides),
  with `Config` and `Secrets` added to the lexicon.
- Layered config and secrets code: `SharedConfig` and `SharedSecrets` in `shared/include/`,
  `Config` and `Secrets` in `detectors/include/`. Real secrets live in gitignored `secrets_*.h`
  files with committed `.h.example` templates.
- Repository layout: one PlatformIO project per haunted object, each in its own top-level folder;
  the smoke alarm piece goes in `detectors/`.
