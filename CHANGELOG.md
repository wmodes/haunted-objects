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
  to espressif32 8.0.0).
- Repository layout: one PlatformIO project per haunted object, each in its own top-level folder;
  the smoke alarm piece goes in `detectors/`.
