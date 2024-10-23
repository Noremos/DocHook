#!/bin/bash
# export LDFLAGS="-L/opt/homebrew/opt/llvm/lib -fdiagnostics-color"
export CPPFLAGS="-fdiagnostics-color"
export LDFLAGS="-fdiagnostics-color"
export CC=clang
export CXX=clang++
export CXXFLAGS='-fdiagnostics-color'
export CFLAGS='-fdiagnostics-color'
# export PATH="/opt/homebrew/opt/llvm/bin:$PATH"

export EXTRA='-DBUILD_TEST:BOOL=false'
export OUT='Build/Temp'

if [ -z "$1" ]; then
    build_type="Debug";
else
    build_type=$1;
fi

sh ./build.sh $build_type "Unix Makefiles"
