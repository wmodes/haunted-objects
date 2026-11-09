# Dev README

For working on the firmware. Design decisions live in the [Design Document](Design%20Document.md).

## Toolchain

PlatformIO via the CLI (not the VS Code extension), so builds, flashing, and serial monitoring
can all be driven from a terminal. Target board: classic ESP32 (`esp32dev`).

Each haunted object is its own PlatformIO project in its own top-level folder; the smoke alarm
piece is `detectors/`. Run PlatformIO commands from inside that folder.

Not set up yet. This section will hold the install steps and the build, flash, and monitor
commands once the project is scaffolded.

## Secrets

WiFi and OTA credentials go in `secrets.h`, which is gitignored. Never commit it. Create a secrets-sample.h for the repo with credentials #############ed out.

## Testbenches

None yet. Each class gets its own test file that goes through its public interface. Every
testbench is listed here with how to run it.
