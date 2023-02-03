# TODO: add other host platform/arch combinations
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
UNAME_R := $(shell uname -r)

ifeq ($(UNAME_S),Linux)
	HOST_PLATFORM=x86_64-pc-linux-gnu
endif
ifeq ($(UNAME_S),Darwin)
	ifeq ($(UNAME_M),arm64)
		HOST_PLATFORM=arm64-apple-darwin$(UNAME_R)
	endif
	ifeq ($(UNAME_M),x86_64)
		HOST_PLATFORM=x86_64-apple-darwin$(UNAME_R)
	endif
endif
ifeq ($(WASM_ENABLE_THREADS),true)
	THREADS_FLAG="-pthread"
endif

CONFIGURE_COMPILER_FLAGS += \
	CFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(THREADS_FLAG) $(ICU_DEFINES)" \
	CXXFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(THREADS_FLAG) $(ICU_DEFINES)" \
	CC="$(WASI_SDK_PATH)/bin/clang --sysroot=$(WASI_SDK_PATH)/share/wasi-sysroot" \
	CXX="$(WASI_SDK_PATH)/bin/clang++ --sysroot=$(WASI_SDK_PATH)/share/wasi-sysroot" \
	--host=$(HOST_PLATFORM) --build=wasm32 \
	RANLIB="$(WASI_SDK_PATH)/bin/llvm-ranlib" 

check-env:
	:
