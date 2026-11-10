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
- Documentation pass for the conversation experiment: the Design Document now records its final
  state (timing model, difficulty, jump-ins, length drawn up front, the eight rounds of tuning by
  ear, and the 300-seed results), the three slot-frequency draws, and the limits of telling slots
  apart by pitch. Lexicon gains `jump-in` (proposed) and an updated `heat`.
- Changed: conversation long pauses doubled (hard end only: median 1.8 to 3.6 s, ceiling 2.4 to
  4.8 s, inner pause 577 to 1154 ms); silences over 1 s go from 13% to 25% in the audio. Confirmed
  by ear.
- Changed: slot chirp frequencies redrawn from 3000-3800 Hz (was 3000-3500 Hz) to make the default
  pair easier to tell apart: slots 1 and 2 are now 3140 and 3670 Hz. Known limit: only consecutive
  slot numbers are guaranteed 250 Hz apart; six non-consecutive pairs are 30 Hz apart.
- Changed: conversation jump-ins land 0-35 ms after the other turn starts (was 10-150 ms) so
  talking over is audible. A try at shrinking every hot pause toward zero was reverted.
- Changed: conversation length is drawn first, from 60-240 s, independent of the pacing.
- Changed: conversation timing is now gap-only. A chirp is a turn's whole speech, so the
  "implied utterance" was removed (its heavy tail caused the very long and very short spacings in
  the cool opening) and the silence between chirps is modeled directly: log-normal, median 300 ms
  (trivial) to 1.8 s (hard) by difficulty, hard bounds of 200 ms to 2.4 s enforced by redrawing,
  no jump-ins until heat builds, and the lapse chance halved (0.25 to 0.12) to offset the doubled
  turn count. Cool opening is about 1.4 turns a second. `implied utterance` is retired from the
  lexicon.
- Changed: conversation pauses trimmed 25% at both ends (log scale) after listening: pause before
  a reply 87-825 ms (was 60-1200), inner pause of a two-chirp turn 182-577 ms (was 150-700), and
  the random spreads cut by 25%. Pooled 10th/median/90th percentile is now 88/247/574 ms.
- Changed: conversation experiment now models difficulty (how hard a turn is to answer, 0 to 1,
  with memory and occasional topic shifts). The pause before a reply and the pause inside a
  two-chirp turn follow it, so pauses vary widely and come in streaks (correlation of consecutive
  pauses 0.39; 10th to 90th percentile 66 to 728 ms). `difficulty` added to the lexicon.
- Changed: conversation experiment revised after listening and a check against turn-taking
  literature. Spacing is now an implied utterance (median 0.9 s) plus a skewed gap (median 150
  ms); same-speaker pauses run 140 ms longer; heat drives talking over and no longer slows the
  opening; two-chirp turns are rare (about 1 in 10) and heat-weighted; lapses come only in the
  wind-down. The experiment now also prints statistics over 300 seeds. `implied utterance` and
  `lapse` proposed for the lexicon.
- Design Document reorganized into repository-wide decisions and a smoke-alarm-piece section,
  with the suites as subheads: how they fit together (equal suites, succession rules, durations,
  recipe/generator/player split, typed recipes first and JSON5 later), battery-ambient (with the
  random warmup offset), conversation (moved), and ensemble (not yet designed). Lexicon updated
  (`suite`, `recipe`, `warmup offset`; `score`, `succession rules`, and `SuiteScheduler` proposed)
  and open questions collected.
- Conversation experiment (`detectors/experiments/conversation/`), step 1 of the conversation
  suite: two slots, heat following a bell curve over the conversation, heat-driven turn lengths and
  gaps, per-slot talkativeness and wordiness drawn from a seed, rendered to a WAV with each slot's
  real voice. Verified by checking the WAV against the generated score.
- Design decision: experiments may hard-code values; the no-hard-coded-values rule applies to the
  piece and shared code.
- Design decision: the conversation suite is a generated, café-style exchange driven by heat (a
  bell curve over the conversation) and per-slot talkativeness and wordiness; human turn-taking
  research recorded. `cast`, `turn`, `heat`, `talkativeness`, `wordiness`, and `temperament`
  added to the lexicon.
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
