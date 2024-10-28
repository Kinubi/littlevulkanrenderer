
-- LVR Dependencies

VULKAN_SDK = "/Users/barendbotha/VulkanSDK/1.3.296.0/macOS/"

IncludeDir = {}
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["includes"] = "include/"
--IncludeDir['glm'] = "%{VULKAN_SDK}/Include/glm"


LibraryDir = {}
--LibraryDir["glfw"]
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
-- LibraryDir["VulkanSDK_Debug"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["Vulkan"] = "vulkan"
Library["glfw"] = "glfw"
-- Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC"] = "shaderc_shared"
Library["SPIRV_Cross"] = "spirv-cross-core"
Library["SPIRV_Cross_GLSL"] = "spirv-cross-glsl"
Library["SPIRV_Tools_"] = "SPIRV-Tools"


