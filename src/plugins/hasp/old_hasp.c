#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "old_hasp.h"

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
struct sec_hasp_key_table {
  uint32_t nkeys;
  struct sec_hasp_key *keys;
};

/* index 0 (type): 1 = dev, 2 = retail 0=retail f2
   index 1 (language): 3001 = english */
/* 
3001 -> World
3002 -> Korea
3003 -> Mexico
3004 -> Brazil
3005 -> China
3006 -> Japan
 */
static uint32_t sec_hasp_features[] = {0, 3001};
static const uint32_t sec_hasp_key_id = 0x1234;
static struct sec_hasp_key_table sec_hasp_keys;


int is_feature_present(int feature){
        int i;
        for(i=0;i<2;i++){
                if(feature == sec_hasp_features[i]){
                        return 1;
                }
        }
        return 0;
}

void sec_hasp_init(const uint8_t *key_data, size_t len, int language_fid)
{
    printf("SEC HASP INIT\n");
    sec_hasp_features[1] = language_fid;
  sec_hasp_keys.nkeys = len / sizeof(struct sec_hasp_key);
  sec_hasp_keys.keys =
      malloc(sizeof(struct sec_hasp_key) * sec_hasp_keys.nkeys);

  memcpy(
      sec_hasp_keys.keys,
      key_data,
      sec_hasp_keys.nkeys * sizeof(struct sec_hasp_key));

  printf("Loaded %d keys\n", sec_hasp_keys.nkeys);
}

int sec_hasp_api_login(int feature, int vendor_code, int *handle){
    if(!is_feature_present(feature)){return 1;}
  *handle = (int) &sec_hasp_key_id;

  //printf("login, feature %d, vendor_code %d, ret handle 0x%X\n",feature,vendor_code,*handle);

  return 0;
}

int sec_hasp_api_logout(int handle)
{
  if (handle != (int) &sec_hasp_key_id) {
   // printf("Logout of unknown handle 0x%X\n", handle);
  } else {
  //  printf("Logout: 0x%X\n", handle);
  }

  return 0;
}

unsigned int sec_hasp_api_getid(void)
{
    printf("HASP KEY GET ID\n");

  return sec_hasp_key_id;
}

int sec_hasp_get_info(const char *scope, const char *format,char **info){
    char hasp_info[128] = {0x00};
    //printf("[%s] IN\n",__FUNCTION__);
    sprintf(hasp_info,"<hasp id=\"%d\"/>",sec_hasp_key_id);
    //printf("Populated Hasp Info: %s\n",hasp_info);
    if(info != NULL){
        *info = malloc(strlen(hasp_info)+1);
        strcpy(*info,hasp_info);
    }
    //printf("[%s] OUT\n",__FUNCTION__);
    return 0;
}

int sec_hasp_api_decrypt(int handle, void *buffer, size_t length)
{
  char *buf;

  if (handle != (int) &sec_hasp_key_id) {
    printf("Unknown handle 0x%X sending decrypt request, ignored\n");
    return -1;
  }

  for (uint32_t i = 0; i < sec_hasp_keys.nkeys; i++) {
    if (!memcmp(
            buffer,
            sec_hasp_keys.keys[i].req,
            sizeof(sec_hasp_keys.keys[i].req))) {

      printf(
         "Decrypt %u -> %u\n",
        *((uint64_t *) sec_hasp_keys.keys[i].req),
        *((uint64_t *) sec_hasp_keys.keys[i].resp));
      memcpy(
          buffer,
          sec_hasp_keys.keys[i].resp,
          sizeof(sec_hasp_keys.keys[i].resp));
      return 0;
    }
  }

  buf = util_str_buffer(buffer, length);

  printf("Missing key for request %s\n", buf);
  free(buf);

  return 0;
}