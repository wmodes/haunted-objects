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
- Blink experiment (`detectors/experiments/blink/`): `HeartbeatBlinker` toggles the on-board LED
  and logs a heartbeat over serial, using the piece's layered `Config` and `Secrets`. Verified on
  the ELEGOO ESP32 board.
- Design decision: tests (formal, `test/`) are separate from experiments (throwaway,
  `experiments/`, one-way dependency); `test` and `experiment` added to the lexicon.
- Detector profiles: `detectors/include/DetectorProfiles.h` gives each of 12 slots a fixed chirp
  length, frequency, and interval, with a `model` recording where the values came from, validated
  at compile time. Design Document records the research behind the ranges; `slot` and
  `detector profile` added to the lexicon.
- Design decision: a slot's voice (chirp length and frequency) is the same in every suite; only
  timing varies by suite. `voice` and `timing` added to the lexicon. The `chirpIntervalSeconds`
  field comment now says it is the ambient low-battery interval (timing, not voice).
- Changed: detector profile chirp frequencies redrawn from 3000-3500 Hz (was 2850-3250 Hz, an
  unsourced guess that sounded too low); lengths, intervals, and models unchanged. Research on
  typical smoke-alarm pitch (about 3100-3200 Hz) recorded in the Design Document.
- Chirp-table experiment (`detectors/experiments/chirp-table/`): a host-only PlatformIO `native`
  program that renders slots 1-6 of the detector profile table to a WAV, with a random warmup
  offset per slot and a `waveform` setting (sine default; square and sawtooth). Verified by
  analyzing the WAV against the table. `*.wav` is gitignored.
- Design Document: research on the horn drive waveform (square, push-pull; no evidence found for
  sawtooth) and the open question of which sounds right.
- Wokwi emulation: the blink experiment runs on a simulated ESP32 via `wokwi-cli`
  (`wokwi.toml`, `diagram.json`), passing on its serial output. The API token lives in the
  gitignored `.env`; `.env.example` is the committed template.
- Layered config and secrets code: `SharedConfig` and `SharedSecrets` in `shared/include/`,
  `Config` and `Secrets` in `detectors/include/`. Real secrets live in gitignored `secrets_*.h`
  files with committed `.h.example` templates.
- Repository layout: one PlatformIO project per haunted object, each in its own top-level folder;
  the smoke alarm piece goes in `detectors/`.
