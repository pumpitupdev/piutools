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
	mkdir -p ./dist/plugins
	cp -R ./ext/* ./dist/
	if [ -d ./build ]; then cp -R ./build/* ./dist/; fi

.PHONY: piutools.so
piutools.so: $(BUILD_ROOT)/piutools.so

# --- Core Dependencies ---
$(BUILD_ROOT)/piutools.so: $(BUILD_ROOT) $(LOADER_SOURCES)
	cc -shared $(CFLAGS) -m32 -fPIC $(LOADER_SOURCES) $(PLUGIN_INCLUDES) -ldl -o $@

# --- Plugins ---

GENERIC_PLUGINS := asound ata_hdd ata_hdd_infinity eeprom exec_blocker fake_libusb hasp \
		   io_buttonboard io_mk5io io_mk6io locale lockchip \
		   network pit s3d_opengl sighandler statfix system_info \
		   ticket_dispenser
PLUGINS := ds1963s_in_ds2480b pro1_data_zip microdog stlfix usb_profile x11_keyboard_input $(GENERIC_PLUGINS)
PLUGIN_OBJS := $(patsubst %,$(PLUGIN_BUILD_ROOT)/%.plugin,$(PLUGINS))

.PHONY: plugins
plugins: $(PLUGIN_OBJS)

$(PLUGIN_BUILD_ROOT):
	mkdir -p $(PLUGIN_BUILD_ROOT)

define generic_plugin_build
$(PLUGIN_BUILD_ROOT)/$(1).plugin: $$(wildcard src/plugins/$(1)/*.c) | $(BUILD_ROOT)
	cc -shared -m32 -fPIC $(CFLAGS) $$^ $(PLUGIN_INCLUDES) -o $$@
endef

$(foreach plugin_name,$(GENERIC_PLUGINS),$(eval $(call generic_plugin_build,$(plugin_name))))

# -- special snowflake plugin builds --

$(PLUGIN_BUILD_ROOT)/microdog.plugin: $(wildcard src/plugins/microdog/*.c) src/plugins/microdog/microdog.c | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC src/plugins/microdog/microdog/*.c src/plugins/microdog/microdog.c $(PLUGIN_INCLUDES) -o $@

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

$(PLUGIN_BUILD_ROOT)/ds1963s_in_ds2480b.plugin: $(DS1963S_UTILS_SOURCES) $(DS1963S_IN_DS2480B_SOURCES)
	cc -shared -m32 -fPIC $(CFLAGS) $(DS1963S_IN_DS2480B_SOURCES) $(DS1963S_UTILS_SOURCES) $(PLUGIN_INCLUDES) -lpthread -I src/plugins/ds1963s_in_ds2480b/ds1963s-utils/src -o $@

PRO1_DATA_ZIP_OW_SOURCES := src/plugins/pro1_data_zip/ow/crcutil.c \
							src/plugins/pro1_data_zip/ow/ds2480ut.c \
							src/plugins/pro1_data_zip/ow/linuxlnk.c \
							src/plugins/pro1_data_zip/ow/owerr.c \
							src/plugins/pro1_data_zip/ow/owllu.c \
							src/plugins/pro1_data_zip/ow/ownetu.c \
							src/plugins/pro1_data_zip/ow/owsesu.c \
							src/plugins/pro1_data_zip/ow/owtrnu.c \
							src/plugins/pro1_data_zip/ow/sha18.c \
							src/plugins/pro1_data_zip/ow/shaib.c

PRO1_DATA_ZIP_SOURCES := src/plugins/pro1_data_zip/pro1_data_zip.c \
						 src/plugins/pro1_data_zip/aes.c \
						 src/plugins/pro1_data_zip/dongle.c \
						 src/plugins/pro1_data_zip/enc_zip_file.c \
						 src/plugins/pro1_data_zip/sig.c \
						 src/plugins/pro1_data_zip/util.c

#LTC_OBJS := src/plugins/pro1_data_zip/ltc/linux_x86/libtomcrypt.a \
#			src/plugins/pro1_data_zip/ltc/linux_x86/libtommath.a
LTC_OBJS := src/plugins/pro1_data_zip/ltc/linux_x86/libtomcrypt_debug.a \
			src/plugins/pro1_data_zip/ltc/linux_x86/libtommath.a

$(PLUGIN_BUILD_ROOT)/pro1_data_zip.plugin: $(PRO1_DATA_ZIP_OW_SOURCES) $(PRO1_DATA_ZIP_SOURCES) $(LTC_OBJS)
	cc -shared -m32 -fPIC $(CFLAGS) $(PRO1_DATA_ZIP_SOURCES) $(PRO1_DATA_ZIP_OW_SOURCES) $(LTC_OBJS) -I src/plugins/pro1_data_zip/ltc/headers $(PLUGIN_INCLUDES) -o $@

.PHONY: clean
clean:
	rm -f $(PLUGIN_OBJS) $(BUILD_ROOT)/piutools.so
