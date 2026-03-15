VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}

IncludeDir["GLFW"] = "%{wks.location}/external/GLFW/include"
IncludeDir["glm"] = "%{wks.location}/external/glm"
IncludeDir["portaudio"] = "%{wks.location}/external/portaudio/include"
IncludeDir["imgui"] = "%{wks.location}/external/imgui"
if os.host() == "windows" then
	IncludeDir["Vulkan"] = "%{VULKAN_SDK}/Include"
elseif os.host() == "macosx" then
	IncludeDir["Vulkan"] = "%{VULKAN_SDK}/include"
end

LibraryDir = {}

if os.host() == "windows" then
	LibraryDir["Vulkan"] = "%{VULKAN_SDK}/Lib"
elseif os.host() == "macosx" then
    local vulkan_sdk = os.getenv("VULKAN_SDK")
    if vulkan_sdk:sub(1, 1) == "~" then
        vulkan_sdk = os.getenv("HOME") .. vulkan_sdk:sub(2)
    end

    LibraryDir["Vulkan"] = vulkan_sdk .. "/lib"
end

Library = {}

if os.host() == "windows" then
	Library["Vulkan"] = "%{LibraryDir.Vulkan}/vulkan-1.lib"

	Library["ShaderC_Debug"] = "%{LibraryDir.Vulkan}/shaderc_sharedd.lib"
	Library["SPIRV_Cross_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-cored.lib"
	Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-glsld.lib"
	Library["SPIRV_Cross_HLSL_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-hlsld.lib"
	Library["SPIRV_Cross_MSL_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-msld.lib"

	Library["ShaderC_Release"] = "%{LibraryDir.Vulkan}/shaderc_shared.lib"
	Library["SPIRV_Cross_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-core.lib"
	Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-glsl.lib"
	Library["SPIRV_Cross_HLSL_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-hlsl.lib"
	Library["SPIRV_Cross_MSL_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-msl.lib"
elseif os.host() == "macosx" then
	Library["Vulkan"] = "vulkan"

	Library["ShaderC_Debug"] = "%{LibraryDir.Vulkan}/libshaderc.a"
	Library["SPIRV_Cross_Debug"] = "%{LibraryDir.Vulkan}/libspirv-cross-core.a"
	Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.Vulkan}/libspirv-cross-glsl.a"
	Library["SPIRV_Cross_HLSL_Debug"] = "%{LibraryDir.Vulkan}/libspirv-cross-hlsl.a"
	Library["SPIRV_Cross_MSL_Debug"] = "%{LibraryDir.Vulkan}/libspirv-cross-msl.a"

	Library["ShaderC_Release"] = "%{LibraryDir.Vulkan}/libshaderc.a"
	Library["SPIRV_Cross_Release"] = "%{LibraryDir.Vulkan}/libspirv-cross-core.a"
	Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.Vulkan}/libspirv-cross-glsl.a"
	Library["SPIRV_Cross_HLSL_Release"] = "%{LibraryDir.Vulkan}/libspirv-cross-hlsl.a"
	Library["SPIRV_Cross_MSL_Release"] = "%{LibraryDir.Vulkan}/libspirv-cross-msl.a"
end

workspace "wire"
    architecture "x86_64"
    startproject "fibre"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

    flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
    include "external/GLFW"
	include "external/portaudio"
	include "external/imgui"
group ""

project "wire"
    location "wire"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.location}/src/**.h",
		"%{prj.location}/src/**.cpp",
		"external/stb_image/stb_image.h",
		"external/stb_image/stb_image.cpp",
		"external/tinyobjloader/tiny_obj_loader.h",
		"external/tinyobjloader/tiny_obj_loader.cpp",
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "GLFW_INCLUDE_NONE",
        "NOMINMAX",
		"_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING"
    }

    includedirs
	{
		"%{prj.location}/src",
	}

    externalincludedirs
	{
		"external/stb_image",
		"external/tinyobjloader",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.portaudio}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.imgui}",
        "%{IncludeDir.Vulkan}"
    }

    links
    {
        "GLFW",
		"portaudio",
		"imgui",
        "%{Library.Vulkan}"
    }

    filter "system:windows"
		systemversion "latest"

		links
		{
			"dwmapi.lib"
		}

	filter "system:macosx"
		files
		{
			"%{prj.location}/src/**.mm"
		}

		libdirs
		{
			"%{LibraryDir.Vulkan}"
		}

		links
		{
			"vulkan"
		}

    filter "configurations:Debug"
		defines "WR_DEBUG"
		runtime "Debug"
		symbols "on"

		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",
			"%{Library.SPIRV_Cross_HLSL_Debug}",
			"%{Library.SPIRV_Cross_MSL_Debug}"
		}

    filter "configurations:Release"
		defines "WR_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
			"%{Library.SPIRV_Cross_HLSL_Release}",
			"%{Library.SPIRV_Cross_MSL_Release}",
		}

    filter "configurations:Dist"
		defines "WR_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
			"%{Library.SPIRV_Cross_HLSL_Release}",
			"%{Library.SPIRV_Cross_MSL_Release}",
		}

project "fibre"
    location "fibre"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.location}/src/**.h",
		"%{prj.location}/src/**.cpp",
	}

	includedirs
	{
		"%{wks.location}/Wire/src"
	}

    externalincludedirs
	{
		"external/tinyobjloader",
		"%{IncludeDir.imgui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.GLFW}"
    }

    links
    {
        "wire",
		"GLFW",
		"portaudio",
		"imgui",
        "%{Library.Vulkan}"
    }

    filter "system:windows"
		systemversion "latest"

		defines { "NOMINMAX" }

	filter "system:macosx"
		libdirs
		{
			"%{VULKAN_SDK}/lib",
			"%{LibraryDir.Vulkan}"
		}

		links
		{
			"shaderc",
			"shaderc_util",
			"glslang",
			"vulkan",
			"CoreFoundation.framework",
			"CoreGraphics.framework",
			"IOKit.framework",
			"AppKit.framework",
		}

		buildoptions { "-fexperimental-library" } -- for std::execution::par
		linkoptions { "-stdlib=libc++", "-fexperimental-library" }

	filter "action:xcode4"
		xcodebuildresources
		{
			"%{prj.location}/shaders/**",
			"%{prj.location}/fonts/**",
			"%{prj.location}/models/**"
		}

		xcodebuildsettings
		{
			["LD_RUNPATH_SEARCH_PATHS"] = LibraryDir["Vulkan"]
		}

	filter "configurations:Debug"
		kind "ConsoleApp"
		defines "WR_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		kind "ConsoleApp"
		defines "WR_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		kind "WindowedApp"
		defines "WR_DIST"
		runtime "Release"
		optimize "on"
