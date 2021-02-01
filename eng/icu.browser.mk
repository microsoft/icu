ENV_INIT_SCRIPT = source $(EMSDK_PATH)/emsdk_env.sh &&
ENV_CONFIGURE_WRAPPER = emconfigure

CONFIGURE_COMPILER_FLAGS += \
	CFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES)" \
	CXXFLAGS="-Oz -fno-exceptions -Wno-sign-compare $(ICU_DEFINES)"

check-env:
	@if [ -z "$(EMSDK_PATH)" ]; then echo "The EMSDK_PATH environment variable needs to set to the location of the emscripten SDK."; exit 1; fi

