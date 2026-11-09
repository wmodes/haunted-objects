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

**Voice versus timing.** A slot's *voice* is its chirp length and chirp frequency, and it is the
same in every suite. Only *timing* (when chirps happen, and in what pattern) changes from suite to
suite. The profile's chirp interval is therefore the battery-ambient suite's timing, not part of
the voice. The conversation and ensemble suites draw their own timing from their recipes and reuse
each slot's voice unchanged.

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

Chirp frequency is drawn from 3000-3500 Hz, based on patent test results and a product manual
(see the research notes below); tune each horn by ear once the horns are wired. Not sourced, so
assumed: chirp length for the Kidde and BRK/First Alert models. A 10 ms burst into a resonant piezo housing probably sounds longer than
10 ms because the housing rings (unverified inference); judge by ear on the real horns.

#### Redrawing the profile table

If the table needs drawing again (more slots, a different spread, new research), draw it this way
so the result stays comparable:

1. **Slots and models.** Give each model an equal share of the slots (the current 12 slots are the
   six models below, two slots each), shuffled randomly across the slots.
2. **Draw uniformly within each model's range**, then round: chirp length and chirp interval to
   0.1, chirp frequency to the nearest 10 Hz.
3. **Check the result.** Slots count up from 1, and each interval is longer than its chirp. The
   header's compile-time check enforces this.

Ranges used for the current draw. Ranges marked "assumed" are not from a source:

| Model | Chirp length range | Chirp interval range | Basis |
|---|---|---|---|
| MC145010-class | 9.5-11.5 ms | 38.9-47.1 s | datasheet limits |
| MC146012?-class | 7.2-8.8 ms | 28.8-35.2 s | datasheet limits |
| A5348-class | 8.0-12.0 ms | 32.0-48.0 s | datasheet limits |
| A5366-class | 9.0-11.0 ms | 41.0-45.0 s | datasheet gives about 10 ms and 43 s; spread assumed |
| Kidde-style | 8.0-12.0 ms (assumed) | 30.0-40.0 s | interval from Kidde; length from the chip datasheets |
| BRK/First-Alert-style | 8.0-12.0 ms (assumed) | 55.0-60.0 s | interval from BRK and First Alert, capped at 60 s by NFPA 72; length from the chip datasheets |

Chirp frequency for every model: 3000-3500 Hz, rounded to the nearest 10 Hz (see the research
notes). The first draw used 2850-3250 Hz, an unsourced guess that sounded too low, so only the
frequencies were redrawn and the lengths, intervals, and models were kept.

The lengths and intervals were drawn with Python's `random.Random(20261109)` and the frequencies
with `random.Random(20261111)`. The scripts were throwaways and were not kept, so the seeds alone
cannot reproduce the table. These parameters can.

#### Research notes and caveats

- **How reliable these numbers are.** They come from web-search summaries of datasheets, patents,
  and manufacturer pages. The datasheets themselves were not read directly, and several sources
  are secondary (consumer guides). Before relying on a figure for a decision that matters, check
  it against the datasheet or the manufacturer's manual.
- **Low-battery chirp interval.** Chip-driven alarms land at about 29-48 s. Manufacturer pages
  give 30-40 s, 30-60 s, or about 60 s, and Kidde's own pages disagree with each other. NFPA 72
  requires the signal at least once a minute for 7 days, so about 60 s is the ceiling. Overall
  range: about 30-60 s.
- **Chirp length.** About 8-12 ms (extremes 7.2-12 ms) from four chip datasheets and patent
  descriptions of the horn-drive burst; patents describe 10-12 ms. No measured waveform of a
  real alarm was found.
- **End-of-life is a different signal.** A sealed-battery Kidde alarm chirps about every 30 s to
  say it should be replaced, and a fresh battery does not clear it. This is the same rhythm as
  some low-battery chirps. A double-chirp end-of-life pattern appeared in one manual excerpt but is
  unconfirmed.
- **Cold triggers low-battery chirps.** Kidde says cold air can start the low-battery chirp,
  which explains chirps that begin overnight. A possible source of realistic variation, not used.
- **Chirp frequency.** Typical smoke-alarm horns sit near 3100-3200 Hz. A patent describes
  standard detectors as producing a 3100-3200 Hz pure tone. A patent citing tests of about 18
  detectors reports post-1996 units peaking near 3200 Hz, plus or minus about 10%. A UL 217 alarm
  manual states 3.2 +/- 0.3 kHz ([X-Sense XS03](https://manuals.online/x-sense/smoke-alarm/xs03/user-manual/15mgz)),
  and a [UK government page](https://www.gov.uk/government/news/hard-wired-for-sound-love-to-hate-your-smoke-alarm--2)
  says about 3000 Hz. The patents were found by search and I could not tell which said what
  (candidates: [US 12112610](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/12112610),
  [US 7015807](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/7015807)). One
  [First Alert manual](https://manualsfile.com/product/p9fshpd34c.html) says its horn sweeps
  2200-3400 Hz (a specification, not a measurement). These are patents and a manual, not detector
  datasheets. The first draw used 2850-3250 Hz, an unsourced guess whose low end (2910-2980 Hz in
  four slots) sounded unlikely; the table now uses 3000-3500 Hz.
- **Could not find:** a chirp frequency in any detector datasheet; a measured recording or
  spectrogram of a real chirp; the UL 217 text itself; chirp length for Kidde, BRK, or First Alert
  models.
- **How to firm this up.** Read the cited datasheets directly; measure a real working alarm if one
  becomes available (the salvaged units no longer work); and judge the final chirp length and
  pitch by ear on the real horns, since each horn's housing resonates and rings.

### Drive waveform (smoke alarm piece)

What waveform commercial alarm horns use, from the research. The sources are datasheets,
application notes, and patents read through search summaries; no measured spectrum of a real
smoke alarm was found.

- **Electrical drive is a rectangular (square) wave** from a push-pull output stage. Microchip's
  horn-driver parts use a push-pull stage
  ([RE46C100 datasheet](https://datasheet.chipsmall.com/RE46C100S8F/539556.pdf)). A 1983
  [patent](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/4496914) describes
  alternating which supply side each horn terminal connects to, to double the voltage swing and
  reach the roughly 85 dB a smoke alarm needs. That is the same idea as the TB6612FNG push-pull
  drive in this piece. Some parts allow a non-50% duty cycle.
- **On a three-terminal piezo the horn sets its own pitch.** The feedback electrode returns to the
  driver, which flips its output when the feedback crosses half the supply, so the horn's
  resonance sets the frequency, not the chip
  ([TI TPS8802 datasheet](https://www.ti.com/document-viewer/TPS8802/datasheet/GUID-CD79A4AA-B021-4E9B-9284-30B5571CA41C),
  RE46C100). A related NXP [application note](https://www.nxp.com/docs/en/application-note/AN1690.pdf)
  gives example values for a 3.4 kHz three-lead horn.
- **A two-terminal drive, which is this piece (the feedback wire is unused), is driven externally.**
  TI's datasheet says a two-terminal horn can take a square wave at its resonant frequency, a PWM
  sine, or an arbitrary PWM shape.
- **The acoustic output is closer to a sine than the electrical drive is.** A bare piezo disc is
  weak, so it sits in a resonant cavity
  ([PZT application note](https://www.framboise314.fr/wp-content/uploads/2018/02/PZT_Application_note.pdf)).
  A vendor frequency scan shows a dominant peak at the fundamental with weaker harmonics, and the
  vendor says square drive gives more sound level than sine.
- **Horn pitch examples cluster around 2.8-3.4 kHz** (a 2.8 kHz scan, the 3.4 kHz application-note
  example, and [SparkFun's 3 kHz unit](https://sparkfun.com/products/13940)). That fits the
  3000-3500 Hz range now used in the profile table; the smoke-alarm-specific figures are in the
  research notes above.
- **Sawtooth: no evidence found.** Nothing I read describes a sawtooth drive. A sawtooth would
  need deliberately shaped (PWM-synthesized) drive; the H-bridge produces a square natively. The
  piezo and its cavity filter whatever they are given, so a real horn may sound buzzier than a
  clean square.
- **Not found:** the MC145010's horn-drive details, the Allegro A5348/A5358 horn drivers, and any
  measurement of a real smoke alarm's waveform or spectrum.

Decision: waveform is a setting, not hard-coded. The detector is expected to drive its horns with
a square wave (what the H-bridge produces natively and what the research documents). The chirp-table
experiment, which plays through computer speakers, defaults to sine, and also offers square and
sawtooth so they can be compared by ear against how the real alarms sounded. Open: which waveform
the real horns should get; judge by ear once the horns are wired.

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
| voice | A slot's chirp length and chirp frequency. The same in every suite. |
| timing | When a slot's chirps happen and in what pattern. Varies by suite; the profile's chirp interval is the battery-ambient suite's timing. |
| slot | The numbered position of a detector, counted from 1. A slot always behaves the same across power-ups. |
| detector profile | A slot's fixed chirp identity: chirp length, chirp frequency, chirp interval, and the `model` it imitates. |
| test | A formal check of a class through its public interface, including mutation runs. Lives in `test/`. |
| experiment | A throwaway probe of hardware or an idea. Lives in `experiments/`; nothing outside it depends on it. |
| Secrets | The immutable value object holding credentials (WiFi, OTA), built once from the gitignored `secrets_*.h` files. |
