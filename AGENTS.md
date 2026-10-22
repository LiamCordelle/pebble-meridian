# Working on this watchface

Read `shared-kit/AGENTS.md` before editing health, weather, settings, phone
messaging or lifecycle code. Initialize missing dependencies with
`git submodule update --init shared-kit`.

## Decide where a change belongs

- Shared health/HR history, weather validation/cache/retries, settings parsing,
  world clocks, AppMessage transport, touch/lifecycle/timing, shared calculations
  and LabData/LabDesign contract changes belong in `shared-kit`.
- This face's layout, artwork, wording, scenarios, assets, exposed settings,
  UUID and app version belong in this repository. Its phone configuration is
  `src/pkjs/app-config.js`; retain its existing storageKey.
- `src/c/kit`, `src/pkjs/kit`, `src/c/lab.h` and `src/c/lab_common.c` are links
  into the kit. Editing through them edits the kit submodule, not a local copy.
  Do not replace the links or create a parallel implementation here. Prefer
  editing through `shared-kit/` so the repository ownership is explicit.
- When a requested improvement touches both, make coordinated kit and face
  changes. Add a kit configuration option/interface if the faces need different
  behaviour. Do not fork shared code or copy it into another watchface.

## Reuse before adding face-local logic

For every implementation change, check whether another face could use the
behaviour. General calculations and services belong in `shared-kit`, even for
their first consumer. Moon phase/illumination calculations belong in the kit;
moon artwork, placement and styling belong here. Existing local implementations
are not a precedent: when touching reusable local logic, extract it into the kit
and use the shared API. Keep unrelated refactors out of scope. Consumers retain
face-specific settings semantics/persistence and presentation; add generic kit
interfaces where needed rather than embedding consumer names in shared code.

## Check kit upgrades at the start of a watchface update

1. Record the current kit SHA and worktree state. Fetch `shared-kit`'s
   `origin/main` and inspect changes since the current revision. Use the latest
   shared, tested `main` in the new watchface version. For a clean dependency,
   `npm run kit:update` performs the guarded update without building. Re-read the
   upgraded kit's AGENTS.md/release notes and handle API or behaviour changes.
2. Preserve dirty or unpublished kit work. Fetch/check for updates, but do not
   reset/stash/drop edits or abandon commits to force an upgrade. Integrate the
   work through the shared kit workflow. Report network/integration blockers and
   current/latest known SHAs; do not silently claim a stale kit is up to date.
   Continue independent work while an upgrade is blocked.
3. Run shared and consumer tests, render coverage and runtime checks appropriate
   to the change with the selected kit. Finish with `npm run release -- --render`.
   If release advances the kit again, repeat affected validation on that revision.
   Record the updated gitlink and kit version/SHA with the face release and
   provenance; report the old/new dependency revisions. Use `--pinned` only for
   an explicitly requested historical/pinned build.

## Validate and share a kit change

1. Add regression coverage in `shared-kit/tests`; keep consumer-specific checks
   in `tests/consumer.test.js` and rendering in `tools/render_scenes.py`.
2. Run `python3 tests/run.py`, then from the kit run
   `python3 tools/check_consumers.py --render /path/to/pebble-mondrian /path/to/pebble-underground`.
   The latter tests the edited kit against both consumers in isolated snapshots.
3. For runtime changes, install/test the affected PBWs in QEMU; stop it afterward.
4. Commit/share the tested kit revision in the kit repository first. Then record
   the updated `shared-kit` gitlink in this face's release commit. A kit commit
   inside a consumer is a commit to the shared repository, not to this face.
   Coordinate compatibility changes across both faces before sharing on main.

## Build and release

`pebble build` uses the checked-out kit and supports local kit development.
`npm run release` automatically fetches the latest kit main, refuses unfinished
or unpublished kit changes, tests, clean-builds live data and records provenance.
`npm run release -- --pinned` uses the current kit without fetching; restore the
recorded gitlink with `git submodule update --init` first for historical builds.
Use `npm run release -- --render` to include render coverage. These commands
build locally; Pebble store publication still requires the user's instruction.
Do not change Signal Deck as part of kit work; it has not been migrated.

Commit author: Liam Cordelle <liamcordelle1234@gmail.com>.

## Meridian review workflow

Iterate visually with `python3 tools/render_scenes.py`; use QEMU only for final
runtime checks. Preserve Watchface Lab 39 Meridian v2 artwork outside the
production changes documented in KIT.md. Keep consumer integration notes in
root KIT.md; leave the dependency clean unless shared behaviour needs a fix.
Do not commit design-facing changes until Liam signs off. Ask before creating
branches, remotes or PRs. Never publish without an explicit request.

## Shared behaviour and runtime checks

Use shared-kit `kit_calc.h` for moon phase, temperature/event-time formatting,
forecast searches, daylight state and activity estimates. `kit_data.c` owns
live-data assembly, moon illumination and world-time projection. Keep fixture
values/preview overrides here, but call the shared calculations. Unknown sun
times are distinct from night; goal times are estimates and goal-day counts
are not streaks. New/default step goals are 8,000; preserve valid saved goals.

The kit focuses on data, behaviour and test support. Keep artwork, layout,
fonts, drawing and face-specific wording here; existing shared drawing helpers
remain compatibility utilities, not a requirement to grow a drawing framework.

Run `tools/emulator_smoke.py` with pebble-tool's Python for the shared fresh
default, saved-goal, explicit restart, synthetic weather/cache and detail checks.
It uses `shared-kit/tools/emulator.py`, restores the SDK profile and stops only
its owned session. Keep face-specific visual assertions in local checkers.
Use the shared session/transport/fixture/log APIs for new emulator scenarios;
do not copy their implementations or hardcode a kit version in log assertions.
