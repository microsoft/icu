ARCH=x64
IOS_MIN_VERSION=8.0
ICU_FILTER = $(TOP)/icu-filters/icudt.json
ICU_VER = 67

ifeq ($(ARCH),x64)
	IOS_ARCH=x86_64
	IOS_SDK=iphonesimulator
	IOS_ICU_HOST=i686-apple-darwin11
endif
ifeq ($(ARCH),arm64)
	IOS_ARCH=arm64
	IOS_SDK=iphoneos
	IOS_ICU_HOST=arm-apple-darwin
endif
ifeq ($(ARCH),arm)
	IOS_ARCH=armv7s
	IOS_SDK=iphoneos
	IOS_ICU_HOST=arm-apple-darwin
endif

XCODE_DEVELOPER=$(shell xcode-select --print-path)
XCODE_SDK=$(shell xcodebuild -version -sdk $(IOS_SDK) | grep -E '^Path' | sed 's/Path: //')

all: icu-ios

TOP = $(CURDIR)/../
HOST_BUILDDIR = $(TOP)/artifacts/obj/icu-host
HOST_BINDIR = $(TOP)/artifacts/bin/icu-host
IOS_BUILDDIR = $(TOP)/artifacts/obj/icu-ios-$(IOS_ARCH)
IOS_BINDIR = $(TOP)/artifacts/bin/icu-ios-$(IOS_ARCH)

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

$(IOS_BUILDDIR):
	mkdir -p $@

.PHONY: icu-ios
icu-ios: $(IOS_BUILDDIR)/.stamp-configure-ios
	cd $(IOS_BUILDDIR) && $(MAKE) -j8 all && $(MAKE) install
	cp $(IOS_BUILDDIR)/data/out/icudt$(ICU_VER)l.dat $(IOS_BINDIR)/$(basename $(notdir $(ICU_FILTER))).dat

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

$(IOS_BUILDDIR)/.stamp-configure-ios: $(ICU_FILTER) $(HOST_BUILDDIR)/.stamp-host | $(IOS_BUILDDIR)
	rm -rf $(IOS_BUILDDIR)/data/out/tmp
	cd $(IOS_BUILDDIR) && \
	ICU_DATA_FILTER_FILE=$(ICU_FILTER) \
	$(TOP)/icu/icu4c/source/configure \
	--prefix=$(IOS_BINDIR) \
	--host=$(IOS_ICU_HOST) \
	--enable-static \
	--disable-shared \
	--disable-tests \
	--disable-extras \
	--disable-samples \
	--disable-icuio \
	--disable-renaming \
	--disable-icu-config \
	--disable-strict \
	--disable-layout \
	--disable-layoutex \
	--disable-tools \
	--disable-dyload \
	--with-cross-build=$(HOST_BUILDDIR) \
	--with-data-packaging=archive \
	$(CONFIGURE_ADD_ARGS) \
	CXX="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++" \
	CC="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang" \
	CFLAGS="-isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ -arch $(IOS_ARCH) -miphoneos-version-min=$(IOS_MIN_VERSION) -Oz $(ICU_DEFINES)" \
	CXXFLAGS="-isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ -I./include/ -arch $(IOS_ARCH) -miphoneos-version-min=$(IOS_MIN_VERSION) -fno-exceptions -Oz -Wno-sign-compare $(ICU_DEFINES)" \
	LDFLAGS="-L$(XCODE_SDK)/usr/lib/ -isysroot $(XCODE_SDK) -miphoneos-version-min=$(IOS_MIN_VERSION)"


