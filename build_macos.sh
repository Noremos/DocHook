#!/bin/bash
# export LDFLAGS="-L/opt/homebrew/opt/llvm/lib -fdiagnostics-color"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include '-fdiagnostics-color"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib -fdiagnostics-color -L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++"
export CC=/opt/homebrew/opt/llvm/bin/clang
export CXX=/opt/homebrew/opt/llvm/bin/clang++
export CXXFLAGS='-fdiagnostics-color'
export CFLAGS='-fdiagnostics-color'
# export PATH="/opt/homebrew/opt/llvm/bin:$PATH"

export EXTRA='-DBUILD_TEST:BOOL=false'
export OUT='Build/Temp'

if [ -z "$1" ]; then
    build_type = "Debug";
else
    build_type = $1;
fi

./build.sh build_type Ninja