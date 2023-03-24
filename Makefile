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
plugins: asound.plugin  ata_hdd.plugin microdog_34.plugin s3d_opengl.plugin deadlock.plugin  filesystem_redirect.plugin ticket_dispenser.plugin usbfs_null.plugin io_x11_ckdur.plugin

asound.plugin:
	cc -shared -m32 -fPIC src/plugins/asound/asound.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

ata_hdd.plugin:
	cc -shared -m32 -fPIC src/plugins/ata_hdd/ata_hdd.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

deadlock.plugin:
	cc -shared -m32 -fPIC src/plugins/deadlock/deadlock.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

exec_blocker.plugin:
	cc -shared -m32 -fPIC src/plugins/exec_blocker/exec_blocker.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

locale.plugin:
	cc -shared -m32 -fPIC src/plugins/locale/locale.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

pit.plugin:
	cc -shared -m32 -fPIC src/plugins/pit/pit.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

io_mk5io.plugin:
	cc -shared -m32 $(PLUGIN_INCLUDES) src/plugins/io_mk5io/*.c -o $(PLUGIN_BUILD_ROOT)/$@

eeprom.plugin:
	cc -shared -m32 $(PLUGIN_INCLUDES) src/plugins/eeprom/*.c -o $(PLUGIN_BUILD_ROOT)/$@
	
lockchip.plugin:
	cc -shared -m32 $(PLUGIN_INCLUDES) src/plugins/lockchip/*.c -o $(PLUGIN_BUILD_ROOT)/$@

io_mk6io.plugin:
	cc -shared -m32 -fPIC src/plugins/io_mk6io/*.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

x11_keyboard_input.plugin:
	cc -shared -m32 -fPIC src/plugins/x11_keyboard_input/*.c $(PLUGIN_INCLUDES) -lX11 -o $(PLUGIN_BUILD_ROOT)/$@

fake_libusb.plugin:
	cc -shared -m32 -fPIC src/plugins/fake_libusb/*.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@


microdog.plugin:
	cc -shared -m32 $(PLUGIN_INCLUDES) src/plugins/microdog/microdog/*.c -I src/plugins/microdog/microdog src/plugins/microdog/microdog.c -o $(PLUGIN_BUILD_ROOT)/$@

s3d_opengl.plugin:
	cc -shared -m32 -fPIC src/plugins/s3d_opengl/s3d_opengl.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

usbfs_null.plugin:
	cc -shared -m32 -fPIC src/plugins/usbfs_null/usbfs_null.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

ticket_dispenser.plugin:
	cc -shared -m32 -fPIC src/plugins/ticket_dispenser/*.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

usb_profile.plugin:
	cc -shared -m32 -fPIC src/plugins/usb_profile/*.c src/plugins/usb_profile/nx2/*.c -lpthread $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@

# --- WORK IN PROGRESS ---
usbfs_emulator.plugin:
	cc -shared -m32 -fPIC src/plugins/usbfs_emulator/usbfs_emulator.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@


filesystem_redirect.plugin:
	cc -shared -m32 -fPIC src/plugins/filesystem_redirect/filesystem_redirect.c $(PLUGIN_INCLUDES) -o $(PLUGIN_BUILD_ROOT)/$@



