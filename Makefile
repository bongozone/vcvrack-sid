RACK_DIR ?= ../..
SLUG = PulsumQuadratum-sid
VERSION = 0.6.0dev

FLAGS += -Idep/include/resid
SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += $(wildcard LICENSE*) res

# Static libs
libresid := dep/lib/libresid.a
OBJECTS += $(libresid)

# Dependencies
$(shell mkdir -p dep)
DEP_LOCAL := dep
DEPS += $(libresid)

$(libresid):
	cd dep && $(WGET) http://www.zimmers.net/anonftp/pub/cbm/crossplatform/emulators/resid/resid-0.16.tar.gz
	cd dep && $(UNTAR) resid-0.16.tar.gz
	cd dep/resid-0.16 && $(CONFIGURE)
	cd dep/resid-0.16 && $(MAKE)
	cd dep/resid-0.16 && $(MAKE) install


include $(RACK_DIR)/plugin.mk
