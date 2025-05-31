#!/bin/bash

rm -rf build && mkdir build

ABIS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

for ABI in "${ABIS[@]}"; do
  BUILD_DIR=build/$ABI

  cmake -B $BUILD_DIR \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release

  cmake --build $BUILD_DIR
done

mkdir -p sharedLibPreCompiled/lib
mkdir -p sharedLibPreCompiled/lib/armeabi-v7a
mkdir -p sharedLibPreCompiled/lib/arm64-v8a
mkdir -p sharedLibPreCompiled/lib/x86
mkdir -p sharedLibPreCompiled/lib/x86_64
cp build/armeabi-v7a/libhack.so sharedLibPreCompiled/lib/armeabi-v7a/libhack.so
cp build/arm64-v8a/libhack.so sharedLibPreCompiled/lib/arm64-v8a/libhack.so
cp build/x86/libhack.so sharedLibPreCompiled/lib/x86/libhack.so
cp build/x86_64/libhack.so sharedLibPreCompiled/lib/x86_64/libhack.so