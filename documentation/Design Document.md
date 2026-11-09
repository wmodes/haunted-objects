# Design Document

All design decisions for the haunted objects are compiled here. Each object is a "piece" with
its own folder; sections specific to one piece say so in their headings.

## Concept (smoke alarm piece)

Salvaged smoke alarms, reassembled to look stock, chirping like their batteries are dying. The
horns are disconnected from the original alarm boards and driven by a microcontroller instead.

## Decisions

### Repository layout

One repository holds every haunted object. Each piece lives in its own top-level folder and is
its own PlatformIO project (`platformio.ini`, `src/`, `include/`, `test/`), so builds and
flashing stay independent. The smoke alarm piece lives in `detectors/`; future pieces get
sibling folders of the same shape. Documentation stays repo-wide in `documentation/`: one Design
Document and one lexicon, with each piece as its own section.

```
haunted-objects/
  README.md, CHANGELOG.md, .gitignore
  documentation/     Design Document, DEV-README
  detectors/         smoke alarm piece
  <next object>/     future pieces
```

### Configuration and secrets

Every value that can change what the firmware does lives in config, never hard-coded. Config and
secrets are both layered: shared defaults first, then each piece's own values, so the local value
overwrites the global one.

- **Typed values, not macros.** The shared layer is a struct whose fields carry default values;
  a piece builds on it and sets the fields it needs to change. A misspelled field name is a
  compile error (a misspelled macro would silently do nothing), and the constructor validates
  and clamps values.
- **One source of truth.** `Config` and `Secrets` are immutable value objects, built once at
  startup and handed to every class that needs them. Nothing else reads the raw values.
- **Where things live.** Shared layer in `shared/include/`, each piece's layer in its own
  `include/`. Real credentials go in gitignored files named `secrets_<layer>.h`
  (`secrets_shared.h`, `secrets_detectors.h`); each has a committed `.h.example` template with
  `#####` placeholders. Every real secrets file starts with `secrets_` and ends in `.h`, so one
  ignore rule (`secrets_*.h`) covers all of them and never matches a template. The underscore
  matters: macOS ignores filename case, so a bare `secrets*` would also match the `Secrets.h`
  class header.
- **Resolved at compile time.** Changing a value means rebuilding and reflashing (over OTA once
  that exists).

### Behavior (smoke alarm piece): ambient with interludes

The piece is a hybrid of emergent and composed behavior.

- **Ambient** is the default state. Each alarm plays its own battery pattern, inspired by the
  rhythm of a real manufacturer's low-battery chirp. Ambient lasts for a configurable period
  between interludes (`ambient_period_minutes`).
- An **interlude** breaks into ambient, then hands back to it. There are two kinds:
  - **Conversation**: the alarms appear to talk to each other, tentative at first, then a full
    and increasingly frenetic exchange (the *Close Encounters of the Third Kind* model).
  - **Ensemble**: the alarms appear to work together on a rhythmic or musical score that starts
    simple and grows more complicated. None of the horns are tuned.
- After an interlude, the alarms fall back to their battery patterns.

### Hardware (smoke alarm piece)

- **Microcontroller:** a single ELEGOO ESP32 dev board, classic ESP-WROOM-32 (not S2/S3/C3/C6),
  CP2102 USB-serial, USB-C, 30-pin. Chosen for its many independent PWM channels. One board
  drives all the horns directly, so interludes need no inter-board synchronization.
- **Horn drivers:** TB6612FNG dual H-bridge boards, one board per two horns, independently
  controlled. Run at 9V.
- **Horns:** 3-terminal piezos salvaged from the alarms, kept in their resonant housings. Two
  drive wires per horn; the feedback wire is unused. Polarity doesn't matter. Each horn has its
  own resonant sweet spot somewhere around 3kHz.
- **Drive:** push-pull (opposite-phase signals into the H-bridge), which makes the horns loud but
  costs two ESP32 pins per horn.
- **Connections:** quick-disconnect 2-pin pigtails per alarm.
- **Power:** separate supplies for the ESP32 and the 9V horn rail, grounds tied together.

### Tooling

- PlatformIO via the CLI, not the VS Code extension, with each piece as its own PlatformIO
  project inside this repo.
- The priority is that Claude Code can build, flash, and read serial output directly.
- OTA updates over WiFi later, since the board will end up hidden in an installation.

## Open questions (smoke alarm piece)

- How many alarms (this sets the pin budget: two ESP32 pins per horn)
- How loud it needs to be in the space

## Appendix: Lexicon

Code, docs, and conversation use these words. A new term is proposed and approved before it is used.

| Term | Meaning |
|---|---|
| chirp | One burst from one horn. |
| battery pattern | An alarm's own manufacturer-inspired chirp rhythm, running independently. |
| ambient | The default state: every alarm plays its battery pattern independently. |
| interlude | A composed episode that breaks into ambient and then hands back to it. |
| conversation | An interlude in which the alarms answer each other, tentative at first, then frenetic. |
| ensemble | An interlude in which the alarms build a rhythmic score together, simple at first, then more complex. |
| InterludeScheduler | The class that decides when an interlude happens and which kind. |
| Config | The immutable value object holding every tunable setting, built once and handed to the classes that need it. |
| Secrets | The immutable value object holding credentials (WiFi, OTA), built once from the gitignored `secrets_*.h` files. |
