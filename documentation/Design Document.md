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
    experiments/     throwaway probes, one PlatformIO project each
  <next object>/     future pieces
```

### Tests and experiments

Tests and experiments are different things and live in different places.

- **Tests** are formal: unit tests (PlatformIO's `test/` folder) and mutation runs. Each class
  gets its own test file and is tested through its public interface.
- **Experiments** are throwaway probes, such as the blink test that proved the toolchain and
  board. They live in `experiments/` inside the piece, never in the piece's `src/` or
  `include/`. Each is its own PlatformIO project.
- **One-way dependency.** An experiment may use the piece's code and the shared layer, but
  nothing outside `experiments/` may depend on an experiment. Deleting the whole folder has no
  lasting consequence.
- **Where a setting lives.** A setting that is generally useful to the piece goes in the piece's
  `Config`. A setting only one experiment needs lives in that experiment's own config header, so
  deleting the experiment leaves no dross in main config.

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

### Detector profiles (smoke alarm piece)

Each slot has a fixed chirp identity that holds across power-ups: whatever is plugged into slot 1
always acts like slot 1. A slot's **profile** gives its chirp length, chirp frequency, and chirp
interval, plus a `model` naming the commercial detector it imitates, which records where the
numbers came from. The table lives in `detectors/include/DetectorProfiles.h` (12 slots, which
leaves headroom for the real alarm count) and is validated at compile time. The values were drawn
once, at random within each model's range, then fixed; edit them freely.

What commercial low-battery chirps measure, from the research (chirp length is the horn-drive
burst from detector chip datasheets, not a measured waveform):

| Model | Chirp length | Chirp interval | Source |
|---|---|---|---|
| MC145010-class | 9.5-11.5 ms | 38.9-47.1 s | [NXP MC145010](https://www.nxp.com/products/MC145010) datasheet |
| MC146012?-class | 7.2-8.8 ms | 28.8-35.2 s | a second Motorola datasheet; part number unconfirmed |
| A5348-class | 8-12 ms | 32-48 s | [Allegro A5348](https://www.allegromicro.com/-/media/files/datasheets/a5348-datasheet.pdf) datasheet |
| A5366-class | about 10 ms | about 43 s | [Allegro/Sensitron A5366](https://html.alldatasheet.com/html-pdf/218675/SENSITRON/A5366/4404/9/A5366.html) datasheet; the spread around these is assumed |
| Kidde-style | assumed 8-12 ms | 30-40 s | [Kidde support](https://www.kidde.com/home-safety/en/us/support/help-center/browse-articles/articles/what_causes_intermittent_beeping_or_chirping_.aspx) |
| BRK/First-Alert-style | assumed 8-12 ms | 55-60 s | [BRK](https://www.brkelectronics.com/faqs/diy/why-do-smoke-alarms-chirp-intermittently-new-construction), [First Alert](https://www.firstalertstore.com/store/product-support/why-is-my-smoke-alarm-chirping-beep-patterns.htm) |

NFPA 72 requires the low-battery signal at least once a minute for 7 days
([excerpt](https://fsc.ri.gov/media/156/download)), which caps commercial intervals at about 60 s.
The overall interval range is about 30-60 s.

Not sourced, so assumed: chirp frequency (placed near 3 kHz, 2850-3250 Hz, where the horns
resonate; tune each by ear once the horns are wired), and chirp length for the Kidde and
BRK/First Alert models. A 10 ms burst into a resonant piezo housing probably sounds longer than
10 ms because the housing rings (unverified inference); judge by ear on the real horns.

### Tooling

- PlatformIO via the CLI, not the VS Code extension, with each piece as its own PlatformIO
  project inside this repo.
- The priority is that Claude Code can build, flash, and read serial output directly.
- OTA updates over WiFi later, since the board will end up hidden in an installation.
- Firmware can be emulated on the development Mac with the Wokwi CLI: a simulated classic ESP32,
  run headless, with assertions on its serial output. This lets logic be exercised without
  hardware. Emulation checks signals and logic, not sound, loudness, or piezo resonance.

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
| slot | The numbered position of a detector, counted from 1. A slot always behaves the same across power-ups. |
| detector profile | A slot's fixed chirp identity: chirp length, chirp frequency, chirp interval, and the `model` it imitates. |
| test | A formal check of a class through its public interface, including mutation runs. Lives in `test/`. |
| experiment | A throwaway probe of hardware or an idea. Lives in `experiments/`; nothing outside it depends on it. |
| Secrets | The immutable value object holding credentials (WiFi, OTA), built once from the gitignored `secrets_*.h` files. |
