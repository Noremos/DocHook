git submodule update --init --recursive
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg.exe install
D:\WSpace\cmake\bin\cmake.exe -B Build/Win -S . -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TEST:BOOL=false