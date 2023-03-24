#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nx2_save.h"

#define BASE 65521

static uint32_t util_adler32_calc(uint32_t initval, const uint8_t *input, size_t length)
{
  unsigned int s1 = initval & 0xffff;
  unsigned int s2 = (initval >> 16) & 0xffff;
  unsigned int n;

  for (n = 0; n < length; n++) {
    s1 = (s1 + input[n]) % BASE;
    s2 = (s2 + s1) % BASE;
  }

  return (s2 << 16) + s1;
}

static char *util_str_merge(const char *str1, const char *str2)
{
  char *out;
  size_t str1_len;
  size_t str2_len;
  size_t total;

  str1_len = strlen(str1);
  str2_len = strlen(str2);
  total = str1_len + str2_len + 1;

  out = malloc(total);
  memcpy(out, str1, str1_len);
  memcpy(out + str1_len, str2, str2_len);
  out[total - 1] = '\0';

  return out;
}
static char *util_str_buffer(const uint8_t *buf, size_t len)
{
  char *ret;
  size_t pos;

  pos = 0;
  ret = malloc(len * 3 + 1);

  for (size_t i = 0; i < len; i++) {
    uint8_t tmp = buf[i];

    pos += sprintf(ret + pos, "%02X ", tmp);
  }

  ret[len * 3] = '\0';

  return ret;
}

static char *
asset_nx2_usb_save_time_to_string(const struct asset_nx2_usb_save_time *time)
{
  char *buffer;

  buffer = (char *) malloc(128);
  memset(buffer, 0, 128);

  snprintf(
      buffer,
      128,
      "year: %d\n"
      "month: %d\n"
      "day: %d\n"
      "hour: %d\n"
      "min: %d\n"
      "ms: %d\n",
      time->year,
      time->month,
      time->day,
      time->hour,
      time->min,
      time->ms);

  return buffer;
}

static char *asset_nx2_usb_save_review_to_string(
    const struct asset_nx2_usb_save_review *review)
{
  char *buffer;
  char *land;
  char *mission;

  buffer = (char *) malloc(1024);
  memset(buffer, 0, 1024);

  land = util_str_buffer(
      (const uint8_t *) review->worldmax_current_land,
      ASSET_NX2_USB_SAVE_WORLDMAX_LOC_MAX);
  mission = util_str_buffer(
      (const uint8_t *) review->worldmax_current_mission,
      ASSET_NX2_USB_SAVE_WORLDMAX_LOC_MAX);

  snprintf(
      buffer,
      1024,
      ">>> review:\n"
      "player_id: %s\n"
      "mileage: %d\n"
      "reward_count: %d\n"
      "worldmax_count: %d\n"
      "play_count: %d\n"
      "worldmax_current_land: %s\n"
      "worldmax_current_mission: %s\n"
      "kcal: %d\n"
      "v02: %d\n",
      review->player_id,
      review->mileage,
      review->reward_count,
      review->worldmax_count,
      review->play_count,
      land,
      mission,
      review->kcal,
      review->v02);

  free(land);
  free(mission);

  return buffer;
}

static char *
asset_nx2_usb_save_stats_to_string(const struct asset_nx2_usb_save_stats *stats)
{
  char *merged;
  char *buffer;
  char *timestamp;
  char *tmp;
  const size_t buf_size = 512;

  merged = (char *) malloc(buf_size);
  memset(merged, 0, buf_size);

  buffer = (char *) malloc(buf_size);
  memset(buffer, 0, buf_size);

  timestamp = asset_nx2_usb_save_time_to_string(&stats->timestamp);

  snprintf(
      merged,
      buf_size,
      ">>> stats:\n"
      "adler32: 0x%X\n"
      "usb_serial: %s\n"
      "timestamp: %s"
      "avatar_id: %d\n"
      "rank: %d\n"
      "country_id: %d\n"
      "player_id: %s\n"
      "mileage: %d\n"
      "play_count: %d\n"
      "kcal: %d\n"
      "v02: %d\n"
      "worldmax_map_position: 0x%X\n"
      "reward_count: %d\n"
      "worldmax_count: %d\n",
      stats->adler32,
      stats->usb_serial,
      timestamp,
      stats->avatar_id,
      stats->rank,
      stats->country_id,
      stats->player_id,
      stats->mileage,
      stats->play_count,
      stats->kcal,
      stats->v02,
      stats->worldmax_map_position,
      stats->reward_count,
      stats->worldmax_count);

  free(timestamp);

  tmp = util_str_merge(merged, "song_unlocks:\n");

  free(merged);
  merged = tmp;

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_SONG_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "song slot: %d\n"
        "mode: 0x%X\n"
        "unused: 0x%X\n"
        "song: %d\n"
        "reveal: %d\n",
        i,
        stats->song_unlocks[i].mode,
        stats->song_unlocks[i].unused,
        stats->song_unlocks[i].song,
        stats->song_unlocks[i].reveal);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_MISSION_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "mission slot: %d\n"
        "clear_flags_directions: 0x%X\n"
        "clear_flag_mission: %d\n"
        "unused: 0x%X\n",
        i,
        stats->worldmax_mission_unlocks[i].clear_flags_directions,
        stats->worldmax_mission_unlocks[i].clear_flag_mission,
        stats->worldmax_mission_unlocks[i].unused);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_MISSION_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "worldmax_high_score %d: %d\n",
        i,
        stats->worldmax_high_score[i]);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_MISSION_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "worldmax_challenge %d: %d\n",
        i,
        stats->worldmax_challenge[i]);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_BARRICADE_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "worldmax_barricade %d:\n"
        "flag_open: %d\n"
        "unused: 0x%X\n",
        i,
        stats->worldmax_barricade[i].flag_open,
        stats->worldmax_barricade[i].unused);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_EVENT_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "worldmax_event %d:\n"
        "flag_cleared: %d\n"
        "unused: 0x%X\n",
        i,
        stats->worldmax_event[i].flag_cleared,
        stats->worldmax_event[i].unused);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_WARP_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "worldmax_warp %d:\n"
        "direction_clear: 0x%X\n"
        "clear: %d\n",
        "unused: 0x%X\n",
        i,
        stats->worldmax_warp[i].direction_clear,
        stats->worldmax_warp[i].clear,
        stats->worldmax_warp[i].unused);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_WORLDMAX_LOC_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "worldmax_current_land %d: %d\n",
        i,
        stats->worldmax_current_land[i]);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_WORLDMAX_LOC_MAX; i++) {

    snprintf(
        buffer,
        buf_size,
        "worldmax_current_mission %d: %d\n",
        i,
        stats->worldmax_current_mission[i]);

    tmp = util_str_merge(merged, buffer);

    free(merged);
    merged = tmp;
  }

  for (uint32_t i = 0; i < ASSET_NX2_USB_SAVE_SONG_MAX; i++) {

    for (uint32_t j = 0; j < ASSET_NX2_USB_SAVE_NUM_MODES; j++) {
      snprintf(
          buffer,
          buf_size,
          "song_scores %d/%d:\n"
          "score: %d\n"
          "player_id: %s\n",
          i,
          j,
          stats->song_scores[i][j].score,
          stats->song_scores[i][j].player_id);

      tmp = util_str_merge(merged, buffer);

      free(merged);
      merged = tmp;
    }
  }

  free(buffer);

  return merged;
}

struct asset_nx2_usb_save *asset_nx2_usb_save_new(void)
{
  struct asset_nx2_usb_save *save;
  char name[10];

  save = (struct asset_nx2_usb_save *) malloc(
      sizeof(struct asset_nx2_usb_save));

  memset(save, 0, sizeof(struct asset_nx2_usb_save));

  sprintf(name, "NX%06d", rand() % 1000000);
  strcpy(save->review.player_id, name);
  strcpy(save->stats.player_id, name);

  /* default world map position */
  save->stats.worldmax_map_position = 0xEF001;

  /* nx2 release date as default timestamp */
  save->stats.timestamp.year = 2008;
  save->stats.timestamp.month = 1;
  save->stats.timestamp.day = 21;
  save->stats.timestamp.hour = 0;
  save->stats.timestamp.min = 0;
  save->stats.timestamp.ms = 0;

  return save;
}

void asset_nx2_usb_save_finalize(struct asset_nx2_usb_save *save)
{
  /* checksum update */
  save->stats.adler32 = util_adler32_calc(
      1,
      ((const uint8_t *) &save->stats) + 4,
      sizeof(struct asset_nx2_usb_save_stats) - 4);
}

char *asset_nx2_usb_save_to_string(const struct asset_nx2_usb_save *save)
{
  char *review;
  char *stats;
  char *merged;

  review = asset_nx2_usb_save_review_to_string(&save->review);
  stats = asset_nx2_usb_save_stats_to_string(&save->stats);

  merged = util_str_merge(review, stats);

  free(review);
  free(stats);

  return merged;
}

void asset_nx2_usb_save_decrypt(uint8_t *buf, size_t len)
{
  /* skip non encrypted review area */
  buf += sizeof(struct asset_nx2_usb_save_review);
  len -= sizeof(struct asset_nx2_usb_save_review);

  for (size_t a = len - 1; a > 0; --a) {
    buf[a] = (buf[a] ^ buf[a - 1]) + ((a * 1234567) >> 8);
  }
}

void asset_nx2_usb_save_encrypt(uint8_t *buf, size_t len)
{
  /* skip non encrypted review area */
  buf += sizeof(struct asset_nx2_usb_save_review);
  len -= sizeof(struct asset_nx2_usb_save_review);

  for (size_t a = 1; a < len; ++a) {
    buf[a] = (buf[a] - ((a * 1234567) >> 8)) ^ buf[a - 1];
  }
}