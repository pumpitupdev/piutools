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

GENERIC_PLUGINS := asound ata_hdd ata_hdd_infinity eeprom exec_blocker fake_libusb hasp \
		   io_buttonboard io_mk5io io_mk6io locale lockchip \
		   network pit s3d_opengl sighandler statfix system_info \
		   ticket_dispenser
PLUGINS := microdog stlfix usb_profile x11_keyboard_input $(GENERIC_PLUGINS)
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

$(PLUGIN_BUILD_ROOT)/stlfix.plugin: $(wildcard src/plugins/stlfix/*.c) | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC src/plugins/stlfix/*.c $(PLUGIN_INCLUDES) -ldl -o $@

$(PLUGIN_BUILD_ROOT)/usb_profile.plugin: $(wildcard src/plugins/usb_profile/*.c) $(wildcard src/plugins/usb_profile/*/*.c) | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC src/plugins/usb_profile/*.c src/plugins/usb_profile/nx2/*.c src/plugins/usb_profile/nxa/*.c src/plugins/usb_profile/fex/*.c src/plugins/usb_profile/fiesta/*.c src/plugins/usb_profile/fiesta2/*.c src/plugins/usb_profile/prime/*.c -lpthread $(PLUGIN_INCLUDES) -o $@

$(PLUGIN_BUILD_ROOT)/x11_keyboard_input.plugin: $(wildcard src/plugins/x11_keyboard_input/*.c) | $(PLUGIN_BUILD_ROOT)
	cc -shared -m32 -fPIC src/plugins/x11_keyboard_input/*.c $(PLUGIN_INCLUDES) -lX11 -o $@


.PHONY: clean
clean:
	rm -f $(PLUGIN_OBJS) $(BUILD_ROOT)/piutools.so