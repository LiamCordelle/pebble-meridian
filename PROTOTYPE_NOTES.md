# 39 · Meridian v2

## Pitch

Meridian, tuned for a one-second glance at the wrist. It is still Solar's 24-hour sky around a
porthole that floods with your steps. The sun drags the day's steps behind it as rays and the
coming rain falls ahead of it, and the time inverts under the water. Every number is now big
enough to read at arm's length. The dial is painted pixel-exact, the rays are bolder and the
water is deeper and calmer.

## What changed from v1 (34)

| v1 | v2 | Why |
|---|---|---|
| Silkscreen 8 labels: `UV 3`, `RAIN 15:00`, `STEPS 68%`, `BPM`, date | Pixel icons and positions, Gothic 24 bold values | Every Silkscreen label failed the glance check |
| `SAT 17 OCT` in Silkscreen 8 | `SAT 17` in Gothic 24 bold, still above the time | Date is a must-have; the month does not fit at 24 px |
| Rain-peak corner (`60%`, `RAIN 15:00`) | Next sun event (`18:03` + horizon icon) | Brief asks for the next sun event; the streaks already show rain |
| `STEPS 68%` | `6,842` only | The water already shows goal %; each metric once |
| Pale `00AAFF` water, navy wet ink | Deep `0055FF` water, black wet ink | Calmer, deeper disc; black gains contrast over navy (4.8:1 vs 3.8:1) |
| Rays max 15 px; streaks 3 px wide, 1–13 px long | Rays max 21 px; streaks 5 px wide, 5–21 px long (a 20% hour: 7 px, was 3) | Bolder at real size |
| Ring R 81–89 (`fill_radial`), AA lines and circles | Ring R 85–91 painted per pixel, crisp capsules and discs | No anti-aliased edges (v1's render had 7 AA blend colours, v2 has none) |
| Charging = `CHARGING` label | Lavender `99%` + bolt glyph | Same footprint as the battery glyph |

Kept: the sky ring and twilight steps, the sun hand (white core by day), the rays, the porthole
flood with Tide's Stokes wave, the NZ bead, the moon at midnight, exceptions-only device status,
and the tap view's layer swap with the goal ETA.

## Metric → element (face)

| Metric | Element |
|---|---|
| Time | LECO 36 bold in the porthole, hand-placed 5x5 colon, ink centred on x = 100 |
| Date | `SAT 17`, Gothic 24 bold, above the time (floods with the time above ~78%) |
| Steps / goal | Porthole water level: floor at 0, brim at the goal. A tick under the time once met |
| Steps total | Bottom left: footprints (yellow = the rays) + `6,842` |
| Steps per hour | Pale-yellow 5 px rays behind the sun, length ∝ steps (scale = peak hour, floor 2,000), max 21 px |
| Rain per hour | Light-blue 5 px streaks ahead of the sun (≥ 10%), 5–21 px |
| Heart rate | Bottom right: `64` + pink heart |
| Temperature, condition | Top left: Solar's pixel weather glyph + `14°` |
| UV | Top left, second line: `UV` (lavender label) + `3` |
| Next sun event | Top right: `18:03` + peach horizon icon (peach = the day arc). Sunrise after dark |
| Time of day, twilight | Sun on the sky ring; day arc and three twilight steps |
| NZ time | White bead riding the ring |
| Moon phase | Phase disc at midnight, mid-band |
| Battery, charging, phone | Exceptions only, taking over the top-right corner: battery % with a glyph in Signal Deck colours (below 20%), lavender `99%` + bolt while charging, coral `NO PHONE` |

Missing data: no weather gives an all-night ring, no streaks, a dim cloud with `--`, `UV --`
and a dim horizon `--`. Zero steps leave the porthole dry. No heart rate shows `--`.

## Detail view (tap)

The same dial with the layers swapped, as v1:

- **Behind the sun: heart rate.** Elevated hours as pink 5 px rays; resting hours stay dots.
- **Ahead of the sun: temperature.** The rest of today as finer 3 px peach streaks. At 5 px
  they turned the right half into a peach sunburst.
- **Porthole (same water):** flag + `12:03` (goal ETA at today's pace, Summit's formula) or
  tick + `13:26` (goal met), the total in LECO 36 bold (LECO 32 bold from 10,000), and pace
  against your usual day: `+1,722`, `-340` or `ON PACE`.
- **Corners:** thermometer (peach = the streaks) + `17°/9°`; heart + `52-141` with the battery
  `76%` under it; the sun event the face doesn't show (sunrise by day, sunset by night); `22:09 NZ`.

## Readability

Checked with `tools/glance.py` every round (`shots/demo_glance.png`, `shots/detail_glance.png`).
v1's glance (`scratch/v1-demo_glance.png`) kept only the time and the four corner values; every
label was mush.

- **Enlarged:** corner values from Gothic 18 bold (11 px) to Gothic 24 bold (14 px digits). The
  date went from Silkscreen 8 (5 px caps) to Gothic 24 bold. Rays went from 15 to 21 px, and
  streaks from 3 to 5 px wide.
- **Replaced labels with icons:** footprints, heart, horizon and weather glyphs, all 2 px strokes,
  10–15 px tall, coloured as the dial elements they stand for. The only label left is `UV`, in
  Gothic 18 bold on the value's baseline.
- **Moved:** UV got its own line under the temperature. The rain peak left the face (the streaks
  show it). The battery is only on the face when it matters, and always in the tap view.
- **Dropped:** the step %, the month, and all Silkscreen.
- **Tried and rejected:** UV printed inside the sun (Gothic 18 bold in an r 9 disc) was a blob at
  glance. `1,722 AHEAD` in Gothic 18 bold under water was mush; `+1,722` in Gothic 24 bold reads.
- **Result:** in the demo glance, the time, date, temperature, UV, sunset, steps and heart rate
  all read. In the detail glance, every value reads. In the stress glance, the flooded
  `WED 30` / `20:48` read in black on the water.

## Ground: navy kept, paper explored

The brief asked to try warm paper by day and navy at night. The paper palette is still in
`design.c` behind `#define MERIDIAN_PAPER 1` (off by default, so it costs no bytes). Renders:
`scratch/paper-day.png`, `scratch/paper-day-detail.png`, and side by side in
`scratch/navy-paper-1x.png`.

- **Paper's win:** Tide's porthole: deep `0055AA` water with white digits (6.6:1) is the
  cleanest inversion in either version.
- **Navy's win:** the sky. The sun, rays and peach day arc are light, warm colours that glow on
  navy, but fall to 1.1–1.8:1 on cream (pale yellow all but vanishes). On paper they had to
  become coral and sky blue, and the dial lost the glow that made v1 Liam's favourite. Navy also
  never flips the whole face at sunset.

## Palette (navy)

| Role | Code | Corrected |
|---|---|---|
| Night sky (background) | `000055` | `#001e41` |
| Night track, moon shadow | `0000AA` | `#004387` |
| Twilight (astro / nautical / civil) | `5555AA` / `AA55AA` / `FF5555` | `#4f6790` / `#9a7099` / `#e6727c` |
| Day arc, temperature streaks, horizon and thermometer icons | `FFAA55` | `#f1ad93` |
| Sun, step rays, footprints | `FFFF55` | `#fff1b5` |
| Water | `0055FF` | `#007dce` |
| Wet ink (time, date, tick under water) | `000000` | `#000000` |
| Rain streaks, water crest line | `55AAFF` | `#69b5dd` |
| Porthole rim, `UV` label, clouds, charging | `AAAAFF` | `#a7bae2` |
| Hour dots, placeholders | `5555AA` | `#4f6790` |
| Heart icon and rays | `FF55AA` | `#e37fa7` |
| Time, values, sun core, NZ bead | `FFFFFF` | white |
| Moon | `FFFFAA` | `#fff6d3` |
| `NO PHONE` | `FF5555` | `#e6727c` |

## Fonts

- Time: system `LECO_36_BOLD_NUMBERS`. Detail total: LECO 36 bold, or `LECO_32_BOLD_NUMBERS`
  from 10,000.
- Values, date, detail porthole lines: system `GOTHIC_24_BOLD`.
- `UV` label, `NO PHONE`, detail battery: system `GOTHIC_18_BOLD`.
- No Silkscreen and no added resources; `package.json` is unchanged from the scaffold.

## Emery rendering lessons

- **A pixel-exact dial is cheap.** One frame-buffer pass paints the background, the ring
  (`atan2_lookup` → minute → sky colour) and the rim. Rays are capsules: every pixel whose centre
  is within 2.5 px of the segment, in 1/256 px fixed point. The render uses 14 colours, all from
  the palette. v1 had seven AA blends (`AAAA55`, `555555`, `AA5555`…).
- **Capsule widths come out odd** when the axis runs through pixel centres: a half-width of 2.5
  gives 5 px, and 1.5 gives 3 px.
- **Black beats navy as wet ink.** Under mid-blue water, `000000` gives 4.8:1 against `0055FF`,
  where the background navy gives 3.8:1. Both look like "dark" on the wrist.
- **Deeper water options:** `0000FF` lost the digits (3.8:1), and teal `00AAAA` went murky
  again. `5555FF` read as lilac next to the lavender rim. `0055FF` reads as water.
- **LECO 20 bold has caps, `,`, `°` and `%`** in this firmware: `SAT 17`, `UV 3` and `6,842` all
  render (BRIEF §3 says letters exist only in the AM/PM variant; I only checked size 20).
- **A corner's second line has about 45 px** before the R 91 ring (at y 35 the ring is at x 145),
  which is why the charging bolt replaces the battery glyph instead of sitting beside it.
- **`char rows[][13]` with 13-character strings** silently drops the NUL in C, and the rows run
  together across the screen. Size glyph arrays at length + 1.

## Ideas worth stealing

1. **Pixel-exact dial painter.** Sky ring by `atan2_lookup` per pixel and radial capsules by
   distance to the segment: crisp at every angle, no AA fringes, about 4k ring pixels a minute.
2. **Black wet ink.** When a flood recolours text over mid-tone water, pure black buys 25% more
   contrast than the background's dark tone.
3. **Icons as the legend.** Each corner icon wears the colour of the dial element it explains:
   footprints = the yellow rays, horizon = the peach day arc, heart = the pink detail rays.
4. **Status in the same footprint.** The charging bolt swaps in for the battery glyph, so the
   status never widens.

## Borrowed code (credits)

- **34 Meridian** (base): everything not listed as new above. Through v1, that includes:
  - **16 Solar:** geometry, sky schedule, past and future hour placement, moon, weather glyphs,
    `draw_time`, `draw_big_number`.
  - **08 Tide:** Stokes waterline, sea state, frame-buffer flood.
  - **21 Summit:** ETA formula and goal-met minute, unchanged.
  - **28 Underground:** exceptions-only status.
  - **24 GMT:** NZ read on a 24-hour scale.
- New in v2: the per-pixel ring and capsule painter, the crisp disc, the icon set (footprints,
  heart, horizon, thermometer, flag, bolt, small battery), the palette table and the paper
  switch.

## Known limitations and next steps

- Exceptions take over the sun-event corner. While charging or offline, the next sunrise or
  sunset only shows as the end of the day arc.
- After sunset the face shows today's sunrise as the next one; the data model has no tomorrow.
- `+1,722` in the tap view relies on context; `AHEAD` didn't fit at a glanceable size.
- The month is only implied (no corner for it).
- Around 01:00 with five-digit steps, the sun's halo can graze the steps value; the text draws
  on top.
- The paper ground is a build switch, not a setting.
- Checked with `render.py` and the variants in `scratch/` (12h, sunrise at night, battery-only
  exception, ON PACE, all seven weather kinds). Not checked on a real panel.
