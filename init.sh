git submodule update --init --recursive
./vcpkg/bootstrap-vcpkg.sh

# glew requires the following libraries from the system package manager
apt install libxmu-dev libxi-dev libgl-dev libxrandr-dev libxinerama-dev libxcursor-dev
# glfw dependices
apt install pkg-config libglu1-mesa-dev

./vcpkg/vcpkg install