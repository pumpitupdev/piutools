BUILD_ROOT := build
PLUGIN_BUILD_ROOT := $(BUILD_ROOT)/plugins

PLUGIN_INCLUDES := src/plugin_sdk/*.c -I src

ifeq ($(findstring .plugin,$(MAKECMDGOALS)),.plugin)
  $(shell mkdir -p $(PLUGIN_BUILD_ROOT))
endif

# --- Core Dependencies ---
loader:
	cc -shared -m32 -fPIC src/piutools_loader.c $(PLUGIN_INCLUDES) -ldl -o $(BUILD_ROOT)/piutools.so

# --- Plugins ---
plugins: asound.plugin  ata_hdd.plugin microdog_34.plugin s3d_opengl.plugin deadlock.plugin  filesystem_redirect.plugin ticket_dispenser.plugin io_x11_ckdur.plugin

asound.plugin:
	cc -shared -m32 -fPIC src/plugins/asound/asound.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

ata_hdd.plugin:
	cc -shared -m32 -fPIC src/plugins/ata_hdd/ata_hdd.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

deadlock.plugin:
	cc -shared -m32 -fPIC src/plugins/deadlock/deadlock.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

ioport.plugin:
	cc -shared -m32 -fPIC src/plugins/ioport/ioport.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@


io_x11_ckdur.plugin:
	cc -shared -m32 -fPIC src/plugins/io_x11_ckdur/io_x11_ckdur.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

microdog_34.plugin:
	cc -shared -m32 $(PLUGIN_INCLUDES) src/plugins/microdog_34/microdog/*.c -I src/plugins/microdog_34/microdog src/plugins/microdog_34/microdog_34.c -o $(PLUGIN_BUILD_ROOT)/$@

s3d_opengl.plugin:
	cc -shared -m32 -fPIC src/plugins/s3d_opengl/s3d_opengl.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

usbfs_null.plugin:
	cc -shared -m32 -fPIC src/plugins/usbfs_null/usbfs_null.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

# --- WORK IN PROGRESS ---
usbfs_emulator.plugin:
	cc -shared -m32 -fPIC src/plugins/usbfs_emulator/usbfs_emulator.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@


filesystem_redirect.plugin:
	cc -shared -m32 -fPIC src/plugins/filesystem_redirect/filesystem_redirect.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

TICKET_DISPENSER_COMPONENTS := src/plugins/ticket/ticket.c -I src/plugins/ticket
ticket_dispenser.plugin:
	cc -shared -m32 -fPIC $(TICKET_DISPENSER_COMPONENTS) src/plugins/ticket_dispenser.c $(PPL_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@
