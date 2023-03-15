// Plugin for Ticket Dispenser Support
// Plugin for RainbowChina/SafeNET MicroDOG API Version 3.4
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>


#include <ticket.h>

typedef int (*ioctl_func_t)(int, unsigned long, ...);
typedef int (*open_func_t)(const char *, int);
ioctl_func_t next_ioctl;
open_func_t next_open;

static int dispenser_enabled;


static int x_ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void *arg = va_arg(args, void *);
    va_end(args);
    if(fd == FAKE_TICKET_FD){
        parse_ticketcmd((unsigned char*)arg);
        return 0;
    }
    return next_ioctl(fd, request, arg);
}

int x_open(const char *pathname, int flags) {
    printf("Ticket Dispenser Open\n");
    // Ticket Endpoint Handling
    if(strstr(pathname,OLD_TICKET_ENDPOINT)){
        // Do nothing for now.
        return FAKE_TICKET_FD;
    }
    return next_open(pathname, flags);
}

// fopen /proc/bus/usb/devices

static int read_config_file(const char *filename){
    config_t cfg;
    config_setting_t *setting;
    int success = 1;

    config_init(&cfg);

    if (!config_read_file(&cfg, filename)) {
        DBG_printf("[%s] Error: %s:%d - %s", __FILE__, config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        success = 0;
        goto cleanup;
    }

    setting = config_lookup(&cfg, "ticket_dispenser.enabled");
    if (setting != NULL && config_setting_type(setting) == CONFIG_TYPE_INT) {
        dispenser_enabled = config_setting_get_int(setting);
    }
cleanup:
    config_destroy(&cfg);
    return success;
}

static HCCHookInfoEntry entries[] = {
    {"libc.so.6","ioctl",(void*)x_ioctl,(void*)&next_ioctl,1},
    {"libc.so.6","open",(void*)x_open,(void*)&next_open,1}
};

int plugin_init(const char* config_path, PHCCHookInfoEntry *hook_entry_table){
    read_config_file(config_path); 
    if(!dispenser_enabled){return 0;} 
    *hook_entry_table = entries;
    return sizeof(entries) / sizeof(HCCHookInfoEntry);
}


