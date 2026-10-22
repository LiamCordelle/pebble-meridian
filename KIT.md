# Meridian kit integration — 1.0

Meridian depends on **pebble-watchface-kit 2.0.0**, revision
`42dd429169de83fcd45821b425ab8bd4bc759d81` (verified against GitHub main on
22 October 2026). This is the same dependency used by Mondrian and Underground.
The kit is a Git submodule at `shared-kit`, not copied production code.
No kit behaviour or protocol changes were needed for Meridian.

## Ownership and entry points

- `src/c/kit` → `shared-kit/src/c/kit`: health/history, weather scheduling and
  cache, settings, world clocks, minute ticks, touch/flick and eight-second detail shell.
- `src/pkjs/kit` → `shared-kit/src/pkjs/kit`: serialized phone transport,
  Open-Meteo encoding, Clay schema and settings persistence.
- `src/c/lab.h` and `src/c/lab_common.c` link to the shared LabData/LabDesign contract.
- Meridian owns `design.c`, scenario data, the `design_meridian2` registration,
  launcher/store assets, metadata and its `src/pkjs/app-config.js`.
- `meridian-kit-settings-v1` is the phone namespace. The shared setting keys,
  including the hidden LARGE_LABELS=0 compatibility field, retain kit numbering.

Shared behavioural changes must be made and tested in the kit repository,
then committed/shared there before updating this dependency. Do not replace
these links with local implementations. See `shared-kit/AGENTS.md` and
`shared-kit/docs/copied-kit-history.md` for protocol, health and emulator history.
This consumer-owned KIT.md records integration without making the dependency dirty.

## Drawing contract and gotchas

Temperatures in LabData remain Celsius. Convert only displayed values to °F;
radial lengths use Celsius so changing units preserves the geometry. Always use
`rolling_count` and `rolling_hour` to locate upcoming observations, never wrap a
short forecast or index rolling arrays by clock hour. On the fixed 24-hour dial,
tomorrow's hours share positions with today's history: paint forecast first and
history above it. Rain can be covered by a longer history ray. A repeated DST
clock hour has one dial position; the first upcoming observation occupies it.

Use the kit's `next_sun_min` for tomorrow's sunrise, and `uv_max_x10` at night.
Valid weather does not imply sun times (polar data); retain placeholders.
The detail corner retains the prototype's alternate sun event.

Recorded HR history is independent of the current reading. Show its range even
when current HR is missing, and append * for partial/loading history. A missing
range remains -- (or --*). Current-hour goal interpolation must use elapsed
minutes, not a completed hour's 60 minutes.

The renderer excludes lab_main.c; watch-only kit calls are guarded by LAB_HOST_RENDER.
Production defaults to scenario 3. MERIDIAN_* controls exist only in host previews.
There is no animation and no second tick. Default touch subscription matches
both existing consumers. Physical Time 2 touch power and draw timings still
require device measurements; QEMU is not a power meter. A ten-second subscription
variant is available through KIT_TOUCH_WINDOW_MS=10000.

## Build and validation

`git submodule update --init shared-kit`, `npm ci`, then `pebble build`.
`python3 tests/run.py` runs the shared suite and Meridian checks.
`python3 tools/render_scenes.py` renders 47 scenes and checks the original demo,
midnight rain, short caches, peak UV, pace direction and recorded HR availability.
`npm run release -- --pinned --render` uses the same tested release workflow
as the sibling faces. It requires a committed app HEAD for provenance and
records the exact source/kit revisions and PBW hash.

Final QEMU commands, log, screenshots and results are in `shots/emulator-*`.
Run `tools/emulator_check.py --clean-profile` with pebble-tool's Python to repeat.
It refuses an already-running QEMU, preserves/restores the existing SDK profile,
checks the active app UUID, exercises real settings/AppMessage/cache/service paths
with synthetic data, and stops QEMU even on failure.

## Changelog

1.0 (22 October 2026): integrated the shared 2.0.0 dependency. Meridian's rain
and temperature forecasts use the rolling horizon, night UV uses daily peak,
next sunrise uses tomorrow's event, and pace uses explicit up/down triangles.
Combined phone/battery warnings use separate rows. Wide detail ranges retain
at least Gothic 18 bold. Fixed partial-hour goal time; preserved the daytime
main face pixel for pixel. No shared kit source files changed.

## Final integration verification — 22 October 2026

Shared regression tests and Meridian's consumer/goal-time checks passed. The 47
render scenes and unchanged-pixel audit passed. Final QEMU pixel assertions
verified detail return/toggle, live units, persisted settings/weather on restart,
and battery/phone statuses. Recorded health and a correlated 48-hour snapshot
were accepted; no app faults appeared. Frames: 2–11 ms. The relaunch exit
reported zero remaining allocations. App footprint: 23,594 bytes; available
heap: 107,478 bytes. The kit dependency remains clean and unchanged.

Harness lessons: SDK install synchronizes the emulator clock, so timestamp
weather fixtures at delivery time. Restore both phone Clay storage and watch
settings before testing cache persistence. Device-state notifications redraw
asynchronously: wait for callbacks/painting before screenshots. Connection
Service debounces phone loss for 25 seconds; the harness waits 27 seconds.
Earlier tests produced unsigned wall-clock timing underflows around emulator
clock synchronization; preserve raw timings instead of treating them as CPU
measurements. The final run had no such underflows. Always verify the active
UUID and restore/stop the temporary profile even if a check fails.

## Public release 1.0.1

Liam approved the icon, source commit, new public GitHub repository and Pebble
publication. The 25×25 launcher icon uses the same MENU_ICON/menuIcon resource
registration as Mondrian and Underground; 80×80 and 144×144 store icons are
included. The release uses the unchanged shared kit 2.0.0 dependency.

Release 1.0.1 was published on 22 October 2026 at
https://apps.repebble.com/660f9fba42274715a277bb81, with public source at
https://github.com/LiamCordelle/pebble-meridian. The normal latest-kit release
workflow fetched main, passed tests and 47 renders, and built from a clean
source commit. The kit remained at 2.0.0 / 42dd429. Public metadata confirms
two Emery screenshots and launcher icon resource ID 1; the downloaded public
PBW matches every local ZIP entry and the complete SHA-256. See
`store/published.json` for the exact release/source/artifact receipt.

## 8,000-step default — 22 October 2026

Updated to shared kit 2.1.2 (`71a935d`). Fresh watch/phone settings and
face fallbacks use 8,000 steps. Valid saved goals are preserved. Protocol keys,
persistence layouts and explicit screenshot fixtures are unchanged. Shared and
consumer tests, all five render suites and fresh-profile QEMU checks cover the
new default and a saved 12,500-step goal across explicit stop/start.
