include "dependencies.lua"

workspace "LVR"
	configurations { "Debug",  "Release"}
    conformancemode "On"


project "LVR"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "Off"

	

	flags { "MultiProcessorCompile" }

	-- NOTE(Peter): Don't remove this. Please never use Annex K functions ("secure", e.g _s) functions.


	files {
		"src/**.h",
		"src/**.c",
		"src/**.hpp",
		"src/**.cpp",
	}

	
	includedirs { "src",
	 			"%{IncludeDir.includes}",

			}

		removefiles { "src/platform/linux/**.h","src/platform/linux/**.cpp" }

	defines {
		"GLM_FORCE_RADIANS",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_ENABLE_EXPERIMENTAL"
	}

	links
	{
		"%{Library.ShaderC}",
		"%{Library.SPIRV_Cross}",
		"%{Library.SPIRV_Cross_GLSL}",
		"%{Library.Vulkan}",
		"%{Library.glfw}",
	}


    filter "action:vs*"
        linkoptions { "/ignore:4099" } -- NOTE(Peter): Disable no PDB found warning
        disablewarnings { "4068" } -- Disable "Unknown #pragma mark warning"



	filter "configurations:Debug"
		optimize "Off"
		symbols "On"





	filter "configurations:Release"
		optimize "Full"
		symbols "Off"
		vectorextensions "AVX2"
		isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }




	filter "system:windows"
		buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

	filter "system:linux"

		architecture "x86_64"
		defines { "HZ_PLATFORM_LINUX"}
		links { "dw", "dl", "unwind", "pthread", "vulkan","glfw", "wayland-client" }
		buildoptions { "-march=x86-64-v3" }

	filter "system:macosx"
		architecture "aarch64"
		defines { "LVR_PLATFORM_MACOS"}
		links {   "vulkan","glfw" }

	
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")





