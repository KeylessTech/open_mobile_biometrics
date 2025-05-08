#!/bin/bash

set -e

BUILD_NATIVE=false
BUILD_ANDROID=false
BUILD_IOS=false

# Parse command-line arguments
if [ $# -eq 0 ]; then
  BUILD_NATIVE=true
  BUILD_ANDROID=true
  BUILD_IOS=true
else
  for arg in "$@"; do
    case $arg in
      --native) BUILD_NATIVE=true ;;
      --android) BUILD_ANDROID=true ;;
      --ios) BUILD_IOS=true ;;
      *) echo "Unknown option $arg"; exit 1 ;;
    esac
  done
fi

# Native build (macOS or Linux)
if [ "$BUILD_NATIVE" = true ]; then
  echo "🛠 Building for native host..."
  rm -rf build-native
  cmake -S . -B build-native -DCMAKE_OSX_ARCHITECTURES=arm64
  cmake --build build-native --parallel
fi

# Android build for every ABI, all under build-android/*
if [ "$BUILD_ANDROID" = true ]; then
  ANDROID_API_LEVEL=21
  ANDROID_NDK="${ANDROID_NDK:-$HOME/Android/Sdk/ndk-bundle}"
  TOOLCHAIN="$ANDROID_NDK/build/cmake/android.toolchain.cmake"
  ABIS=(armeabi-v7a arm64-v8a x86 x86_64)

  # clear out old
  rm -rf build-android
  mkdir -p build-android

  for ABI in "${ABIS[@]}"; do
    BUILD_DIR=build-android/$ABI
    echo "🤖 Building Android for ABI: $ABI → $BUILD_DIR"
    cmake -S . -B "$BUILD_DIR" \
      -DANDROID_ABI="$ABI" \
      -DANDROID_PLATFORM=android-$ANDROID_API_LEVEL \
      -DANDROID_NDK="$ANDROID_NDK" \
      -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"
    cmake --build "$BUILD_DIR" --parallel
  done
fi

# iOS build
if [ "$BUILD_IOS" = true ]; then
  echo "📱 Building for iOS..."
  rm -rf build-ios
  cmake -S . -B build-ios -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
  cmake --build build-ios --config Release --parallel
fi

echo "✅ All builds complete."
