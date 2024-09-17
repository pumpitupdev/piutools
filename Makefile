BUILD_ROOT := dist
PLUGIN_BUILD_ROOT := $(BUILD_ROOT)/plugins

PLUGIN_INCLUDES := -I include
CFLAGS ?=

ifeq ($(findstring .plugin,$(MAKECMDGOALS)),.plugin)
  $(shell mkdir -p $(PLUGIN_BUILD_ROOT))
endif

LOADER_SOURCES := $(wildcard src/core/*.c)

# --- Meta Actions ---

all: $(BUILD_ROOT)/piutools.so plugins


.PHONY: dist
dist:
	mkdir -p ./dist
	cp -R ./ext/* ./dist/
	cp -R ./build/* ./dist/

$(BUILD_ROOT):
	mkdir -p $@

.PHONY: piutools.so
piutools.so: $(BUILD_ROOT)/piutools.so

# --- Core Dependencies ---
$(BUILD_ROOT)/piutools.so: $(BUILD_ROOT) $(LOADER_SOURCES)
	cc -shared $(CFLAGS) -m32 -fPIC $(LOADER_SOURCES) $(PLUGIN_INCLUDES) -ldl -o $@

# --- Plugins ---

GENERIC_PLUGINS := asound ata_hdd ata_hdd_infinity dlscard eeprom exec_blocker fake_libusb hasp \
		   io_buttonboard io_dlsio io_mk5io io_mk6io io_htbio locale lockchip \
		   network pit pumpio_udp s3d_opengl sighandler statfix system_info \
		   ticket_dispenser
PLUGINS := ds1963s_in_ds2480b microdog stlfix usb_profile x11_keyboard_input $(GENERIC_PLUGINS)
PLUGIN_OBJS := $(patsubst %,$(PLUGIN_BUILD_ROOT)/%.plugin,$(PLUGINS))

.PHONY: plugins
plugins: $(PLUGIN_OBJS)

$(PLUGIN_BUILD_ROOT):
	mkdir -p $(PLUGIN_BUILD_ROOT)

define generic_plugin_build
$(PLUGIN_BUILD_ROOT)/$(1).plugin: $$(wildcard src/plugins/$(1)/*.c) | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC $(CFLAGS) $$^ $(PLUGIN_INCLUDES) -o $$@
endef

$(foreach plugin_name,$(GENERIC_PLUGINS),$(eval $(call generic_plugin_build,$(plugin_name))))

# -- special snowflake plugin builds --

$(PLUGIN_BUILD_ROOT)/microdog.plugin: $(wildcard src/plugins/microdog/*.c) src/plugins/microdog/microdog.c | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC src/plugins/microdog/microdog/*.c src/plugins/microdog/microdog.c $(PLUGIN_INCLUDES) -o $@

reboot_blocker.plugin:
	cc -shared -m32 -fPIC src/plugins/reboot_blocker/reboot_blocker.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@
  
pit.plugin:
	cc -shared -m32 -fPIC src/plugins/pit/pit.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

reboot_blocker.plugin:
	cc -shared -m32 -fPIC src/plugins/reboot_blocker/reboot_blocker.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

s3d_opengl.plugin:
	cc -shared -m32 -fPIC src/plugins/s3d_opengl/s3d_opengl.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

sighandler.plugin:
	cc -shared -m32 -fPIC src/plugins/sighandler/sighandler.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

statfix.plugin:
	cc -shared -m32 -fPIC src/plugins/statfix/*.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

stlfix.plugin:
	cc -shared -m32 -fPIC src/plugins/stlfix/*.c $(PLUGIN_INCLUDES) -ldl -o $(PLUGIN_BUILD_ROOT)/$@

system_info.plugin:
	cc -shared -m32 -fPIC src/plugins/system_info/*.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

ticket_dispenser.plugin:
	cc -shared -m32 -fPIC src/plugins/ticket_dispenser/*.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

usb_profile.plugin:
	cc -shared -m32 -fPIC src/plugins/usb_profile/*.c src/plugins/usb_profile/nx2/*.c src/plugins/usb_profile/nxa/*.c src/plugins/usb_profile/fex/*.c src/plugins/usb_profile/fiesta/*.c src/plugins/usb_profile/fiesta2/*.c src/plugins/usb_profile/prime/*.c -lpthread $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

x11_keyboard_input.plugin:
	cc -shared -m32 -fPIC src/plugins/x11_keyboard_input/*.c $(PLUGIN_INCLUDES) -lX11 -o $(PLUGIN_BUILD_ROOT)/$@
=======
$(PLUGIN_BUILD_ROOT)/stlfix.plugin: $(wildcard src/plugins/stlfix/*.c) | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC src/plugins/stlfix/*.c $(PLUGIN_INCLUDES) -ldl -o $@

$(PLUGIN_BUILD_ROOT)/usb_profile.plugin: $(wildcard src/plugins/usb_profile/*.c) $(wildcard src/plugins/usb_profile/*/*.c) | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC src/plugins/usb_profile/*.c src/plugins/usb_profile/nx2/*.c src/plugins/usb_profile/nxa/*.c src/plugins/usb_profile/fex/*.c src/plugins/usb_profile/fiesta/*.c src/plugins/usb_profile/fiesta2/*.c src/plugins/usb_profile/prime/*.c -lpthread $(PLUGIN_INCLUDES) -o $@

$(PLUGIN_BUILD_ROOT)/x11_keyboard_input.plugin: $(wildcard src/plugins/x11_keyboard_input/*.c) | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC src/plugins/x11_keyboard_input/*.c $(PLUGIN_INCLUDES) -lX11 -o $@

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

DS1963S_IN_DS2480B_SOURCES := src/plugins/ds1963s_in_ds2480b/ds1963s_in_ds2480b.c \
							  src/plugins/ds1963s_in_ds2480b/base64.c

$(PLUGIN_BUILD_ROOT)/ds1963s_in_ds2480b.plugin: $(DS1963S_UTILS_SOURCES) $(DS1963S_IN_DS2480B_SOURCES) | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC $(CFLAGS) $(DS1963s_IN_DS2480B_SOURCES) $(DS1963S_UTILS_SOURCES) $(PLUGIN_INCLUDES) -lpthread -I src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src -o $@

.PHONY: clean
clean:
	rm -f $(PLUGIN_OBJS) $(BUILD_ROOT)/piutools.so
