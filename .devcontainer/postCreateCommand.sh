EMSDK_PATH=$PWD/artifacts/emsdk
rm -rf $EMSDK_PATH
git clone https://github.com/emscripten-core/emsdk.git $EMSDK_PATH
EMSCRIPTEN_VERSION="`cat ./.devcontainer/emscripten-version.txt 2>&1`"
cd $EMSDK_PATH && ./emsdk install $EMSCRIPTEN_VERSION
cd $EMSDK_PATH && ./emsdk activate $EMSCRIPTEN_VERSION
# ready to build, e.g.:
# ./build.sh /p:TargetOS=Browser /p:TargetArchitecture=wasm /p:IcuTracing=true