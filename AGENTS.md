# Working on this watchface

Read `shared-kit/AGENTS.md` before editing health, weather, settings, phone
messaging or lifecycle code. Initialize missing dependencies with
`git submodule update --init shared-kit`.

## Decide where a change belongs

- Shared health/HR history, weather validation/cache/retries, settings parsing,
  world clocks, AppMessage transport, touch/lifecycle/timing, common drawing
  helpers and LabData/LabDesign contract changes belong in `shared-kit`.
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
