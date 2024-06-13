// DLS Card Plugin
#define _XOPEN_SOURCE 600
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#include "mem_hook.h"
#include "patch.h"
#include <PIUTools_SDK.h>
#include <time.h>


char fake_device_path[64];
char real_device_path[64];
char card_profile_path[64];
static char config_card_speed_hack = 0;
static char config_unlock_blocker = 0;
#define CARD_NUM_PAGES 47
#define CARD_PAGE_SIZE 48
#define CARD_DATA_SIZE (CARD_NUM_PAGES * CARD_PAGE_SIZE)

#define SPEED_HACK 1
//#define DEBUG_CARD_PROTOCOL 1

static unsigned char card_data[CARD_DATA_SIZE] = {0x00};
static unsigned char response_buf[256] = {0x00};  // Buffer for response data
static unsigned char card_inserted = 0;           // Flag for card inserted status
static size_t response_len = 0;                  // Length of response data
static size_t response_offset = 0;               // Offset for tracking read position

static unsigned int* clstate = (unsigned int*)0x0A7A1930;

// Card Insertion Thread
void *card_insertion_thread(void *arg) {
    unsigned char toggle_card_state = 0;
    unsigned char last_toggle_state = 0;
    // Set this to a toggle, I should be able to let go of the button.
    while (1) {
        toggle_card_state = (PIUTools_IO_IN[PINPUT_P1_NFC_READ]) ? 0x01 : 0;
        if(toggle_card_state && toggle_card_state != last_toggle_state){
            card_inserted ^= 0x01;
            DBG_printf("Card Inserted: %d\n", card_inserted);

        }
        last_toggle_state = toggle_card_state;
        usleep(500 * 1000);
    }         
    return NULL;
}

void setup_fake_terminal(const char *fake_path) {
    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    grantpt(master_fd);
    unlockpt(master_fd);
    char *slave_name = ptsname(master_fd);
    unlink(fake_path);
    symlink(slave_name, fake_path);
}


// -- Card Functions --
void load_card_data(void) {
    FILE *file = fopen(card_profile_path, "rb");
    if (file) {
        fread(card_data, sizeof(unsigned char), CARD_DATA_SIZE, file);
        fclose(file);
    } else {
        // Initialize card data with default values if file doesn't exist
        memset(card_data, 0, CARD_DATA_SIZE);
        card_data[0] = 0;
        card_data[1] = 0;
        card_data[2] = 0;
        card_data[3] = 0xFF;
        card_data[4] = 0x07;
        card_data[5] = 0x64;    
        card_data[16] = 0x05;
        card_data[17] = 0x00;
        card_data[18] = 0x00;   
        memset(card_data + 19, 0xFF, CARD_DATA_SIZE - 19); 
    }
    // Hack to set the 'number of plays' to 255 so we don't have the card run out.
    card_data[0x03] = 0xFF;
}

void save_card_data(void) {
    FILE *file = fopen(card_profile_path, "wb");
    if (file) {
        fwrite(card_data, sizeof(unsigned char), CARD_DATA_SIZE, file);
        fclose(file);
    }
}

int card_write(int fd, const unsigned char* buf, size_t nbyte) {
    if (nbyte < 2) return nbyte;  // Not enough data to process

    unsigned char cmd = buf[2];
    if (cmd == 'S') {
        // Start/Status command
        if(card_inserted){
            load_card_data();  // Load the card data
        }
        response_buf[0] = 0x02;
        response_buf[1] = 0x0A;
        response_buf[2] = 'S';
        response_buf[3] = 0x06;
        response_buf[4] =  card_inserted; // Status code depends on if the card is inserted.
        response_buf[5] = 0x01;  // Serial number
        response_buf[6] = 0x00;
        response_buf[7] = 0x00;
        response_buf[8] = 0x00;
        response_buf[9] = 0x00;  // Unknown
        response_buf[10] = 0xFF;
        response_buf[11] = 0x03;
        response_len = 12;
        response_offset = 0;
    } else if (cmd == 'R' && nbyte >= 5) {
        // Read command
        unsigned char page_num = buf[3];
        if (page_num > 0 && page_num <= CARD_NUM_PAGES) {
            unsigned char index = page_num - 1;
            response_buf[0] = 0x02;
            response_buf[1] = 0x34;
            response_buf[2] = 'R';
            response_buf[3] = 0x30;  // Number of bytes read (always 0x30)
            memcpy(response_buf + 4, card_data + index * CARD_PAGE_SIZE, CARD_PAGE_SIZE);
            response_buf[52] = 0xFF;
            response_buf[53] = 0x03;
            response_len = 54;
            response_offset = 0;
        } else {
            // Invalid page number
            response_buf[0] = 0x02;
            response_buf[1] = 0x07;
            response_buf[2] = 'R';
            response_buf[3] = page_num;
            response_buf[4] = 0x00;
            response_buf[5] = 0x00;
            response_buf[6] = 0x00;
            response_buf[7] = 0xFF;
            response_buf[8] = 0x03;
            response_len = 9;
            response_offset = 0;
        }
    } else if (cmd == 'W' && nbyte >= 54) {
        // Write command
        unsigned char page_num = buf[3];
        if (page_num > 0 && page_num <= CARD_NUM_PAGES) {
            unsigned char index = page_num - 1;
            memcpy(card_data + index * CARD_PAGE_SIZE, buf + 4, CARD_PAGE_SIZE);
            save_card_data();  // Save the card data

            // Valid response
            response_buf[0] = 0x02;
            response_buf[1] = 0x04;
            response_buf[2] = 'W';
            response_buf[3] = page_num;
            response_buf[4] = 0x01;  // Status code OK
            response_buf[5] = 0xFF;
            response_buf[6] = 0x03;
            response_len = 7;
            response_offset = 0;
        } else {
            // Invalid page number
            response_buf[0] = 0x02;
            response_buf[1] = 0x07;
            response_buf[2] = 'W';
            response_buf[3] = page_num;
            response_buf[4] = 0x00;
            response_buf[5] = 0x00;
            response_buf[6] = 0x00;
            response_buf[7] = 0xFF;
            response_buf[8] = 0x03;
            response_len = 9;
            response_offset = 0;
        }
    
    }

    #ifdef DEBUG_CARD_PROTOCOL
    DBG_printf("Card Write: %d\n", nbyte);
    DBG_printf("Request Packet: \n");
    DBG_print_buffer(buf, nbyte);
    #endif
    return nbyte;
}



int card_read(int fd, unsigned char* buf, size_t nbyte) {
    if (response_len == 0 || response_offset >= response_len) return 0;

    size_t to_read = nbyte < (response_len - response_offset) ? nbyte : (response_len - response_offset);
    memcpy(buf, response_buf + response_offset, to_read);
    response_offset += to_read;

    #ifdef DEBUG_CARD_PROTOCOL    
    DBG_printf("Card Read: %d\n", to_read);
    DBG_printf("Response Packet: \n");
    DBG_print_buffer(buf, to_read);
    DBG_printf("Response Length: %d\n", to_read);
    #endif    

    return to_read;
}


// -- Hooks --
#define FCN_CHECK_WRITE_1 (void*)0x0804EF9E
#define FCN_CHECK_READ_1  (void*)0x0804EFD4
#define FCN_CHECK_READ_2  (void*)0x0804F03F

#define FCN_READ_WRITE_1  (void*)0x0804F201
#define FCN_READ_READ_1   (void*)0x0804F225
#define FCN_READ_READ_2   (void*)0x0804F2E8

#define FCN_WRITE_WRITE_1 (void*)0x0804F0B8
#define FCN_WRITE_READ_1  (void*)0x0804F0E5


#define CARD_SLEEP_1 0x0804EFAA
#define CARD_SLEEP_2 0x0804F20D
#define CARD_SLEEP_3 0x0804F247
#define CARD_SLEEP_4 0x0804F26F
#define CARD_SLEEP_5 0x0804F32C
#define CARD_SLEEP_6 0x0804F0C4
#define CARD_SLEEP_7 0x0804F107
#define CARD_SLEEP_8 0x0804F126




static HookConfigEntry plugin_config[] = {
    CONFIG_ENTRY("DLSCARD","speed_hack",CONFIG_TYPE_BOOL,&config_card_speed_hack,sizeof(config_card_speed_hack)),
    CONFIG_ENTRY("DLSCARD","unlock_blocker",CONFIG_TYPE_BOOL,&config_unlock_blocker,sizeof(config_unlock_blocker)),
    {}
};


const PHookEntry plugin_init(void){

    PIUTools_Config_Read(plugin_config);    
    
    // Set up Filesystem Redirection for the Card Reader    
    sprintf(real_device_path, "/dev/ttyS%d", 0);
    sprintf(fake_device_path, "${TMP_ROOT_PATH}/ttyS%d", 0);
    PIUTools_Path_Resolve(fake_device_path,fake_device_path);        
    setup_fake_terminal(fake_device_path);
    chmod(fake_device_path, 0666);        
    PIUTools_Filesystem_AddRedirect(real_device_path,fake_device_path);

    // Set up Path for Card Profile
    sprintf(card_profile_path, "${SAVE_ROOT_PATH}/dlscard.dat");
    PIUTools_Path_Resolve(card_profile_path,card_profile_path);  

    // Patch Reads/Writes
    mem_hook_patch_function(FCN_CHECK_WRITE_1, card_write);
    mem_hook_patch_function(FCN_CHECK_READ_1, card_read);
    mem_hook_patch_function(FCN_CHECK_READ_2, card_read);

    mem_hook_patch_function(FCN_READ_WRITE_1, card_write);
    mem_hook_patch_function(FCN_READ_READ_1, card_read);
    mem_hook_patch_function(FCN_READ_READ_2, card_read);    
    
    mem_hook_patch_function(FCN_WRITE_WRITE_1, card_write);
    mem_hook_patch_function(FCN_WRITE_READ_1, card_read);
    
    // Remove I/O Sleep for Card Read/Write to Speed Things Up: HACK
    if(config_card_speed_hack){
        util_patch_write_memory(CARD_SLEEP_1, (const void *) "\x90\x90\x90\x90\x90", 5);
        util_patch_write_memory(CARD_SLEEP_2, (const void *) "\x90\x90\x90\x90\x90", 5);
        util_patch_write_memory(CARD_SLEEP_3, (const void *) "\x90\x90\x90\x90\x90", 5);
        util_patch_write_memory(CARD_SLEEP_4, (const void *) "\x90\x90\x90\x90\x90", 5);
        util_patch_write_memory(CARD_SLEEP_5, (const void *) "\x90\x90\x90\x90\x90", 5);
        util_patch_write_memory(CARD_SLEEP_6, (const void *) "\x90\x90\x90\x90\x90", 5);
        util_patch_write_memory(CARD_SLEEP_7, (const void *) "\x90\x90\x90\x90\x90", 5);
        util_patch_write_memory(CARD_SLEEP_8, (const void *) "\x90\x90\x90\x90\x90", 5);
    }

    if(config_unlock_blocker){
        util_patch_write_memory(0x0807a3ab, (const void *) "\xEB", 1);
        util_patch_write_memory(0x0807a3c1, (const void *) "\xEB", 1);
        util_patch_write_memory(0x0807a3e0, (const void *) "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90",10);
    }
    pthread_t card_state_thread;
    pthread_create(&card_state_thread, NULL, card_insertion_thread, NULL);
    printf("DLS Card Plugin Loaded\n");

return NULL;
}