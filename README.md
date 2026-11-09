# Haunted Objects

Ordinary objects that behave as if something is wrong with them.

## Smoke Alarm Piece

Salvaged smoke alarms, reassembled to look stock, chirping like their batteries are dying. The
original alarm boards are gone; the horns are driven by a single ESP32 instead. The alarms are
labeled non-functional.

Most of the time each alarm chirps its own manufacturer-inspired low-battery rhythm. At
intervals the alarms break into a composed interlude, either talking to each other or building a
rhythmic score together, and then fall back to their battery chirps.

### Hardware

- ELEGOO ESP32 dev board (classic ESP-WROOM-32, 30-pin)
- TB6612FNG dual H-bridge driver boards, one per two horns, run at 9V
- 3-terminal piezo horns salvaged from the alarms, driven push-pull

### Source

Source lives in [detectors/](detectors/). Each haunted object gets its own folder and its own
PlatformIO project, so more pieces can join this repo later.

### Documentation

- [Design Document](documentation/Design%20Document.md): design decisions, hardware, open
  questions, and the project lexicon
- [Dev README](documentation/DEV-README.md): toolchain and testbenches
- [CHANGELOG](CHANGELOG.md)

## Status

Early setup. There is no firmware yet.
