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

# Проверяем передан ли аргумент
if [ -z "$1" ]; then
    # Если аргумент не передан, задаем значение по умолчанию
    build_type="Debug"
else
    if [ "$1" == "Tests" ]; then
        EXTRA='-DBUILD_TEST:BOOL=true'
        OUT='Tests/Temp'
        echo "RUN TESTS..."
    else
        echo "RUN $1 Build..."
    fi

    # Иначе, используем переданный аргумент
    build_type="$1"
fi

export OUT="Build/Temp/${build_type}"
# export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
# export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"

cmake -B "${OUT}" -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja -DCMAKE_BUILD_TYPE="${build_type}" "${EXTRA}"
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