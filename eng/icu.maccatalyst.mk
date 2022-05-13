MACCATALYST_SDK=macosx

ifeq ($(TARGET_ARCHITECTURE),arm64)
	MACCATALYST_ARCH=-arch arm64
	MACCATALYST_TARGET=-target arm64-apple-ios14.2-macabi
	MACCATALYST_ICU_HOST=arm-apple-darwin
endif
ifeq ($(TARGET_ARCHITECTURE),x64)
	MACCATALYST_ARCH=-arch x86_64
	MACCATALYST_TARGET=-target x86_64-apple-ios13.5-macabi
	MACCATALYST_ICU_HOST=i686-apple-darwin11
endif

XCODE_DEVELOPER := $(shell xcode-select --print-path)
XCODE_SDK := $(shell xcodebuild -version -sdk $(MACCATALYST_SDK) | grep -E '^Path' | sed 's/Path: //')

CONFIGURE_COMPILER_FLAGS += \
	CFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) -isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ $(MACCATALYST_ARCH) $(MACCATALYST_TARGET)" \
	CXXFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) -isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ -I./include/ $(MACCATALYST_ARCH) $(MACCATALYST_TARGET)" \
	LDFLAGS="-L$(XCODE_SDK)/usr/lib/ -isysroot $(XCODE_SDK) $(MACCATALYST_TARGET)" \
	CC="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang" \
	CXX="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"

CONFIGURE_ARGS += --host=$(MACCATALYST_ICU_HOST)

check-env:
	:
