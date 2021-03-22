IOS_MIN_VERSION=10.0

### Delete this when https://github.com/dotnet/runtime/pull/49305 is merged ###
ifeq ($(TARGET_ARCHITECTURE),x64)
	IOS_ARCH=-arch x86_64
	IOS_SDK=iphonesimulator
	IOS_ICU_HOST=i686-apple-darwin11
endif
### Delete this when https://github.com/dotnet/runtime/pull/49305 is merged ###
ifeq ($(TARGET_ARCHITECTURE),x86)
	IOS_ARCH=-arch i386
	IOS_SDK=iphonesimulator
	IOS_ICU_HOST=i686-apple-darwin11
endif
ifeq ($(TARGET_ARCHITECTURE),arm64)
	IOS_ARCH=-arch arm64
	IOS_SDK=iphoneos
	IOS_ICU_HOST=arm-apple-darwin
endif
ifeq ($(TARGET_ARCHITECTURE),arm)
	IOS_ARCH=-arch armv7 -arch armv7s
	IOS_SDK=iphoneos
	IOS_ICU_HOST=arm-apple-darwin
endif

XCODE_DEVELOPER := $(shell xcode-select --print-path)
XCODE_SDK := $(shell xcodebuild -version -sdk $(IOS_SDK) | grep -E '^Path' | sed 's/Path: //')

CONFIGURE_COMPILER_FLAGS += \
	CFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) -isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ $(IOS_ARCH) -miphoneos-version-min=$(IOS_MIN_VERSION)" \
	CXXFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) -isysroot $(XCODE_SDK) -I$(XCODE_SDK)/usr/include/ -I./include/ $(IOS_ARCH) -miphoneos-version-min=$(IOS_MIN_VERSION)" \
	LDFLAGS="-L$(XCODE_SDK)/usr/lib/ -isysroot $(XCODE_SDK) -miphoneos-version-min=$(IOS_MIN_VERSION)" \
	CC="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang" \
	CXX="$(XCODE_DEVELOPER)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"

CONFIGURE_ARGS += --host=$(IOS_ICU_HOST)

check-env:
	:
