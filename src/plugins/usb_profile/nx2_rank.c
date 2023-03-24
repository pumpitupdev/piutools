#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "adler32.h"

#include "nx2_rank.h"

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

struct asset_nx2_usb_rank *asset_nx2_usb_rank_new(void)
{
  struct asset_nx2_usb_rank *rank;

  rank = (struct asset_nx2_usb_rank *) malloc(
      sizeof(struct asset_nx2_usb_rank));

  memset(rank, 0, sizeof(struct asset_nx2_usb_rank));

  return rank;
}

void asset_nx2_usb_rank_finalize(struct asset_nx2_usb_rank *rank)
{
  rank->adler32 = util_adler32_calc(
      1, ((const uint8_t *) rank) + 4, sizeof(struct asset_nx2_usb_rank) - 4);
}

char *asset_nx2_usb_rank_to_string(const struct asset_nx2_usb_rank *rank)
{
  char *buffer;
  char *buffer2;
  char *merged;

  buffer = (char *) malloc(128);
  memset(buffer, 0, 128);

   snprintf(
      buffer,
      128,
      "adler32: 0x%X\n"
      "num_rankings: %d\n"
      "ranking entries:\n",
      rank->adler32,
      rank->num_rankings);

  for (uint32_t i = 0; i < ASSET_NX2_USB_RANK_MAX_RANK_ENTRIES; i++) {

    for (uint32_t j = 0; j < ASSET_NX2_USB_RANK_MAX_STAGES; j++) {
      buffer2 = (char *) malloc(512);
      memset(buffer2, 0, 512);

      snprintf(
          buffer2,
          512,
          "%d/%d:\n"
          "game_mode: %d\n"
          "play_order: %d\n"
          "play_score: %d\n"
          "grade: %d\n"
          "mileage: %d\n"
          "play_time: %f\n"
          "kcal: %f\n"
          "v02: %f\n",
          i,
          j,
          rank->entries[i][j].game_mode,
          rank->entries[i][j].play_order,
          rank->entries[i][j].play_score,
          rank->entries[i][j].grade,
          rank->entries[i][j].mileage,
          rank->entries[i][j].play_time,
          rank->entries[i][j].kcal,
          rank->entries[i][j].v02);

      merged = util_str_merge(buffer, buffer2);

      free(buffer);
      free(buffer2);

      buffer = merged;
    }
  }

  return buffer;
}

void asset_nx2_usb_rank_decrypt(uint8_t *buf, size_t len)
{
  for (size_t a = len - 1; a > 0; --a) {
    buf[a] = (buf[a] ^ buf[a - 1]) + ((a * 1234567) >> 8);
  }
}

void asset_nx2_usb_rank_encrypt(uint8_t *buf, size_t len)
{
  for (size_t a = 1; a < len; ++a) {
    buf[a] = (buf[a] - ((a * 1234567) >> 8)) ^ buf[a - 1];
  }
}