#include "lab.h"

// 39 · Meridian v2 (BRIEF.md section 14.39), from 34 · Meridian.
//
// Solar's 24-hour sky around a porthole that floods with the day's steps. The
// sun rides the sky ring and drags each past hour's steps behind it as rays;
// the coming hours' rain falls ahead of it as streaks. The round window in the
// middle fills with water as you walk (empty at 0, full at the goal) and the
// time inverts where the water covers it.
//
// v2 is tuned for a glance at the wrist: the dial is painted pixel-exact (no
// anti-aliased edges), the rays are longer and the streaks as bold as them,
// the water is deeper, and the corners carry Gothic 24 values with pixel icons
// instead of Silkscreen labels.

// Geometry -------------------------------------------------------------------
#define MX 100
#define MY 114
#define R_OUT 91             // sky ring: pixel centres from R_IN - 0.5 to R_OUT + 0.5
#define R_IN 85
#define R_SUN 88
#define RW 52                // porthole: water inside RW + 0.5
#define RIM 2
#define STICK_R0 80          // outer cap centre of every ray and streak
#define RAY_MAX 21           // a peak hour's ray stops 2 px short of the rim
#define BOLD_HW 640          // stick half-widths in 1/256 px: 5 px sticks
#define FINE_HW 384          // 3 px (the detail's temperature streaks)
#define SUN_R 7
#define MOON_R 70
#define TWILIGHT 36

// Palette (Emery 64-colour), as GColor8 bytes ----------------------------------
#define C8(h) ((uint8_t)(0xc0 | (((h) >> 18) & 0x30) | (((h) >> 12) & 0x0c) | (((h) >> 6) & 0x03)))

enum { BG, NIGHT, ASTRO, NAUT, CIVIL, DAY, SUN, INK, SOFT, DOT, RAIN, MOON, WATER, CREST, WET,
       HEART, ALERT, CORE, NCOL };

// Night: navy sky, white ink, deep-blue water with black wet ink.
static const uint8_t PAL_NIGHT[NCOL] = {
  C8(0x000055), C8(0x0000aa), C8(0x5555aa), C8(0xaa55aa), C8(0xff5555), C8(0xffaa55),
  C8(0xffff55), C8(0xffffff), C8(0xaaaaff), C8(0x5555aa), C8(0x55aaff), C8(0xffffaa),
  C8(0x0055ff), C8(0x55aaff), C8(0x000000), C8(0xff55aa), C8(0xff5555), C8(0xffffff),
};

static const uint8_t *PAL = PAL_NIGHT;

static GColor col(int i) {
  GColor c = {.argb = PAL[i]};
  return c;
}

static GFont s_font_time;
static GFont s_font_mid;
static GFont s_font_big;
static GFont s_font_small;
static int16_t s_wl[2 * RW + 1];   // waterline per porthole column

static void load(void) {
  s_font_time = fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS);
  s_font_mid = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
  s_font_big = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_font_small = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
}

// Geometry helpers (Solar) ------------------------------------------------------

// Midnight at the bottom, noon at the top.
static int32_t minute_angle(int minute) {
  return TRIG_MAX_ANGLE / 2 + (int32_t)minute * TRIG_MAX_ANGLE / 1440;
}

static GPoint polar(int r, int32_t angle) {
  int32_t sx = sin_lookup(angle) * r;
  int32_t cy = cos_lookup(angle) * r;
  int32_t half = TRIG_MAX_RATIO / 2;
  return GPoint(MX + (sx + (sx >= 0 ? half : -half)) / TRIG_MAX_RATIO,
                MY - (cy + (cy >= 0 ? half : -half)) / TRIG_MAX_RATIO);
}

static int isqrt(int v) {
  int r = 0;
  while ((r + 1) * (r + 1) <= v) {
    r++;
  }
  return r;
}

static int round_div(int32_t num, int32_t den) {
  return num >= 0 ? (int)((num + den / 2) / den) : -(int)((-num + den / 2) / den);
}

// A crisp disc: every pixel whose centre lies within r + 0.5.
static void disc(GContext *ctx, GPoint c, int r, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  for (int dy = -r; dy <= r; dy++) {
    int w = isqrt(r * r + r - dy * dy);
    graphics_fill_rect(ctx, GRect(c.x - w, c.y + dy, 2 * w + 1, 1), 0, GCornerNone);
  }
}

// The moon sits at midnight, so hours 23 and 0 never carry a stick.
static bool moon_slot(int h) {
  return h == 23 || h == 0;
}

// Minute of the day a past (or the current) hour is drawn at: the middle of
// the part of the hour that has passed, so it always stays behind the sun.
static int past_mid(const LabData *d, int h) {
  return h < d->now.tm_hour ? h * 60 + 30 : h * 60 + d->now.tm_min / 2;
}

// Find the first available future observation for a clock-hour slot. The kit
// supplies local labels so repeated/skipped DST hours are never indexed as UTC.
static int forecast_index(const LabData *d, int h) {
  for (int i = 0; i < d->rolling_count && i < 24; ++i) {
    if (d->rolling_hour[i] == h) return i;
  }
  return -1;
}

static int forecast_mid(const LabData *d, int h, int i) {
  return i == 0 ? h * 60 + (d->now.tm_min + 60) / 2 : h * 60 + 30;
}

static int display_temp(const LabData *d, int c) {
  return d->temp_fahrenheit ? (c * 9 + (c >= 0 ? 2 : -2)) / 5 + 32 : c;
}

// The dial, painted pixel by pixel ---------------------------------------------

static bool sun_known(const LabData *d) {
  return d->weather_ok && d->sunrise_min >= 0 && d->sunset_min > d->sunrise_min;
}

static bool sun_is_up(const LabData *d) {
  return sun_known(d) && d->minute_of_day >= d->sunrise_min && d->minute_of_day < d->sunset_min;
}

// Sky colour at a minute of the day: day, three twilight steps, night.
static uint8_t sky_at(const LabData *d, int m) {
  if (!sun_known(d)) {
    return PAL[NIGHT];
  }
  if (m >= d->sunrise_min && m < d->sunset_min) {
    return PAL[DAY];
  }
  int before = (d->sunrise_min - m + 1440) % 1440;
  int after = (m - d->sunset_min + 1440) % 1440 + 1;
  int t = before < after ? before : after;
  return PAL[t <= TWILIGHT ? CIVIL : t <= 2 * TWILIGHT ? NAUT : t <= 3 * TWILIGHT ? ASTRO : NIGHT];
}

// Background, sky ring and porthole rim in one pass: no edge is anti-aliased.
static void paint_ground(GBitmap *fb, GRect bounds, const LabData *d) {
  const int32_t ring_in = R_IN * R_IN - R_IN, ring_out = R_OUT * R_OUT + R_OUT;
  const int32_t rim_in = RW * RW + RW, rim_out = (RW + RIM) * (RW + RIM) + RW + RIM;
  for (int y = bounds.origin.y; y < bounds.origin.y + bounds.size.h; y++) {
    GBitmapDataRowInfo row = gbitmap_get_data_row_info(fb, y);
    int dy = y - MY;
    for (int x = row.min_x; x <= row.max_x; x++) {
      int dx = x - MX;
      int32_t d2 = dx * dx + dy * dy;
      uint8_t c = PAL[BG];
      if (d2 > rim_in && d2 <= rim_out) {
        c = PAL[SOFT];
      } else if (d2 >= ring_in && d2 <= ring_out) {
        int32_t a = atan2_lookup((int16_t)dx, (int16_t)-dy);   // clockwise from 12
        c = sky_at(d, (int)(((a + TRIG_MAX_ANGLE / 2) % TRIG_MAX_ANGLE) * 1440 / TRIG_MAX_ANGLE));
      }
      row.data[x] = c;
    }
  }
}

// A round-capped radial stick from STICK_R0 inwards, `len` px between cap
// centres: every pixel whose centre lies within `hw` of the segment.
static void paint_stick(GBitmap *fb, int32_t angle, int len, int32_t hw, uint8_t c) {
  int32_t s = sin_lookup(angle), k = cos_lookup(angle);
  int32_t r0 = STICK_R0 * 256, r1 = (STICK_R0 - len) * 256;
  GPoint a = polar(STICK_R0, angle), b = polar(STICK_R0 - len, angle);
  int x0 = (a.x < b.x ? a.x : b.x) - 4, x1 = (a.x > b.x ? a.x : b.x) + 4;
  int y0 = (a.y < b.y ? a.y : b.y) - 4, y1 = (a.y > b.y ? a.y : b.y) + 4;
  for (int y = y0; y <= y1; y++) {
    GBitmapDataRowInfo row = gbitmap_get_data_row_info(fb, y);
    int32_t py = y - MY;
    for (int x = x0; x <= x1; x++) {
      int32_t px = x - MX;
      int32_t along = (px * s - py * k) / 256;
      int32_t perp = (px * k + py * s) / 256;
      int32_t e = along > r0 ? along - r0 : (along < r1 ? r1 - along : 0);
      if (perp * perp + e * e <= hw * hw) {
        row.data[x] = c;
      }
    }
  }
}

static void paint_dot(GBitmap *fb, int32_t angle) {
  GPoint p = polar(STICK_R0, angle);
  for (int y = p.y - 1; y <= p.y; y++) {
    GBitmapDataRowInfo row = gbitmap_get_data_row_info(fb, y);
    row.data[p.x - 1] = PAL[DOT];
    row.data[p.x] = PAL[DOT];
  }
}

static int step_scale(const LabData *d) {
  int peak = 0;
  for (int h = 0; h < 24; h++) {
    peak = d->steps_by_hour[h] > peak ? d->steps_by_hour[h] : peak;
  }
  return peak > 2000 ? peak : 2000;
}

// Behind the sun: each hour's steps as a ray. Ahead of it: each hour's chance
// of rain as a streak. Hours with nothing to show keep a dot.
static void paint_face_sticks(GBitmap *fb, const LabData *d) {
  int now_h = d->now.tm_hour;
  int scale = step_scale(d);
  for (int h = 0; h < 24; h++) {
    bool open = !moon_slot(h);
    int steps = (open && d->steps_ok && h <= now_h) ? d->steps_by_hour[h] : 0;
    int i = forecast_index(d, h);
    int rain = (open && d->weather_ok && i >= 0) ? d->rolling_rain[i] : 0;
    // Paint forecasts first: today's history stays legible when tomorrow's
    // hours wrap onto the same 24-hour dial positions.
    if (rain >= 10) {
      paint_stick(fb, minute_angle(forecast_mid(d, h, i)),
                  3 + (rain * (RAY_MAX - 3) + 50) / 100, BOLD_HW, PAL[RAIN]);
    }
    if (steps > 0) {
      paint_stick(fb, minute_angle(past_mid(d, h)), (steps * RAY_MAX + scale / 2) / scale, BOLD_HW,
                  PAL[SUN]);
    }
    if (steps <= 0 && rain < 10) {
      paint_dot(fb, minute_angle(h * 60 + 30));
    }
  }
}

// Detail sky: heart rate behind the sun (only elevated hours become rays),
// the rest of today's temperature ahead of it.
static void paint_detail_sticks(GBitmap *fb, const LabData *d) {
  int now_h = d->now.tm_hour;
  int lo = 0, hi = 0;
  for (int h = 0; h <= now_h; h++) {
    int bpm = d->hr_by_hour[h];
    if (bpm > 0) {
      lo = (lo == 0 || bpm < lo) ? bpm : lo;
      hi = bpm > hi ? bpm : hi;
    }
  }
  int span = hi - lo < 40 ? 40 : hi - lo;
  int tlo = 0, thi = 0;
  for (int i = 0; i < d->rolling_count && i < 24; i++) {
    if (i == 0 || d->rolling_temp[i] < tlo) tlo = d->rolling_temp[i];
    if (i == 0 || d->rolling_temp[i] > thi) thi = d->rolling_temp[i];
  }
  int tspan = thi - tlo < 8 ? 8 : thi - tlo;
  for (int h = 0; h < 24; h++) {
    bool open = !moon_slot(h);
    int bpm = (open && h <= now_h) ? d->hr_by_hour[h] : 0;
    int len = bpm > 0 ? (bpm - lo) * RAY_MAX / span : 0;
    int i = forecast_index(d, h);
    bool temp = open && d->weather_ok && i >= 0;
    if (temp) {
      paint_stick(fb, minute_angle(forecast_mid(d, h, i)),
                  3 + (d->rolling_temp[i] - tlo) * (RAY_MAX - 3) / tspan, FINE_HW, PAL[DAY]);
    }
    if (len >= 2) {
      paint_stick(fb, minute_angle(past_mid(d, h)), len, BOLD_HW, PAL[HEART]);
    }
    if (len < 2 && !temp) {
      paint_dot(fb, minute_angle(h * 60 + 30));
    }
  }
}

static void paint_dial(GContext *ctx, GRect bounds, const LabData *d, bool detail) {
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  if (!fb) {
    return;
  }
  paint_ground(fb, bounds, d);
  if (detail) {
    paint_detail_sticks(fb, d);
  } else {
    paint_face_sticks(fb, d);
  }
  graphics_release_frame_buffer(ctx, fb);
}

// Pixel glyphs ---------------------------------------------------------------------
// String rows on an integer grid, 2 px strokes, 10-14 px tall, one colour each.

static const char FOOT_ROWS[][16] = {
  "..........####.",
  ".........######",
  ".........######",
  ".........######",
  "..####...######",
  ".######...####.",
  ".######...###..",
  ".######........",
  ".######...####.",
  "..####....####.",
  "..###.....####.",
  "...............",
  "..####.........",
  "..####.........",
  "..####.........",
};

static const char HEART_ROWS[][12] = {
  ".###...###.",
  "#####.#####",
  "###########",
  "###########",
  "###########",
  ".#########.",
  "..#######..",
  "...#####...",
  "....###....",
  ".....#.....",
};

// Sun on the horizon: the next sunrise or sunset.
static const char HORIZON_ROWS[][17] = {
  ".......##.......",
  ".......##.......",
  "..##........##..",
  "...##......##...",
  "......####......",
  "....########....",
  "...##########...",
  "..############..",
  "................",
  "################",
  "################",
};

static const char THERMO_ROWS[][7] = {
  "..##..",
  ".####.",
  ".####.",
  ".####.",
  ".####.",
  ".####.",
  ".####.",
  "######",
  "######",
  "######",
  ".####.",
};

static const char FLAG_ROWS[][13] = {
  "############",
  "############",
  "##########..",
  "########....",
  "##########..",
  "############",
  "############",
  "##..........",
  "##..........",
  "##..........",
  "##..........",
  "##..........",
  "##..........",
};

static const char CHARGE_ROWS[][9] = {
  ".....###",
  "....###.",
  "...###..",
  "..###...",
  ".#######",
  "#######.",
  "...###..",
  "..###...",
  ".###....",
  "###.....",
};

static const char TICK_ROWS[][12] = {
  ".........##",
  "........##.",
  ".......##..",
  "##....##...",
  ".##..##....",
  "..####.....",
  "...##......",
};

static void draw_rows(GContext *ctx, const char *row0, int stride, int n, int x, int y, GColor c) {
  graphics_context_set_fill_color(ctx, c);
  for (int r = 0; r < n; r++) {
    const char *row = row0 + r * stride;
    int i = 0;
    while (row[i]) {
      if (row[i] != '#') {
        i++;
        continue;
      }
      int j = i;
      while (row[j] == '#') {
        j++;
      }
      graphics_fill_rect(ctx, GRect(x + i, y + r, j - i, 1), 0, GCornerNone);
      i = j;
    }
  }
}

#define ROWS_W(rows) ((int)sizeof(rows[0]) - 1)
#define ROWS_H(rows) ((int)(sizeof(rows) / sizeof(rows[0])))
#define DRAW_ROWS(ctx, rows, x, y, c) draw_rows(ctx, rows[0], sizeof(rows[0]), ROWS_H(rows), x, y, c)

// Weather glyphs (Solar) -----------------------------------------------------------

static const char SUN_BITS[][15] = {
  "......##......",
  "......##......",
  "..##......##..",
  "..##......##..",
  ".....####.....",
  "....######....",
  "##..######..##",
  "##..######..##",
  "....######....",
  ".....####.....",
  "..##......##..",
  "..##......##..",
  "......##......",
  "......##......",
};

static const char CLOUD_BITS[][17] = {
  "........####....",
  "......########..",
  "...###########..",
  "..#############.",
  ".###############",
  "################",
  "################",
  ".##############.",
};

// The sun behind a cloud: only the rays the cloud doesn't cover.
static const char PARTLY_SUN_BITS[][15] = {
  "......##......",
  "......##......",
  "..##......##..",
  "..##......##..",
  ".....####.....",
  "....######....",
  "##..######....",
  "##..######....",
  "....######....",
  ".....####.....",
};

static const char BOLT_BITS[][7] = {
  "...###",
  "..###.",
  ".#####",
  "...##.",
  "..##..",
};

static void draw_cloud(GContext *ctx, int x, int y, bool knockout, GColor c) {
  if (knockout) {   // 1 px of night sky so the cloud separates from the sun
    static const int8_t OFF[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int o = 0; o < 4; o++) {
      DRAW_ROWS(ctx, CLOUD_BITS, x + OFF[o][0], y + OFF[o][1], col(BG));
    }
  }
  DRAW_ROWS(ctx, CLOUD_BITS, x, y, c);
}

// Weather glyph with its top-left at (x, y), 13-14 px tall; returns its width.
static int draw_weather_glyph(GContext *ctx, int code, int x, int y) {
  switch (lab_weather_kind(code)) {
    case LabWxClear:
      DRAW_ROWS(ctx, SUN_BITS, x, y, col(SUN));
      return 14;
    case LabWxPartly:
      DRAW_ROWS(ctx, PARTLY_SUN_BITS, x, y, col(SUN));
      draw_cloud(ctx, x + 5, y + 6, true, col(SOFT));
      return 21;
    case LabWxFog:
      graphics_context_set_fill_color(ctx, col(SOFT));
      graphics_fill_rect(ctx, GRect(x + 1, y + 2, 13, 2), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(x + 3, y + 6, 13, 2), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(x, y + 10, 12, 2), 0, GCornerNone);
      return 16;
    case LabWxRain:
      draw_cloud(ctx, x, y + 1, false, col(SOFT));
      graphics_context_set_fill_color(ctx, col(RAIN));
      for (int i = 0; i < 3; i++) {
        graphics_fill_rect(ctx, GRect(x + 3 + 4 * i, y + 11, 2, 3), 0, GCornerNone);
      }
      return 16;
    case LabWxSnow:
      draw_cloud(ctx, x, y + 1, false, col(SOFT));
      graphics_context_set_fill_color(ctx, col(INK));
      for (int i = 0; i < 3; i++) {
        graphics_fill_rect(ctx, GRect(x + 3 + 4 * i, y + 11 + (i % 2), 2, 2), 0, GCornerNone);
      }
      return 16;
    case LabWxStorm:
      draw_cloud(ctx, x, y + 1, false, col(SOFT));
      DRAW_ROWS(ctx, BOLT_BITS, x + 5, y + 10, col(SUN));
      return 16;
    default:   // overcast or unknown
      draw_cloud(ctx, x, y + 3, false, col(SOFT));
      return 16;
  }
}

// Text -------------------------------------------------------------------------

#define TIME_RIGHT_BEARING 3
#define TIME_LEFT_BEARING 2
#define TOP_INK 6            // digit tops of the top corners
#define BOTTOM_INK 207       // digit tops of the bottom corners (bases at 220)
#define UV_INK 25            // the UV line under the temperature
#define UV_PAD 21            // "UV" label (Gothic 18 bold, same baseline), then the value

// LECO 36 bold hours and minutes around a hand-placed 5x5 colon, ink centred
// on x = 100. `ink_top` is the top of the digits.
static void draw_time(GContext *ctx, const LabData *d, int ink_top) {
  char hh[4], mm[4];
  int hour = d->now.tm_hour;
  if (!d->is_24h) {
    hour = hour % 12 == 0 ? 12 : hour % 12;
  }
  snprintf(hh, sizeof(hh), d->is_24h ? "%02d" : "%d", hour);
  snprintf(mm, sizeof(mm), "%02d", d->now.tm_min);
  const int dot_w = 5;
  const int gap = 4;
  int box_y = ink_top - 11;
  int shift = (lab_text_size(mm, s_font_time).w - lab_text_size(hh, s_font_time).w) / 2;
  int colon_x = MX - dot_w / 2 - shift;
  graphics_context_set_fill_color(ctx, col(INK));
  graphics_fill_rect(ctx, GRect(colon_x, ink_top + 5, dot_w, dot_w), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(colon_x, ink_top + 15, dot_w, dot_w), 0, GCornerNone);
  lab_text(ctx, hh, s_font_time, GRect(colon_x - gap - 80 + TIME_RIGHT_BEARING, box_y, 80, 40),
           GTextAlignmentRight, col(INK));
  lab_text(ctx, mm, s_font_time, GRect(colon_x + dot_w + gap - TIME_LEFT_BEARING, box_y, 80, 40),
           GTextAlignmentLeft, col(INK));
}

// Gothic 24 bold, ink centred on `cx` with the cap/digit tops at `ink_top`.
static void centred(GContext *ctx, const char *s, int cx, int ink_top, GColor c) {
  lab_text(ctx, s, s_font_big, GRect(cx - 60, ink_top - 10, 120, 32), GTextAlignmentCenter, c);
}

// A corner value in Gothic 24 bold, `pad` px in from the screen's outer edge.
static void corner_value(GContext *ctx, bool right, bool bottom, int pad, const char *val,
                         GColor c) {
  int top = (bottom ? BOTTOM_INK : TOP_INK) - 10;
  GRect box = right ? GRect(196 - pad - 100 + 1, top, 100, 32) : GRect(4 + pad - 1, top, 100, 32);
  lab_text(ctx, val, s_font_big, box, right ? GTextAlignmentRight : GTextAlignmentLeft, c);
}

// A corner icon on the outer edge, centred on the value's digits.
#define CORNER_ICON(ctx, right, bottom, rows, c)                         \
  DRAW_ROWS(ctx, rows, (right) ? 196 - ROWS_W(rows) : 4,                 \
            ((bottom) ? BOTTOM_INK : TOP_INK) + 7 - ROWS_H(rows) / 2, c)

#define ICON_CORNER(ctx, right, bottom, rows, ic, val, vc)          \
  do {                                                              \
    CORNER_ICON(ctx, right, bottom, rows, ic);                      \
    corner_value(ctx, right, bottom, ROWS_W(rows) + 5, val, vc);    \
  } while (0)

// Only exceptional wide ranges use Gothic 18 (11 px digits); normal ranges
// keep the prototype's Gothic 24. Each top group stays in its own half.
static void detail_range(GContext *ctx, bool right, const char *value, int pad, GColor color) {
  int available = right ? 84 - pad : 98 - pad;
  GFont font = lab_text_size(value, s_font_big).w <= available ? s_font_big : s_font_small;
  int ink_h = font == s_font_big ? 14 : 11;
  int y = TOP_INK - (ink_h - 4);
  GRect box = right ? GRect(196 - pad - available + 1, y, available, 32) :
                      GRect(4 + pad - 1, y, available, 32);
  lab_text(ctx, value, font, box, right ? GTextAlignmentRight : GTextAlignmentLeft, color);
}

// Sky bodies -----------------------------------------------------------------------

// Moon phase disc, northern-hemisphere orientation (waxing = lit on the right).
static void draw_moon(GContext *ctx, GPoint c, int r, int phase_e3) {
  disc(ctx, c, r + 2, col(BG));
  disc(ctx, c, r, col(NIGHT));
  int32_t k = cos_lookup((int32_t)phase_e3 * TRIG_MAX_ANGLE / 1000);
  bool waxing = phase_e3 < 500;
  graphics_context_set_fill_color(ctx, col(MOON));
  for (int dy = -r; dy <= r; dy++) {
    int w = isqrt(r * r - dy * dy + r);
    int t = (int)((int32_t)w * k / TRIG_MAX_RATIO);
    int x0 = waxing ? t : -w;
    int x1 = waxing ? w : -t;
    if (x1 > x0) {
      graphics_fill_rect(ctx, GRect(c.x + x0, c.y + dy, x1 - x0 + 1, 1), 0, GCornerNone);
    }
  }
}

// The sun on the sky ring, with a white core while it is up.
static void draw_sun(GContext *ctx, const LabData *d) {
  GPoint p = polar(R_SUN, minute_angle(d->minute_of_day));
  disc(ctx, p, SUN_R + 2, col(BG));
  disc(ctx, p, SUN_R, col(SUN));
  if (sun_is_up(d)) {
    disc(ctx, p, SUN_R - 3, col(CORE));
  }
}

// NZ time: a white bead riding the sky ring, set in a halo like the sun.
static void draw_world_marker(GContext *ctx, const LabData *d) {
  GPoint p = polar(R_SUN, minute_angle(d->world_hour * 60 + d->world_min));
  disc(ctx, p, 4, col(BG));
  disc(ctx, p, 2, col(INK));
}

static void draw_sky_bodies(GContext *ctx, const LabData *d) {
  draw_moon(ctx, GPoint(MX, MY + MOON_R), 7, d->moon_phase_e3);
  draw_world_marker(ctx, d);
  draw_sun(ctx, d);
}

// The porthole --------------------------------------------------------------------
//
// Everything inside is drawn dry (ink on night sky). The water then recolours
// the frame buffer below a Stokes waterline: sky becomes water, ink becomes
// the wet ink, so the type inverts exactly where the water crosses it.

static int32_t goal_of(const LabData *d) {
  return d->step_goal > 0 ? d->step_goal : 10000;
}

// The waterline's base row for a step count: the porthole's floor at 0, its
// top (wave included) at the goal and above.
static int water_y(int32_t steps, int32_t goal, int amp) {
  int32_t permille = steps * 1000 / goal;
  if (permille > 1000) {
    permille = 1000;
  }
  int bottom = MY + RW;
  int top = MY - RW + amp + 1;
  return bottom - (int)((bottom - top) * permille / 1000);
}

static void sea_state(const LabData *d, int *amp, int *wavelength) {
  *amp = 3;
  *wavelength = 40;
  if (d->weather_ok) {
    *amp = lab_clamp(3 + d->wind_kph / 40, 3, 5);
    *wavelength = lab_clamp(42 - d->wind_kph / 8, 30, 42);
  }
}

// Tide's second-order Stokes profile: peaked crests and long flat troughs.
static void fill_waterline(int base, int amp, int wavelength, int phase) {
  for (int i = 0; i <= 2 * RW; i++) {
    int32_t angle = (int32_t)((i + phase) % wavelength) * TRIG_MAX_ANGLE / wavelength;
    int32_t h = cos_lookup(angle) * 4 + cos_lookup(2 * angle);
    s_wl[i] = (int16_t)(base - round_div((int32_t)amp * h, TRIG_MAX_RATIO * 5));
  }
}

static void flood(GContext *ctx) {
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  if (!fb) {
    return;
  }
  for (int dy = -RW; dy <= RW; dy++) {
    int y = MY + dy;
    int hw = isqrt(RW * RW + RW - dy * dy);
    GBitmapDataRowInfo row = gbitmap_get_data_row_info(fb, y);
    for (int x = MX - hw; x <= MX + hw; x++) {
      int wl = s_wl[x - MX + RW];
      if (y < wl) {
        continue;
      }
      uint8_t *p = &row.data[x];
      *p = *p == PAL[BG] ? (y == wl ? PAL[CREST] : PAL[WATER]) : PAL[WET];
    }
  }
  graphics_release_frame_buffer(ctx, fb);
}

static void draw_porthole_water(GContext *ctx, const LabData *d) {
  int32_t steps = d->steps_ok && d->steps > 0 ? d->steps : 0;
  if (steps > 0) {
    int amp, wavelength;
    sea_state(d, &amp, &wavelength);
    fill_waterline(water_y(steps, goal_of(d), amp), amp, wavelength,
                   (d->minute_of_day * 7) % wavelength);
    flood(ctx);
  }
}

// Battery status, right-aligned on x = 196: the percentage, then a bolt while
// charging or else a 13x9 battery glyph whose fill uses Signal Deck's colours.
// `h` is the digit height of `f`. Returns the group's left edge.
static int draw_battery_status(GContext *ctx, const LabData *d, int ink_top, GFont f, int h,
                               GColor c) {
  char buf[16];
  int x = 196 - 13;
  if (d->charging) {
    x = 196 - ROWS_W(CHARGE_ROWS);
    DRAW_ROWS(ctx, CHARGE_ROWS, x, ink_top + (h - 10) / 2, c);
  } else {
    int y = ink_top + (h - 9) / 2;
    graphics_context_set_fill_color(ctx, c);
    graphics_fill_rect(ctx, GRect(x, y, 11, 9), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(x + 11, y + 3, 2, 3), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, col(BG));
    graphics_fill_rect(ctx, GRect(x + 2, y + 2, 7, 5), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, lab_battery_color(d->battery_pct));
    graphics_fill_rect(ctx, GRect(x + 3, y + 3, lab_clamp((d->battery_pct * 5 + 50) / 100, 1, 5),
                                  3), 0, GCornerNone);
  }
  snprintf(buf, sizeof(buf), "%d%%", d->battery_pct > 99 ? 99 : d->battery_pct);
  x -= 4;
  lab_text(ctx, buf, f, GRect(x - 60 + 1, ink_top - (h - 4), 60, 32), GTextAlignmentRight, c);
  return x - lab_text_size(buf, f).w;
}

// Face ---------------------------------------------------------------------------

static void draw(GContext *ctx, GRect bounds, const LabData *d) {
  graphics_context_set_antialiased(ctx, false);
  paint_dial(ctx, bounds, d, false);

  // The porthole, dry: the date over the time; a tick once the goal is met.
  char buf[24];
  snprintf(buf, sizeof(buf), "%s %d", lab_day_abbr(d->now.tm_wday), d->now.tm_mday);
  lab_upper(buf);
  centred(ctx, buf, MX, MY - 34, col(INK));
  draw_time(ctx, d, MY - 12);
  if (d->steps_ok && d->steps >= goal_of(d)) {
    DRAW_ROWS(ctx, TICK_ROWS, MX - 5, MY + 22, col(INK));
  }
  draw_porthole_water(ctx, d);
  draw_sky_bodies(ctx, d);

  // Top left: condition and temperature, the UV index under them.
  if (d->weather_ok) {
    snprintf(buf, sizeof(buf), "%d°", display_temp(d, d->temp));
    int gw = draw_weather_glyph(ctx, d->weather_code, 4, TOP_INK);
    corner_value(ctx, false, false, gw + 5, buf, col(INK));
    snprintf(buf, sizeof(buf), "%d", ((sun_known(d) && !sun_is_up(d) ? d->uv_max_x10 : d->uv_x10) + 5) / 10);
  } else {
    draw_cloud(ctx, 4, TOP_INK + 3, false, col(DOT));
    corner_value(ctx, false, false, 16 + 5, "--", col(DOT));
    snprintf(buf, sizeof(buf), "--");
  }
  lab_text(ctx, "UV", s_font_small, GRect(3, UV_INK - 4, 40, 24), GTextAlignmentLeft, col(SOFT));
  lab_text(ctx, buf, s_font_big, GRect(3 + UV_PAD, UV_INK - 10, 40, 32), GTextAlignmentLeft,
           d->weather_ok ? (sun_known(d) && !sun_is_up(d) ? col(SOFT) : col(INK)) : col(DOT));

  // Top right: the next sun event, unless the watch needs attention.
  bool low = d->battery_pct < 20 && !d->charging;
  if (!d->bt_connected || low || d->charging) {
    if (low || d->charging) {
      draw_battery_status(ctx, d, !d->bt_connected ? UV_INK : TOP_INK, s_font_big, 14,
                          d->charging ? col(SOFT) : lab_battery_color(d->battery_pct));
    }
    if (!d->bt_connected) {
      lab_text(ctx, "NO PHONE", s_font_small, GRect(107, TOP_INK - 4, 90, 24),
               GTextAlignmentRight, col(ALERT));
    }
  } else if (d->weather_ok && d->next_sun_min >= 0) {
    lab_fmt_minute_of_day(buf, sizeof(buf), d->next_sun_min, d->is_24h);
    ICON_CORNER(ctx, true, false, HORIZON_ROWS, col(DAY), buf, col(INK));
  } else {
    ICON_CORNER(ctx, true, false, HORIZON_ROWS, col(DOT), "--", col(DOT));
  }

  // Bottom: steps (the water already shows the goal) and heart rate.
  if (d->steps_ok) {
    lab_fmt_thousands(buf, sizeof(buf), d->steps);
  } else {
    snprintf(buf, sizeof(buf), "--");
  }
  ICON_CORNER(ctx, false, true, FOOT_ROWS, col(SUN), buf, d->steps_ok ? col(INK) : col(DOT));
  if (d->hr_ok) {
    snprintf(buf, sizeof(buf), "%d", d->heart_rate);
  } else {
    snprintf(buf, sizeof(buf), "--");
  }
  ICON_CORNER(ctx, true, true, HEART_ROWS, col(HEART), buf, d->hr_ok ? col(INK) : col(DOT));
}

// ---------------------------------------------------------------------------
// Detail view (tap): the same dial with the layers swapped. Behind the sun,
// each past hour's heart rate; ahead of it, the temperature to come. The
// porthole keeps its water and carries the step story: the ETA to the goal at
// today's pace (Summit's formula), the total and the pace against your usual
// day. The corners turn to ranges and times; their icons double as legend.
// ---------------------------------------------------------------------------

// Minute of the day the goal was reached (from steps by hour), or -1.
static int goal_minute(const LabData *d, int32_t goal) {
  int32_t cum = 0;
  for (int h = 0; h <= d->now.tm_hour && h < 24; h++) {
    int32_t s = d->steps_by_hour[h];
    if (s > 0 && cum + s >= goal) {
      // The current bucket contains only the elapsed part of this hour.
      int minutes = h == d->now.tm_hour ? d->now.tm_min : 60;
      return h * 60 + (int)(((goal - cum) * minutes) / s);
    }
    cum += s;
  }
  return -1;
}

// ETA at today's pace (Summit): steps per minute since the first active hour.
static int eta_minute(const LabData *d, int32_t goal) {
  if (!d->steps_ok || d->steps <= 0) {
    return -1;
  }
  int first = -1;
  for (int h = 0; h < 24; h++) {
    if (d->steps_by_hour[h] > 0) {
      first = h;
      break;
    }
  }
  int elapsed = d->minute_of_day - (first < 0 ? 0 : first * 60);
  if (elapsed < 1) {
    elapsed = 1;
  }
  int32_t left = goal - d->steps;
  return d->minute_of_day + (int)((left * elapsed) / d->steps);
}

// A number in LECO with a hand-placed thousands comma, centred on x = 100.
static void draw_big_number(GContext *ctx, GFont font, int32_t value, int ink_top, GColor color) {
  char head[12], tail[12];
  int box_y = ink_top - 11;
  if (value < 1000) {
    snprintf(tail, sizeof(tail), "%ld", (long)value);
    lab_text(ctx, tail, font, GRect(MX - 60, box_y, 120, 40), GTextAlignmentCenter, color);
    return;
  }
  snprintf(head, sizeof(head), "%ld", (long)(value / 1000));
  snprintf(tail, sizeof(tail), "%03ld", (long)(value % 1000));
  const int comma_w = 5;
  const int gap = 3;
  int wh = lab_text_size(head, font).w - TIME_LEFT_BEARING - TIME_RIGHT_BEARING;
  int wt = lab_text_size(tail, font).w - TIME_LEFT_BEARING - TIME_RIGHT_BEARING;
  int x = MX - (wh + gap + comma_w + gap + wt) / 2;
  lab_text(ctx, head, font, GRect(x - TIME_LEFT_BEARING, box_y, wh + 20, 40), GTextAlignmentLeft,
           color);
  int cx = x + wh + gap;
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, GRect(cx, ink_top + 20, comma_w, comma_w), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(cx, ink_top + 25, 3, 3), 0, GCornerNone);
  lab_text(ctx, tail, font, GRect(cx + comma_w + gap - TIME_LEFT_BEARING, box_y, wt + 20, 40),
           GTextAlignmentLeft, color);
}

static void draw_detail(GContext *ctx, GRect bounds, const LabData *d) {
  graphics_context_set_antialiased(ctx, false);
  paint_dial(ctx, bounds, d, true);

  int32_t goal = goal_of(d);
  int32_t steps = d->steps_ok && d->steps > 0 ? d->steps : 0;
  char buf[24], t[12];

  // Porthole: the goal ETA (flag) or the minute it was met (tick) over the
  // total, and the pace against your usual day below.
  int met = steps >= goal ? goal_minute(d, goal) : -1;
  int eta = steps >= goal ? -1 : eta_minute(d, goal);
  if (met >= 0 || (eta >= 0 && eta < 1440)) {
    lab_fmt_minute_of_day(t, sizeof(t), met >= 0 ? met : eta, d->is_24h);
  } else {
    snprintf(t, sizeof(t), "--:--");
  }
  int tw = lab_text_size(t, s_font_big).w;
  int fx = MX - (ROWS_W(FLAG_ROWS) + 5 + tw) / 2;
  if (met >= 0) {
    DRAW_ROWS(ctx, TICK_ROWS, fx, MY - 31, col(INK));
  } else {
    DRAW_ROWS(ctx, FLAG_ROWS, fx, MY - 34, col(INK));
  }
  lab_text(ctx, t, s_font_big, GRect(fx + ROWS_W(FLAG_ROWS) + 5 - 1, MY - 44, tw + 10, 32),
           GTextAlignmentLeft, col(INK));
  if (d->steps_ok) {
    draw_big_number(ctx, steps < 10000 ? s_font_time : s_font_mid, steps,
                    steps < 10000 ? MY - 12 : MY - 11, col(INK));
  } else {
    lab_text(ctx, "--", s_font_time, GRect(MX - 60, MY - 23, 120, 40), GTextAlignmentCenter,
             col(INK));
  }
  if (d->steps_ok && d->steps_typical > 0) {
    int32_t diff = steps - d->steps_typical;
    char n[12];
    lab_fmt_thousands(n, sizeof(n), diff < 0 ? -diff : diff);
    if (diff > -100 && diff < 100) {
      snprintf(buf, sizeof(buf), "ON PACE");
    } else {
      int nw = lab_text_size(n, s_font_big).w;
      int x = MX - (10 + 4 + nw) / 2;
      graphics_context_set_fill_color(ctx, col(INK));
      for (int row = 0; row < 10; ++row) {
        int w = diff > 0 ? row + 1 : 10 - row;
        graphics_fill_rect(ctx, GRect(x + (10 - w) / 2, MY + 23 + row, w, 1), 0, GCornerNone);
      }
      lab_text(ctx, n, s_font_big, GRect(x + 14 - 1, MY + 11, nw + 3, 32),
               GTextAlignmentLeft, col(INK));
    }
    if (diff > -100 && diff < 100) centred(ctx, buf, MX, MY + 21, col(INK));
  }
  draw_porthole_water(ctx, d);
  draw_sky_bodies(ctx, d);

  // Corners: temperature range (white over the peach streaks), heart range
  // (pink heart = the rays), the sun event the face doesn't show, and NZ.
  if (d->weather_ok) {
    snprintf(buf, sizeof(buf), "%d°/%d°", display_temp(d, d->temp_hi), display_temp(d, d->temp_lo));
  } else {
    snprintf(buf, sizeof(buf), "--");
  }
  CORNER_ICON(ctx, false, false, THERMO_ROWS, col(DAY));
  detail_range(ctx, false, buf, ROWS_W(THERMO_ROWS) + 5, d->weather_ok ? col(INK) : col(DOT));
  if (d->hr_min > 0 && d->hr_max > 0) {
    snprintf(buf, sizeof(buf), "%d-%d%s", d->hr_min, d->hr_max, d->hr_range_partial ? "*" : "");
  } else {
    snprintf(buf, sizeof(buf), d->hr_range_partial ? "--*" : "--");
  }
  CORNER_ICON(ctx, true, false, HEART_ROWS, col(HEART));
  detail_range(ctx, true, buf, ROWS_W(HEART_ROWS) + 5, d->hr_min > 0 ? col(INK) : col(DOT));

  bool day = sun_is_up(d);
  if (sun_known(d)) {
    lab_fmt_minute_of_day(buf, sizeof(buf), day ? d->sunrise_min : d->sunset_min, d->is_24h);
  } else {
    snprintf(buf, sizeof(buf), "--");
  }
  ICON_CORNER(ctx, false, true, HORIZON_ROWS, sun_known(d) ? col(DAY) : col(DOT), buf,
              sun_known(d) ? col(INK) : col(DOT));
  lab_fmt_time(t, sizeof(t), d->world_hour, d->world_min, d->is_24h);
  snprintf(buf, sizeof(buf), "%s %s", t, d->world_label);
  corner_value(ctx, true, true, 0, buf, col(INK));

  // Battery, always shown here: the face only mentions it when it matters.
  draw_battery_status(ctx, d, UV_INK, s_font_small, 11,
                      d->battery_pct < 40 ? lab_battery_color(d->battery_pct) : col(SOFT));
}

const LabDesign design_meridian2 = {"Meridian", load, NULL, draw, draw_detail};
