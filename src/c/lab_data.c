#include "lab.h"
#include <string.h>
#ifdef LAB_HOST_RENDER
#include <stdlib.h>
static int preview_int(const char *name,int fallback) {
  const char *text=getenv(name);
  return text ? atoi(text) : fallback;
}
#endif

#include "kit/kit.h"

#define DEMO_EPOCH 1792228140    // Sat 17 Oct 2026 10:09 BST (09:09 UTC)
#define STRESS_EPOCH 1790797680  // Wed 30 Sep 2026 20:48 BST (19:48 UTC)
#define DEMO_UTC_OFFSET 3600     // the demo/stress clock is UK summer time

static const uint16_t DEMO_STEPS_BY_HOUR[24] = {
  0, 0, 0, 0, 0, 0, 120, 980, 1640, 2612, 1490
};
static const int8_t DEMO_TEMP_BY_HOUR[24] = {
  10, 10, 9, 9, 9, 9, 9, 10, 11, 12, 14, 15, 16, 17, 17, 16, 15, 14, 13, 12, 11, 11, 10, 10
};
static const uint8_t DEMO_RAIN_BY_HOUR[24] = {
  5, 5, 5, 5, 0, 0, 0, 5, 10, 15, 20, 20, 25, 30, 45, 60, 55, 40, 25, 15, 10, 5, 5, 5
};

static const uint16_t STRESS_STEPS_BY_HOUR[24] = {
  0, 0, 0, 0, 0, 312, 1840, 2210, 980, 640, 1220, 860, 1530, 940, 720, 1110, 2380, 3150,
  590, 233, 50
};
static const int8_t STRESS_TEMP_BY_HOUR[24] = {
  -15, -16, -16, -17, -18, -18, -17, -15, -13, -10, -8, -6,
  -5, -4, -4, -5, -7, -9, -11, -12, -12, -13, -14, -14
};
static const uint8_t STRESS_RAIN_BY_HOUR[24] = {
  100, 100, 90, 80, 70, 60, 70, 80, 90, 100, 100, 95,
  90, 85, 80, 85, 90, 95, 100, 100, 100, 100, 100, 100
};

// [0] is today so far (Sat 17 Oct); Sundays (6, 13, 20, 27) are the quiet days.
static const uint16_t DEMO_STEPS_BY_DAY[28] = {
  6842, 11230, 9874, 12640, 8120, 10450, 4320, 13980, 9120, 10870, 7650, 11420, 9980, 5210,
  15240, 8840, 12010, 9430, 10760, 8330, 3980, 12880, 9760, 11140, 8590, 10320, 9050, 6120
};
static const uint8_t DEMO_HR_BY_HOUR[24] = {
  54, 53, 52, 52, 53, 55, 58, 68, 92, 118, 74
};

static const uint16_t STRESS_STEPS_BY_DAY[28] = {
  18765, 32104, 28450, 30999, 25870, 29120, 21480, 34960, 27310, 31870, 26540, 33210, 29980,
  22450, 35210, 28840, 32010, 27430, 30760, 26330, 23980, 34880, 29760, 31140, 28590, 30320,
  29050, 24120
};
static const uint8_t STRESS_HR_BY_HOUR[24] = {
  71, 64, 58, 52, 49, 55, 112, 148, 133, 121, 126, 119, 141, 137, 128, 152, 171, 187, 166,
  151, 148
};

// --- Shared calendar projection for fixed screenshot scenarios --------

static void fill_world_time(LabData *d) {
  kit_world_fill(d,0);
}


static void set_fixed_time(LabData *d, time_t epoch) {
  d->epoch = epoch;
  time_t local = epoch + DEMO_UTC_OFFSET;
  d->now = *gmtime(&local);
}

static void fill_demo_weather(LabData *d) {
  d->weather_ok = true;
  d->temp = 14;
  d->temp_hi = 17;
  d->temp_lo = 9;
  d->weather_code = 2;
  d->rain_pct = 20;
  d->uv_x10 = 30;
  d->humidity_pct = 72;
  d->wind_kph = 12;
  d->wind_from_deg = 225;
  d->sunrise_min = 7 * 60 + 29;
  d->sunset_min = 18 * 60 + 3;
  memcpy(d->temp_by_hour, DEMO_TEMP_BY_HOUR, sizeof(d->temp_by_hour));
  memcpy(d->rain_by_hour, DEMO_RAIN_BY_HOUR, sizeof(d->rain_by_hour));
}

__attribute__((unused)) static void fill_demo(LabData *d) {
  set_fixed_time(d, DEMO_EPOCH);
  d->is_24h = true;

  d->steps_ok = true;
  d->steps = 6842;
  d->step_goal = 10000;
  d->steps_typical = 5120;
  memcpy(d->steps_by_hour, DEMO_STEPS_BY_HOUR, sizeof(d->steps_by_hour));
  d->hr_ok = true;
  d->heart_rate = 64;
  d->hr_min = 52;
  d->hr_max = 141;
  d->active_minutes = 48;
  d->distance_m = 4930;
  d->active_kcal = 312;
  d->sleep_minutes = 7 * 60 + 24;
  memcpy(d->steps_by_day, DEMO_STEPS_BY_DAY, sizeof(d->steps_by_day));
  memcpy(d->hr_by_hour, DEMO_HR_BY_HOUR, sizeof(d->hr_by_hour));

  d->battery_pct = 76;
  d->charging = false;
  d->bt_connected = true;

  fill_demo_weather(d);
}

__attribute__((unused)) static void fill_stress(LabData *d) {
  set_fixed_time(d, STRESS_EPOCH);
  d->is_24h = true;

  d->steps_ok = true;
  d->steps = 18765;
  d->step_goal = 10000;
  d->steps_typical = 9000;
  memcpy(d->steps_by_hour, STRESS_STEPS_BY_HOUR, sizeof(d->steps_by_hour));
  d->hr_ok = true;
  d->heart_rate = 148;
  d->hr_min = 41;
  d->hr_max = 187;
  d->active_minutes = 214;
  d->distance_m = 13512;
  d->active_kcal = 1284;
  d->sleep_minutes = 11 * 60 + 23;
  memcpy(d->steps_by_day, STRESS_STEPS_BY_DAY, sizeof(d->steps_by_day));
  memcpy(d->hr_by_hour, STRESS_HR_BY_HOUR, sizeof(d->hr_by_hour));

  d->battery_pct = 100;
  d->charging = true;
  d->bt_connected = true;

  d->weather_ok = true;
  d->temp = -12;
  d->temp_hi = -4;
  d->temp_lo = -18;
  d->weather_code = 86;
  d->rain_pct = 100;
  d->uv_x10 = 110;
  d->humidity_pct = 100;
  d->wind_kph = 118;
  d->wind_from_deg = 315;
  d->sunrise_min = 7 * 60 + 1;
  d->sunset_min = 18 * 60 + 47;
  memcpy(d->temp_by_hour, STRESS_TEMP_BY_HOUR, sizeof(d->temp_by_hour));
  memcpy(d->rain_by_hour, STRESS_RAIN_BY_HOUR, sizeof(d->rain_by_hour));
}

__attribute__((unused)) static void fill_empty(LabData *d) {
  set_fixed_time(d, DEMO_EPOCH);
  d->is_24h = true;
  d->steps_ok = true;
  d->step_goal = 10000;
  d->battery_pct = 8;
  d->charging = false;
  d->bt_connected = false;
  d->weather_ok = false;
  d->weather_code = -1;
}


void lab_data_refresh(LabData *d) {
  memset(d, 0, sizeof(*d));
#if LAB_SCENARIO == LAB_STRESS
  fill_stress(d);
#elif LAB_SCENARIO == LAB_EMPTY
  fill_empty(d);
#elif LAB_SCENARIO == LAB_LIVE
  kit_live_data_refresh(d);
  return;
#else
  fill_demo(d);
#endif
  d->minute_of_day = d->now.tm_hour * 60 + d->now.tm_min;
#ifdef LAB_HOST_RENDER
  d->now.tm_hour=preview_int("MERIDIAN_HOUR",d->now.tm_hour)%24;
  d->now.tm_min=preview_int("MERIDIAN_MINUTE",d->now.tm_min)%60;
  d->minute_of_day=d->now.tm_hour*60+d->now.tm_min;
  d->is_24h=!preview_int("MERIDIAN_12H",!d->is_24h);

  d->temp_fahrenheit=preview_int("MERIDIAN_FAHRENHEIT",0);
  d->battery_pct=preview_int("MERIDIAN_BATTERY",d->battery_pct);
  d->charging=preview_int("MERIDIAN_CHARGING",d->charging);
  d->bt_connected=preview_int("MERIDIAN_PHONE",d->bt_connected);
  d->weather_code=preview_int("MERIDIAN_WEATHER_CODE",d->weather_code);
  d->uv_x10=preview_int("MERIDIAN_UV",d->uv_x10);
  d->heart_rate=preview_int("MERIDIAN_HR",d->heart_rate);
  d->hr_min=preview_int("MERIDIAN_HR_MIN",d->hr_min);
  d->hr_max=preview_int("MERIDIAN_HR_MAX",d->hr_max);
  d->hr_range_partial=preview_int("MERIDIAN_HR_PARTIAL",0);
  if(preview_int("MERIDIAN_NO_RANGE",0))d->hr_min=d->hr_max=0;
  d->steps=preview_int("MERIDIAN_STEPS",d->steps);
  d->steps_typical=preview_int("MERIDIAN_USUAL",d->steps_typical);
  d->step_goal=preview_int("MERIDIAN_GOAL",d->step_goal);
  d->temp=preview_int("MERIDIAN_TEMP",d->temp);
  d->temp_hi=preview_int("MERIDIAN_HI",d->temp_hi);
  d->temp_lo=preview_int("MERIDIAN_LO",d->temp_lo);
  if(preview_int("MERIDIAN_NO_WEATHER",0)) d->weather_ok=false;
  if(preview_int("MERIDIAN_NO_CURRENT_HR",0)) d->hr_ok=false;
  if(preview_int("MERIDIAN_NO_HEALTH",0)) {
    d->steps_ok=d->hr_ok=false;d->steps=d->heart_rate=d->hr_min=d->hr_max=0;
    memset(d->steps_by_day,0,sizeof(d->steps_by_day));
    memset(d->hr_by_hour,0,sizeof(d->hr_by_hour));
  }
#endif
  fill_world_time(d);
  kit_moon_fill(d);
  // Deterministic forecast extensions for the lab's three screenshot scenes.
  d->tomorrow_sunrise_min = d->sunrise_min + 2;
  d->tomorrow_sunset_min = d->sunset_min - 2;
  d->next_sun_is_set = d->minute_of_day >= d->sunrise_min && d->minute_of_day < d->sunset_min;
  d->next_sun_day_delta = d->minute_of_day >= d->sunset_min ? 1 : 0;
  d->next_sun_min = d->next_sun_is_set ? d->sunset_min :
      (d->next_sun_day_delta ? d->tomorrow_sunrise_min : d->sunrise_min);
#ifdef LAB_HOST_RENDER
  if(preview_int("MERIDIAN_NO_SUN",0)) {
    d->sunrise_min=d->sunset_min=d->tomorrow_sunrise_min=d->tomorrow_sunset_min=-1;
    d->next_sun_min=-1;
  }
#endif
  d->uv_max_x10 = d->uv_x10;
#ifdef LAB_HOST_RENDER
  d->uv_max_x10=preview_int("MERIDIAN_UV_MAX",d->uv_max_x10);
  d->rolling_count=preview_int("MERIDIAN_FORECAST_COUNT",d->weather_ok ? 24 : 0);
#else
  d->rolling_count = d->weather_ok ? 24 : 0;
#endif
  for (int i = 0; i < 24; ++i) {
    int h = (d->now.tm_hour + i) % 24;
    d->rolling_temp[i] = d->temp_by_hour[h];
    d->rolling_rain[i] = d->rain_by_hour[h];
#ifdef LAB_HOST_RENDER
    if(d->now.tm_hour+i>=24)
      d->rolling_rain[i]=preview_int("MERIDIAN_TOMORROW_RAIN",d->rolling_rain[i]);
#endif
    d->rolling_code[i] = d->weather_code;
    d->rolling_hour[i]=h;
    d->rolling_day[i]=(d->now.tm_hour+i)/24;
  }
}
