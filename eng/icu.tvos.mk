TVOS_MIN_VERSION=10.0

ifeq ($(TARGET_ARCHITECTURE),arm64)
	TVOS_ARCH=arm64
	TVOS_SDK=appletvos
	TVOS_ICU_HOST=aarch64-apple-darwin
endif

XCODE_DEVELOPER := $(shell xcode-select --print-path)
XCODE_SDK := $(shell xcodebuild -version -sdk $(TVOS_SDK) | grep -E '^Path' | sed 's/Path: //')

CONFIGURE_COMPILER_FLAGS += \
	CFLAGS="-fembed-bitcode -Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) -isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ -arch $(TVOS_ARCH) -mappletvos-version-min=$(TVOS_MIN_VERSION)" \
	CXXFLAGS="-fembed-bitcode -Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) -isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ -I./include/ -arch $(TVOS_ARCH) -mappletvos-version-min=$(TVOS_MIN_VERSION)" \
	LDFLAGS="-fembed-bitcode -L$(XCODE_SDK)/usr/lib/ -isysroot $(XCODE_SDK) -mappletvos-version-min=$(TVOS_MIN_VERSION)" \
	CC="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang" \
	CXX="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"

CONFIGURE_ARGS += --host=$(TVOS_ICU_HOST)

check-env:
	:
