ANDROID_API=21

ifeq ($(TARGET_ARCHITECTURE),x64)
	ANDROID_FLAGS=-m64
	ANDROID_PLATFORM=x86_64
	ANDROID_CX_SUFFIX=android$(ANDROID_API)
	ANDROID_AR=llvm-ar
	ANDROID_RANLIB=llvm-ranlib
endif
ifeq ($(TARGET_ARCHITECTURE),x86)
	ANDROID_FLAGS=-m32
	ANDROID_PLATFORM=i686
	ANDROID_CX_SUFFIX=android$(ANDROID_API)
	ANDROID_AR=llvm-ar
	ANDROID_RANLIB=llvm-ranlib
endif
ifeq ($(TARGET_ARCHITECTURE),arm64)
	ANDROID_FLAGS=-fpic
	ANDROID_PLATFORM=aarch64
	ANDROID_CX_SUFFIX=android$(ANDROID_API)
	ANDROID_AR=llvm-ar
	ANDROID_RANLIB=llvm-ranlib
endif
ifeq ($(TARGET_ARCHITECTURE),arm)
	ANDROID_FLAGS=-march=armv7-a -mtune=cortex-a8 -mfpu=vfp -mfloat-abi=softfp -fpic
	ANDROID_PLATFORM=armv7a
	ANDROID_CX_SUFFIX=androideabi$(ANDROID_API)
	ANDROID_AR=llvm-ar
	ANDROID_RANLIB=llvm-ranlib
endif

# TODO: add other host platform/arch combinations
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_S),Linux)
	HOST_PLATFORM=linux-x86_64
endif
ifeq ($(UNAME_S),Darwin)
	ifeq ($(UNAME_M),arm64)
		HOST_PLATFORM=darwin-arm64
	endif
	ifeq ($(UNAME_M),x86_64)
		HOST_PLATFORM=darwin-x86_64
	endif
endif

CONFIGURE_COMPILER_FLAGS += \
	CFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) $(ANDROID_FLAGS) -I$(ANDROID_NDK_ROOT)/sysroot/usr/include/$(ANDROID_PLATFORM)-linux-android" \
	CXXFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES) $(ANDROID_FLAGS) -I$(ANDROID_NDK_ROOT)/sysroot/usr/include/$(ANDROID_PLATFORM)-linux-android" \
	CC="$(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/$(HOST_PLATFORM)/bin/$(ANDROID_PLATFORM)-linux-$(ANDROID_CX_SUFFIX)-clang" \
	CXX="$(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/$(HOST_PLATFORM)/bin/$(ANDROID_PLATFORM)-linux-$(ANDROID_CX_SUFFIX)-clang++" \
	AR="$(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/$(HOST_PLATFORM)/bin/$(ANDROID_AR)" \
	RANLIB="$(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/$(HOST_PLATFORM)/bin/$(ANDROID_RANLIB)"

CONFIGURE_ARGS += --host=$(ANDROID_PLATFORM)-linux-android

check-env:
	@if [ -z "$(ANDROID_NDK_ROOT)" ]; then echo "The ANDROID_NDK_ROOT environment variable needs to set to the location of the Android NDK."; exit 1; fi
