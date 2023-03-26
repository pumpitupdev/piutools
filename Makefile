BUILD_ROOT := build
PLUGIN_BUILD_ROOT := $(BUILD_ROOT)/plugins

PLUGIN_COMMON_SOURCES := src/plugin_sdk/*.c
PLUGIN_INCLUDES := -I src -I src/plugin_sdk

LOADER_CFLAGS ?= -shared -m32 -fPIC
PLUGIN_CFLAGS ?= $(LOADER_CFLAGS)

ifeq ($(findstring .plugin,$(MAKECMDGOALS)),.plugin)
  $(shell mkdir -p $(PLUGIN_BUILD_ROOT))
endif

PLUGIN_NAMES := asound \
				ata_hdd \
				deadlock \
				ds1963s_in_ds2480b \
				eeprom \
				exec_blocker \
				fake_libusb \
				filesystem_redirect \
				io_mk5io \
				io_mk6io \
				lockchip \
				locale \
				microdog \
				pit \
				s3d_opengl \
				ticket_dispenser \
				usbfs_null \
				x11_keyboard_input

PLUGIN_OBJS := $(patsubst %,$(PLUGIN_BUILD_ROOT)/%.plugin,$(PLUGIN_NAMES))

all: loader plugins

# --- Core Dependencies ---
.PHONY: loader
loader: $(BUILD_ROOT)/piutools.so

$(BUILD_ROOT)/piutools.so: src/piutools_loader.c $(PLUGIN_COMMON_SOURCES)
	cc $(LOADER_CFLAGS) $^ $(PLUGIN_INCLUDES) -ldl -o $@

$(PLUGIN_BUILD_ROOT)/%.plugin: $(PLUGIN_COMMON_SOURCES) src/plugins/%/*.c
	cc $(PLUGIN_CFLAGS) $(PLUGIN_COMMON_SOURCES) src/plugins/$(patsubst $(PLUGIN_BUILD_ROOT)/%.plugin,%,$@)/*.c $(PLUGIN_INCLUDES) -o $@

# --- Plugins ---
.PHONY: plugins
plugins: $(PLUGIN_OBJS)


# --- Plugins with special snowflake build reqs ---

DS1963S_UTILS_SOURCES := src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/1-wire-bus.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/coroutine.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/ds1963s-common.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/ds1963s-device.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/ds2480b-device.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/sha1.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/transport-factory.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/transport-pty.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/transport-unix.c \
						 src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src/transport.c

$(PLUGIN_BUILD_ROOT)/ds1963s_in_ds2480b.plugin: src/plugins/ds1963s_in_ds2480b/ds1963s_in_ds2480b.c $(DS1963S_UTILS_SOURCES)
	git submodule update --init --recursive # TODO: un-cheese
	cc $(PLUGIN_CFLAGS) $^ $(PLUGIN_INCLUDES) -lpthread -I src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src -o $@

$(PLUGIN_BUILD_ROOT)/x11_keyboard_input.plugin:
	cc $(PLUGIN_CFLAGS) $(PLUGIN_COMMON_SOURCES) src/plugins/x11_keyboard_input/*.c $(PLUGIN_INCLUDES) -lX11 -o $@

$(PLUGIN_BUILD_ROOT)/microdog.plugin: src/plugins/microdog/microdog.c
	cc $(PLUGIN_CFLAGS) $(PLUGIN_COMMON_SOURCES) src/plugins/microdog/microdog/*.c $(PLUGIN_INCLUDES) -I src/plugins/microdog/microdog -o $@

clean:
	rm -f $(PLUGIN_OBJS) $(BUILD_ROOT)/piutools.so
