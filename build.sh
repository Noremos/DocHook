#!/bin/bash

if [ -z "$1" ]; then
    echo "Build type arg (1) is missing"
    exit 1
fi

if [ -z "$2" ]; then
    echo "Build system arg (2) is missing"
    exit 1
fi

build_type="$1"

if [ $build_type = "Tests" ]; then
    EXTRA='-DBUILD_TEST:BOOL=true'
    OUT='Tests/Temp'
    echo "RUN TESTS..."
else
    echo "RUN $1 Build..."
fi



export OUT="Build/Temp/${build_type}"
# export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
# export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"

cmake -B "${OUT}" -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -G "$2" -DCMAKE_BUILD_TYPE="${build_type}" "${EXTRA}"
cmake --build "${OUT}"
retVal=$?
if [ $retVal -ne 0 ]; then
    echo "Error"
    exit $retVal
fi

mv "${OUT}/compile_commands.json" ./Build/compile_commands.json

if [ "${build_type}" = "Tests" ]; then
    ./Build/Temp/SatTests
fi

echo Done
# cmake -B Build -S . -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TEST:BOOL=false
# cmake --build Build


# To use the bundled libc++ please add the following LDFLAGS
# LDFLAGS="-L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++"

# llvm is keg-only, which means it was not symlinked into /opt/homebrew,
# because macOS already provides this software and installing another version in
# parallel can cause all kinds of trouble.

# If you need to have llvm first in your PATH, run:
#   echo 'export PATH="/opt/homebrew/opt/llvm/bin:$PATH"' >> ~/.zshrc

# For compilers to find llvm you may need to set:
#   export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
#   export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"