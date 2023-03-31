#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "nxa_rank.h"

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

struct asset_nxa_usb_rank *asset_nxa_usb_rank_new(void)
{
  struct asset_nxa_usb_rank *rank;

  rank = (struct asset_nxa_usb_rank *) malloc(
      sizeof(struct asset_nxa_usb_rank));

  memset(rank, 0, sizeof(struct asset_nxa_usb_rank));

  return rank;
}

void asset_nxa_usb_rank_finalize(struct asset_nxa_usb_rank *rank)
{
  rank->adler32 = util_adler32_calc(
      1, ((const uint8_t *) rank) + 4, sizeof(struct asset_nxa_usb_rank) - 4);
}

char *asset_nxa_usb_rank_to_string(const struct asset_nxa_usb_rank *rank)
{
  char *buffer;
  char *buffer2;
  char *merged;
  char *usb_ser;

  buffer = (char *) malloc(512);
  memset(buffer, 0, 512);

  usb_ser = util_str_buffer(
      (const uint8_t *) rank->usb_serial, ASSET_NXA_USB_RANK_USB_SERIAL_LEN);

  snprintf(
      buffer,
      512,
      "adler32: 0x%X\n"
      // "usb_serial: %s\n",
      "num_rankings: %d\n"
      "ranking entries:\n",
      rank->adler32,
      // usb_ser,
      rank->num_rankings);

  for (uint32_t i = 0; i < ASSET_NXA_USB_RANK_MAX_RANK_ENTRIES; i++) {

    for (uint32_t j = 0; j < ASSET_NXA_USB_RANK_MAX_STAGES; j++) {
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

  free(usb_ser);

  return buffer;
}

void asset_nxa_usb_rank_decrypt(uint8_t *buf, size_t len)
{
  for (size_t a = len - 1; a > 0; --a) {
    buf[a] = (buf[a] ^ buf[a - 1]) + ((a * 1234567) >> 8);
  }
}

void asset_nxa_usb_rank_encrypt(uint8_t *buf, size_t len)
{
  for (size_t a = 1; a < len; ++a) {
    buf[a] = (buf[a] - ((a * 1234567) >> 8)) ^ buf[a - 1];
  }
}