IOS_MIN_VERSION=11.0

ifeq ($(TARGET_ARCHITECTURE),x64)
	IOS_ARCH=x86_64
	IOS_SDK=iphonesimulator
	IOS_ICU_HOST=x86_64-apple-darwin
endif
ifeq ($(TARGET_ARCHITECTURE),arm64)
	IOS_ARCH=arm64
	IOS_SDK=iphonesimulator
	IOS_ICU_HOST=aarch64-apple-darwin
endif

XCODE_DEVELOPER := $(shell xcode-select --print-path)
XCODE_SDK := $(shell xcodebuild -version -sdk $(IOS_SDK) | grep -E '^Path' | sed 's/Path: //')

CONFIGURE_COMPILER_FLAGS += \
	CFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) -isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ -arch $(IOS_ARCH) -miphonesimulator-version-min=$(IOS_MIN_VERSION)" \
	CXXFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) -isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ -I./include/ -arch $(IOS_ARCH) -miphonesimulator-version-min=$(IOS_MIN_VERSION)" \
	LDFLAGS="-L$(XCODE_SDK)/usr/lib/ -isysroot $(XCODE_SDK) -miphoneos-version-min=$(IOS_MIN_VERSION)" \
	CC="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang" \
	CXX="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"

CONFIGURE_ARGS += --host=$(IOS_ICU_HOST)

check-env:
	:
