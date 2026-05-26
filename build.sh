#!/bin/bash
# Direct build script — no CMake needed
set -e

SDK="$HOME/Developer/AfterEffectsSDK/Examples"
REZ="/Applications/Xcode.app/Contents/Developer/usr/bin/Rez"
OUT="AE3DFlag.aex"

echo "==> Compiling C++ sources..."
clang++ \
  -dynamiclib \
  -std=c++17 \
  -arch arm64 -arch x86_64 \
  -mmacosx-version-min=11.0 \
  -fvisibility=hidden \
  -ffast-math -O2 \
  -I"$SDK/Headers" \
  -I"$SDK/Headers/SP" \
  -I"$SDK/Resources" \
  -I"$SDK/Util" \
  -Isrc \
  -Xlinker -exported_symbol -Xlinker _EffectMain \
  -o AE3DFlag.dylib \
  src/AE3DFlag.cpp \
  src/FlagRenderer.cpp \
  "$SDK/Util/AEGP_SuiteHandler.cpp" \
  "$SDK/Util/AEFX_SuiteHelper.c"

echo "==> Creating bundle structure..."
mkdir -p "$OUT/Contents/MacOS"
mkdir -p "$OUT/Contents/Resources"
cp AE3DFlag.dylib "$OUT/Contents/MacOS/AE3DFlag"
cp res/Info.plist "$OUT/Contents/"

echo "==> Compiling PiPL resource..."
"$REZ" \
  -i "$SDK/Headers" \
  -i "$SDK/Headers/SP" \
  -i "$SDK/Resources" \
  -i "$SDK/Util" \
  -d AE_OS_MAC=1 \
  -d AE_PROC_ARM64=1 \
  -o "$OUT/Contents/Resources/AE3DFlag.rsrc" \
  res/AE3DFlag.r

echo ""
echo "==> Done: $OUT"
echo ""
echo "Install with:"
echo "  cp -R $OUT ~/Library/Application\ Support/Adobe/Common/Plug-ins/7.0/MediaCore/"
