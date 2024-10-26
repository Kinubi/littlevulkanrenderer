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
				--"%{IncludeDir.VulkanSDK}"
			}

	defines {
		"GLM_FORCE_RADIANS",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_ENABLE_EXPERIMENTAL"
	}

    filter "action:vs*"
        linkoptions { "/ignore:4099" } -- NOTE(Peter): Disable no PDB found warning
        disablewarnings { "4068" } -- Disable "Unknown #pragma mark warning"

	filter "language:C++ or language:C"
		architecture "x86_64"

	filter "configurations:Debug"
		optimize "Off"
		symbols "On"


		links
		{
			"%{Library.ShaderC}",
			"%{Library.SPIRV_Cross}",
			"%{Library.SPIRV_Cross_GLSL}"
		}



	filter "configurations:Release"
		optimize "Full"
		symbols "Off"
		vectorextensions "AVX2"
		isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }

		links
		{
			"%{Library.ShaderC}",
			"%{Library.SPIRV_Cross}",
			"%{Library.SPIRV_Cross_GLSL}"
		}


	filter "system:windows"
		buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

	filter "system:linux"
		defines { "HZ_PLATFORM_LINUX", "__EMULATE_UUID", "BACKWARD_HAS_DW", "BACKWARD_HAS_LIBUNWIND" }
		links { "dw", "dl", "unwind", "pthread", "vulkan","glfw", "wayland-client" }
		buildoptions { "-march=x86-64-v3" }
	
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")





