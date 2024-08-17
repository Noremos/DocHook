-- premake5.lua
workspace "Barcode Research"
   configurations { "Debug", "Release" }

project "DocHook"
   kind "ConsoleApp"
   language "C++"
   targetdir "dev/%{cfg.buildcfg}"
   -- outputdir "Build/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

   files {
      "Bind/**.h", "Bind/**.cpp",
      "frontend/**.h", "frontend/**.cpp",
      "backend/**.h", "backend/**.cpp",
      "side/**.h", "side/**.cpp",
      "side/imgui/backends/imgui_impl_glfw.cpp",
      "side/imgui/backends/imgui_impl_opengl3.cpp",
   }
   removefiles{
      "side/sol2"
   }

   includedirs { "side",
      "side/Barcode/PrjBarlib/modules",
      "side/imgui",
      "Bind"
   }

   links { "unofficial-sqlite3", "Lua", "jsoncpp", "glfw3"}

   cppdialect "C++20"
   enablemodules "On"

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"


   postbuildcommands { "postbuild.bat %{wks.location} %{cfg.buildcfg}" }