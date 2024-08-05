xcopy "%1vcpkg_installed\x64-windows\bin" "%1Build\Release" /d /y /s
xcopy "%1vcpkg_installed\x64-windows\debug\bin" "%1Build\Debug" /d /y /s
xcopy "%1Build\Resurses" "%1Build\%2" /d /y /s
