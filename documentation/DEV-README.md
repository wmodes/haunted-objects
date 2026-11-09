# Dev README

For working on the firmware. Design decisions live in the [Design Document](Design%20Document.md).

## Toolchain

PlatformIO via the CLI (not the VS Code extension), so builds, flashing, and serial monitoring
can all be driven from a terminal. Target board: classic ESP32 (`esp32dev`).

Each haunted object is its own PlatformIO project in its own top-level folder; the smoke alarm
piece is `detectors/`. Run PlatformIO commands from inside that folder.

Install (macOS): `brew install platformio`. The project uses the Arduino framework on the
`esp32dev` board, with the platform version pinned in `platformio.ini`. The first build downloads
the toolchain.

`detectors/` was scaffolded with:

```
mkdir detectors
pio project init --project-dir detectors --board esp32dev --project-option "framework=arduino"
```

Build, flash, and monitor, from inside `detectors/` with the board plugged in over USB:

```
pio run                # build
pio run -t upload      # build and flash (finds the USB port automatically)
pio device monitor     # read serial output; Ctrl+C to quit
```

The main project has no firmware yet, so `pio run` in `detectors/` has nothing to build; until it
does, run these commands from an experiment folder (see Experiments below).

`monitor_speed` in `platformio.ini` must match `serialBaudRate` in `SharedConfig.h`; PlatformIO
cannot read the config header, so the value is stated in both places.

## Secrets

WiFi and OTA credentials live in gitignored files named `secrets_<layer>.h`. Never commit them.
Each has a committed `.h.example` template with `#####` placeholders. After cloning, copy each
template and fill in the real values:

```
cp shared/include/secrets_shared.h.example shared/include/secrets_shared.h
cp detectors/include/secrets_detectors.h.example detectors/include/secrets_detectors.h
```

`secrets_shared.h` holds credentials common to every haunted object; a piece's own
`secrets_<piece>.h` overwrites any of them for that piece only. The single ignore rule
`secrets_*.h` covers all of them. (Credentials are also compiled into the firmware image, so
protect the device and its binaries too.)

## Experiments

Throwaway probes live in `detectors/experiments/`, one PlatformIO project per folder, and never
in `src/` or `include/`. An experiment may use the piece's `Config`/`Secrets` and the shared
layer, but nothing may depend on an experiment, so the whole folder can be deleted safely.
Settings only an experiment needs stay in that experiment's own config header.

| Experiment | What it does | Run |
|---|---|---|
| `blink` | Blinks the on-board LED (GPIO2) and prints a heartbeat over serial; proves build, flash, config, and serial. | `pio run -d detectors/experiments/blink -t upload`, then `pio device monitor -d detectors/experiments/blink` |

## Testbenches

None yet. Each class gets its own test file that goes through its public interface. Every
testbench is listed here with how to run it.
