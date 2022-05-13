TOP = $(CURDIR)/../
ICU_FILTER_PATH=$(abspath $(TOP)/icu-filters)

TARGET_OS ?= browser
TARGET_ARCHITECTURE ?= wasm

HOST_OBJDIR = $(TOP)/artifacts/obj/icu-host
TARGET_BINDIR = $(TOP)/artifacts/bin/icu-$(TARGET_OS)-$(TARGET_ARCHITECTURE)
TARGET_OBJDIR = $(TOP)/artifacts/obj/icu-$(TARGET_OS)-$(TARGET_ARCHITECTURE)

# Disable some features we don't need, see icu/icu4c/source/common/unicode/uconfig.h
# TODO: try adding -DUCONFIG_NO_LEGACY_CONVERSION=1
ICU_DEFINES = \
	-DUCONFIG_NO_FILTERED_BREAK_ITERATION=1 \
	-DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
	-DUCONFIG_NO_TRANSLITERATION=1 \
	-DUCONFIG_NO_FILE_IO=1 \
	-DU_CHARSET_IS_UTF8=1 \
	-DU_CHECK_DYLOAD=0 \
	-DU_ENABLE_DYLOAD=0

CONFIGURE_ARGS = \
	--enable-static \
	--disable-shared \
	--disable-tests \
	--disable-extras \
	--disable-samples \
	--disable-icuio \
	--disable-renaming \
	--disable-icu-config \
	--disable-layout \
	--disable-layoutex \
	--disable-tools \
	--with-cross-build=$(HOST_OBJDIR) \
	--with-data-packaging=archive \

ifeq ($(ICU_TRACING),true)
	CONFIGURE_ARGS += --enable-tracing
endif

# include the OS specific configs
include icu.$(TARGET_OS).mk

# Host build
$(HOST_OBJDIR) $(TARGET_BINDIR) $(TARGET_OBJDIR):
	mkdir -p $@

$(HOST_OBJDIR)/.stamp-host: $(HOST_OBJDIR)/.stamp-configure-host
	cd $(HOST_OBJDIR) && $(MAKE) -j8 all
	touch $@

$(HOST_OBJDIR)/.stamp-configure-host: | $(HOST_OBJDIR)
	cd $(HOST_OBJDIR) && $(TOP)/icu/icu4c/source/configure \
	--disable-icu-config --disable-extras --disable-tests --disable-samples
	touch $@


# Target build

# Parameters:
#  $(1): filter file name (without .json suffix)
#  $(2): data output file name (without .dat suffix)
define TargetBuildTemplate

$(TARGET_OBJDIR)/$(1):
	mkdir -p $$@

# Run the configure script
$(TARGET_OBJDIR)/$(1)/.stamp-configure: $(ICU_FILTER_PATH)/$(1).json $(HOST_OBJDIR)/.stamp-host | $(TARGET_OBJDIR)/$(1) check-env
	rm -rf $(TARGET_OBJDIR)/$(1)/data/out/tmp
	$(ENV_INIT_SCRIPT) cd $(TARGET_OBJDIR)/$(1) && \
	ICU_DATA_FILTER_FILE=$(ICU_FILTER_PATH)/$(1).json \
	$(ENV_CONFIGURE_WRAPPER) $(TOP)/icu/icu4c/source/configure \
	--prefix=$(TARGET_OBJDIR)/$(1)/install \
	$(CONFIGURE_ARGS) \
	$(CONFIGURE_COMPILER_FLAGS)
	touch $$@

# run source build and copy outputs to bin dir
lib-$(2): data-$(2)
	cd $(TARGET_OBJDIR)/$(1) && $(MAKE) -j8 all && $(MAKE) install
	rm -rf $(TARGET_BINDIR)/lib
	rm -rf $(TARGET_BINDIR)/include
	cp -R $(TARGET_OBJDIR)/$(1)/install/lib $(TARGET_BINDIR)/lib
	cp -R $(TARGET_OBJDIR)/$(1)/install/include $(TARGET_BINDIR)/include

# run data build and copy data file to bin dir
data-$(2): $(TARGET_OBJDIR)/$(1)/.stamp-configure | $(TARGET_OBJDIR)/$(1) $(TARGET_BINDIR)
	cd $(TARGET_OBJDIR)/$(1) && $(MAKE) -C data all && $(MAKE) -C data install
	cp $(TARGET_OBJDIR)/$(1)/data/out/icudt*.dat $(TARGET_BINDIR)/$(2).dat

endef

ifeq ($(TARGET_OS),browser)
$(eval $(call TargetBuildTemplate,icudt_browser,icudt))
else
$(eval $(call TargetBuildTemplate,icudt_mobile,icudt))
endif
$(eval $(call TargetBuildTemplate,icudt_CJK,icudt_CJK))
$(eval $(call TargetBuildTemplate,icudt_no_CJK,icudt_no_CJK))
$(eval $(call TargetBuildTemplate,icudt_EFIGS,icudt_EFIGS))

# build source+data for the main "icudt" filter and only data for the other filters
all: lib-icudt data-icudt data-icudt_no_CJK data-icudt_EFIGS data-icudt_CJK
