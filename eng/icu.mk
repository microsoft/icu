all: icu-wasm

TOP = $(CURDIR)/../
HOST_BUILDDIR = $(TOP)/artifacts/obj/icu-host
HOST_BINDIR = $(TOP)/artifacts/bin/icu-host
WASM_BUILDDIR = $(TOP)/artifacts/obj/icu-wasm
WASM_BINDIR = $(TOP)/artifacts/bin/icu-wasm
ICU_FILTERS = $(TOP)/icu-filters

check-env:
	@if [ -z "$(EMSDK_PATH)" ]; then echo "The EMSDK_PATH environment variable needs to set to the location of the emscripten SDK."; exit 1; fi

$(HOST_BUILDDIR):
	mkdir -p $@

.PHONY: host
host: $(HOST_BUILDDIR)/.stamp-host

$(HOST_BUILDDIR)/.stamp-host: $(HOST_BUILDDIR)/.stamp-configure-host
	cd $(HOST_BUILDDIR) && $(MAKE) -j8 all && $(MAKE) install
	touch $@

$(HOST_BUILDDIR)/.stamp-configure-host: | $(HOST_BUILDDIR)
	cd $(HOST_BUILDDIR) && $(TOP)/icu/icu4c/source/configure --prefix=$(HOST_BINDIR) --disable-icu-config
	touch $@

$(WASM_BUILDDIR):
	mkdir -p $@

.PHONY: icu-wasm
icu-wasm: $(WASM_BUILDDIR)/.stamp-configure-wasm
	cd $(WASM_BUILDDIR) && $(MAKE) -j8 all && $(MAKE) install

$(WASM_BUILDDIR)/.stamp-configure-wasm: $(HOST_BUILDDIR)/.stamp-host | $(WASM_BUILDDIR) check-env
	cd $(WASM_BUILDDIR) && source $(EMSDK_PATH)/emsdk_env.sh && ICU_DATA_FILTER_FILE=$(ICU_FILTERS)/optimal.json \
	emconfigure $(TOP)/icu/icu4c/source/configure --prefix=$(WASM_BINDIR) --enable-static --disable-shared CFLAGS="-g -Oz" CXXFLAGS="-g -Oz -Wno-sign-compare" --with-cross-build=$(HOST_BUILDDIR) --disable-icu-config --with-data-packaging=archive --disable-extras --disable-renaming
	touch $@
