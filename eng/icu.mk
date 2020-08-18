all: icu-wasm

TOP = $(CURDIR)/../
HOST_BUILDDIR = $(TOP)/artifacts/obj/icu-host
HOST_BINDIR = $(TOP)/artifacts/bin/icu-host
WASM_BUILDDIR = $(TOP)/artifacts/obj/icu-wasm
WASM_BINDIR = $(TOP)/artifacts/bin/icu-wasm
ICU_FILTER = $(TOP)/icu-filters/optimal.json
ICU_VER = 67

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
	cd $(HOST_BUILDDIR) && $(TOP)/icu/icu4c/source/configure \
	--prefix=$(HOST_BINDIR) --disable-icu-config --disable-extras --disable-tests --disable-samples
	touch $@

$(WASM_BUILDDIR):
	mkdir -p $@

.PHONY: icu-wasm
icu-wasm: $(WASM_BUILDDIR)/.stamp-configure-wasm
	cd $(WASM_BUILDDIR) && $(MAKE) -j8 all && $(MAKE) install
	cp $(WASM_BUILDDIR)/data/out/icudt$(ICU_VER)l.dat $(WASM_BINDIR)/icudt_$(basename $(notdir $(ICU_FILTER))).dat

# Disable some features we don't need, see icu/icu4c/source/common/unicode/uconfig.h
ICU_DEFINES= \
	-DUCONFIG_NO_FILTERED_BREAK_ITERATION=1 \
	-DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
	-DUCONFIG_NO_TRANSLITERATION=1 \
	-DUCONFIG_NO_FILE_IO=1 \
	-DU_CHARSET_IS_UTF8=1 \
	-DU_CHECK_DYLOAD=0 \
	-DU_ENABLE_DYLOAD=0

CONFIGURE_ADD_ARGS=
ifeq ($(ICU_TRACING),true)
    CONFIGURE_ADD_ARGS += --enable-tracing
endif

$(WASM_BUILDDIR)/.stamp-configure-wasm: $(ICU_FILTER) $(HOST_BUILDDIR)/.stamp-host | $(WASM_BUILDDIR) check-env
	rm -rf $(WASM_BUILDDIR)/data/out/tmp
	cd $(WASM_BUILDDIR) && source $(EMSDK_PATH)/emsdk_env.sh && \
	ICU_DATA_FILTER_FILE=$(ICU_FILTER) \
	emconfigure $(TOP)/icu/icu4c/source/configure \
	--prefix=$(WASM_BINDIR) \
	--enable-static \
	--disable-shared \
	--disable-tests \
	--disable-extras \
	--disable-samples \
	--disable-icuio \
	--disable-renaming \
	--disable-icu-config \
	--with-cross-build=$(HOST_BUILDDIR) \
	--with-data-packaging=archive \
	$(CONFIGURE_ADD_ARGS) \
	CFLAGS="-Oz $(ICU_DEFINES)" \
	CXXFLAGS="-fno-exceptions -Oz -Wno-sign-compare $(ICU_DEFINES)"
