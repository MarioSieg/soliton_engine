VULKAN_SDK="/Users/mario/VulkanSDK/1.3.296.0/macOS" # Change this to your SDK path
export VULKAN_SDK
PATH="$PATH:$VULKAN_SDK/bin"
export PATH
DYLD_LIBRARY_PATH="$VULKAN_SDK/lib:${DYLD_LIBRARY_PATH:-}"
export DYLD_LIBRARY_PATH
VK_ADD_LAYER_PATH="$VULKAN_SDK/share/vulkan/explicit_layer.d"
export VK_ADD_LAYER_PATH
VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"
export VK_ICD_FILENAMES
VK_DRIVER_FILES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"
export VK_DRIVER_FILES
