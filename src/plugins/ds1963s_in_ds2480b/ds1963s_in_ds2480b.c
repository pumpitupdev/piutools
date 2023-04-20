// vim: set expandtab sw=4 ts=4:
/*
 * ds1963s_in_ds2480b.c -- emulates a Dallas Semiconductor DS1963S
 * iButton in DS2480b (serial) housing
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "ds1963s-device.h"
#include "ds2480b-device.h"
#include "transport-factory.h"
#include "transport-pty.h"

#include <PIUTools_SDK.h>

#define DEFAULT_SERIAL_DEVICE "/dev/ttyS0"

/* file ops */
typedef int (*open_func_t)(const char *, int);
open_func_t next_open;
typedef int (*close_func_t)(int);
close_func_t next_close;

int cur_fd = -1;
static char *pathname;
static struct ds2480b_device ds2480b;
static struct ds1963s_device ds1963s;
static struct transport *serial;
struct one_wire_bus bus;
pthread_t one_wire_thread;

void *one_wire_loop() {
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

const PHookEntry plugin_init(){
    one_wire_bus_init(&bus);
    ds1963s_dev_init(&ds1963s);
    ds2480b_dev_init(&ds2480b);

    if ((serial = transport_factory_new_by_name("pty")) == NULL) {
        fprintf(stderr, "ds1963s_in_ds2480b: failed to create serial transport\n");
        return NULL;
    }

    {
        struct transport_pty_data *pdata;
        pdata = (struct transport_pty_data *)serial->private_data;
        pathname = pdata->pathname_slave;
        PIUTools_Filesystem_AddRedirect(DEFAULT_SERIAL_DEVICE,pathname);
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

    return NULL;
}
