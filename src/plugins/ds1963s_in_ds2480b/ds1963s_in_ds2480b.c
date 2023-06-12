// vim: set expandtab sw=4 ts=4:
/*
 * ds1963s_in_ds2480b.c -- emulates a Dallas Semiconductor DS1963S
 * iButton in DS2480b (serial) housing
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <stdint.h>

#include "ds1963s-device.h"
#include "ds2480b-device.h"
#include "transport-factory.h"
#include "transport-pty.h"
#include "base64.h"

#include <PIUTools_SDK.h>

#define DEFAULT_SERIAL_DEVICE "/dev/ttyS0"

/* file ops */
typedef int (*open_func_t)(const char *, int);
static open_func_t next_open;
typedef int (*close_func_t)(int);
static close_func_t next_close;

static int cur_fd = -1;
static char *pathname;
static struct ds2480b_device ds2480b;
static struct ds1963s_device ds1963s;
static struct transport *serial;
static struct one_wire_bus bus;
static pthread_t one_wire_thread;

// static data that should be populated in the ds1963s authenticated
// data pages upon initialization
#define DS1963S_NUM_AUTH_DATA_PAGES 16
#define DS1963S_AUTH_PAGE_SIZE 32
static char static_auth_data_config_b64[DS1963S_NUM_AUTH_DATA_PAGES][128] = {0};
static uint8_t *auth_data_page_static_data[DS1963S_NUM_AUTH_DATA_PAGES] = {0};

static void *one_wire_loop() {
    one_wire_bus_run(&bus);
    DBG_printf("%s: one-wire thread exited\n", __FUNCTION__);
    return NULL;
}

int ds1963s_open(const char *path, int flags) {
    /* intercept the open() call for the serial device file and open our
     * emulated one instead */
    if (strcmp(path, DEFAULT_SERIAL_DEVICE) == 0) {
        DBG_printf("%s: intercepting open() to %s\n", __FUNCTION__, path);
        cur_fd = next_open(pathname, flags);
        return cur_fd;
    }
    return next_open(path, flags);
}

static void _populate_auth_page_static_data(struct ds1963s_device *dev) {
    for (int i = 0; i < DS1963S_NUM_AUTH_DATA_PAGES; i++) {
        if (auth_data_page_static_data[i] != NULL) {
            memcpy(&(ds1963s.data_memory[i*32]), auth_data_page_static_data[i], 32);
        }
    }
}

/*
 * converts base64 data and saves it as static data for the given auth
 * page
 */
static void _record_auth_page_static_data(int pagenum, char *data) {
    uint8_t *decoded_data = (uint8_t *)malloc(DS1963S_AUTH_PAGE_SIZE);
    base64_decode(data, strlen(data), decoded_data);
    DBG_printf("[%s:%d] authpage%d: %s\n", __FILE__, __LINE__, pagenum, decoded_data);
    auth_data_page_static_data[pagenum] = decoded_data;
}

int ds1963s_close(int fd) {
    /*
     * Recreate the transport for the emulated ibutton
     */
    int ret = next_close(fd);
    if (fd == cur_fd && cur_fd != -1) {
        /* recreate 1w transport entirely, by design the transport chokes after
         * the connection on the other side is closed.
         */
        transport_destroy(serial);
        int joinerr;
        if ((joinerr = pthread_join(one_wire_thread, NULL)) != 0) {
            fprintf(stderr, "%s: pthread_join hoarked: %d\n", __FUNCTION__, joinerr);
            //return -1;
        }

        if ((serial = transport_factory_new_by_name("pty")) == NULL) {
            fprintf(stderr, "ds1963s_in_ds2480b: failed to replace serial transport\n");
            return -1;
        }
        ds2480b.mode = DS2480_MODE_INACTIVE;
        ds2480b_dev_connect_serial(&ds2480b, serial);

        ds1963s.state = DS1963S_STATE_RESET_WAIT;

        {
            struct transport_pty_data *pdata;
            pdata = (struct transport_pty_data *)serial->private_data;
            pathname = pdata->pathname_slave;
            DBG_printf("[%s] Fake ds1963s (re-)ready at %s\n", __FILE__, pathname);
        }
        cur_fd = -1;

        // rebuild 1w bus
        one_wire_bus_init(&bus);
        if (ds2480b_dev_bus_connect(&ds2480b, &bus) == -1) {
            fprintf(stderr, "Could not reconnect DS2480 to 1-wire bus.\n");
            return -1;
        }
        ds1963s_dev_connect_bus(&ds1963s, &bus);

        if (pthread_create(&one_wire_thread, NULL, one_wire_loop, NULL) != 0) {
            fprintf(stderr, "Failed to create one-wire thread.\n");
            return -1;
        }
    }
    return ret;
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "open", ds1963s_open, &next_open, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "close", ds1963s_close, &next_close, 1),
    {}
};

static HookConfigEntry plugin_config[] = {
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage0",CONFIG_TYPE_STRING,static_auth_data_config_b64[0],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage1",CONFIG_TYPE_STRING,static_auth_data_config_b64[1],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage2",CONFIG_TYPE_STRING,static_auth_data_config_b64[2],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage3",CONFIG_TYPE_STRING,static_auth_data_config_b64[3],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage4",CONFIG_TYPE_STRING,static_auth_data_config_b64[4],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage5",CONFIG_TYPE_STRING,static_auth_data_config_b64[5],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage6",CONFIG_TYPE_STRING,static_auth_data_config_b64[6],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage7",CONFIG_TYPE_STRING,static_auth_data_config_b64[7],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage8",CONFIG_TYPE_STRING,static_auth_data_config_b64[8],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage9",CONFIG_TYPE_STRING,static_auth_data_config_b64[9],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage10",CONFIG_TYPE_STRING,static_auth_data_config_b64[10],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage11",CONFIG_TYPE_STRING,static_auth_data_config_b64[11],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage12",CONFIG_TYPE_STRING,static_auth_data_config_b64[12],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage13",CONFIG_TYPE_STRING,static_auth_data_config_b64[13],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage14",CONFIG_TYPE_STRING,static_auth_data_config_b64[14],sizeof(static_auth_data_config_b64[0])),
    CONFIG_ENTRY("DS1963S_IN_DS2480B","authpage15",CONFIG_TYPE_STRING,static_auth_data_config_b64[15],sizeof(static_auth_data_config_b64[0])),
    {}
};

const PHookEntry plugin_init(){
    memset(static_auth_data_config_b64, 0x0, sizeof(static_auth_data_config_b64));
    PIUTools_Config_Read(plugin_config);
    one_wire_bus_init(&bus);
    ds1963s_dev_init(&ds1963s);
    ds2480b_dev_init(&ds2480b);

    for (int i = 0; i < DS1963S_NUM_AUTH_DATA_PAGES; i++) {
        if (strlen(static_auth_data_config_b64[i]) > 0) {
            _record_auth_page_static_data(i, static_auth_data_config_b64[i]);
        }
    }
    _populate_auth_page_static_data(&ds1963s);

    if ((serial = transport_factory_new_by_name("pty")) == NULL) {
        fprintf(stderr, "ds1963s_in_ds2480b: failed to create serial transport\n");
        return NULL;
    }

    {
        struct transport_pty_data *pdata;
        pdata = (struct transport_pty_data *)serial->private_data;
        pathname = pdata->pathname_slave;
        DBG_printf("[%s] Fake ds1963s ready at %s\n", __FILE__, pathname);
    }

    ds2480b_dev_connect_serial(&ds2480b, serial);
    if (ds2480b_dev_bus_connect(&ds2480b, &bus) == -1) {
        fprintf(stderr, "Could not connect DS2480 to 1-wire bus.\n");
        return NULL;
    }
    ds1963s_dev_connect_bus(&ds1963s, &bus);

    if (pthread_create(&one_wire_thread, NULL, one_wire_loop, NULL) != 0) {
        fprintf(stderr, "Failed to create one-wire thread.\n");
        return NULL;
    }

    return entries;
}
