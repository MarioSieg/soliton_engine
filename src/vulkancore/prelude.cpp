// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "prelude.hpp"

namespace vkb {
    auto get_error_string(vk::Result result) noexcept -> const char* {
        switch (static_cast<VkResult>(result)) {
#define str(r) case VK_ ##r: return #r
            str(SUCCESS);
            str(NOT_READY);
            str(TIMEOUT);
            str(EVENT_SET);
            str(EVENT_RESET);
            str(INCOMPLETE);
            str(ERROR_OUT_OF_HOST_MEMORY);
            str(ERROR_OUT_OF_DEVICE_MEMORY);
            str(ERROR_INITIALIZATION_FAILED);
            str(ERROR_DEVICE_LOST);
            str(ERROR_MEMORY_MAP_FAILED);
            str(ERROR_LAYER_NOT_PRESENT);
            str(ERROR_EXTENSION_NOT_PRESENT);
            str(ERROR_FEATURE_NOT_PRESENT);
            str(ERROR_INCOMPATIBLE_DRIVER);
            str(ERROR_TOO_MANY_OBJECTS);
            str(ERROR_FORMAT_NOT_SUPPORTED);
            str(ERROR_FRAGMENTED_POOL);
            str(ERROR_UNKNOWN);
            str(ERROR_OUT_OF_POOL_MEMORY);
            str(ERROR_INVALID_EXTERNAL_HANDLE);
            str(ERROR_FRAGMENTATION);
            str(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
            str(PIPELINE_COMPILE_REQUIRED);
            str(ERROR_SURFACE_LOST_KHR);
            str(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            str(SUBOPTIMAL_KHR);
            str(ERROR_OUT_OF_DATE_KHR);
            str(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            str(ERROR_VALIDATION_FAILED_EXT);
            str(ERROR_INVALID_SHADER_NV);
            str(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR);
            str(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR);
            str(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR);
            str(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR);
            str(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR);
            str(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR);
            str(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
            str(ERROR_NOT_PERMITTED_KHR);
            str(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
            str(THREAD_IDLE_KHR);
            str(THREAD_DONE_KHR);
            str(OPERATION_DEFERRED_KHR);
            str(OPERATION_NOT_DEFERRED_KHR);
            str(ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR);
            str(ERROR_COMPRESSION_EXHAUSTED_EXT);
            str(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
            // str(ERROR_OUT_OF_POOL_MEMORY_KHR);
            // str(ERROR_INVALID_EXTERNAL_HANDLE_KHR);
            // str(ERROR_FRAGMENTATION_EXT);
            // str(ERROR_NOT_PERMITTED_EXT);
            // str(ERROR_INVALID_DEVICE_ADDRESS_EXT);
            // str(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR);
            // str(PIPELINE_COMPILE_REQUIRED_EXT);
            // str(ERROR_PIPELINE_COMPILE_REQUIRED_EXT);
#undef str
            default: return "UNKNOWN_ERROR";
        }
    }

    auto physical_device_type_string(vk::PhysicalDeviceType type) noexcept -> const char* {
        switch (static_cast<VkPhysicalDeviceType>(type)) {
#define str(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
            str(OTHER);
            str(INTEGRATED_GPU);
            str(DISCRETE_GPU);
            str(VIRTUAL_GPU);
            str(CPU);
#undef str
            default: return "UNKNOWN_DEVICE_TYPE";
        }
    }

    auto dump_physical_device_props(const vk::PhysicalDeviceProperties &props) -> void {
        #define dump(x) log_info(#x ": {}", props.limits.x)
#define dump_flags(x) log_info(#x ": {:#x}", static_cast<std::uint32_t>(props.limits.x))

        dump(maxImageDimension1D                            );
        dump(maxImageDimension2D                            );
        dump(maxImageDimension3D                            );
        dump(maxImageDimensionCube                          );
        dump(maxImageArrayLayers                            );
        dump(maxTexelBufferElements                         );
        dump(maxUniformBufferRange                          );
        dump(maxStorageBufferRange                          );
        dump(maxPushConstantsSize                           );
        dump(maxMemoryAllocationCount                       );
        dump(maxSamplerAllocationCount                      );
        dump(bufferImageGranularity                         );
        dump(sparseAddressSpaceSize                         );
        dump(maxBoundDescriptorSets                         );
        dump(maxPerStageDescriptorSamplers                  );
        dump(maxPerStageDescriptorUniformBuffers            );
        dump(maxPerStageDescriptorStorageBuffers            );
        dump(maxPerStageDescriptorSampledImages             );
        dump(maxPerStageDescriptorStorageImages             );
        dump(maxPerStageDescriptorInputAttachments          );
        dump(maxPerStageResources                           );
        dump(maxDescriptorSetSamplers                       );
        dump(maxDescriptorSetUniformBuffers                 );
        dump(maxDescriptorSetUniformBuffersDynamic          );
        dump(maxDescriptorSetStorageBuffers                 );
        dump(maxDescriptorSetStorageBuffersDynamic          );
        dump(maxDescriptorSetSampledImages                  );
        dump(maxDescriptorSetStorageImages                  );
        dump(maxDescriptorSetInputAttachments               );
        dump(maxVertexInputAttributes                       );
        dump(maxVertexInputBindings                         );
        dump(maxVertexInputAttributeOffset                  );
        dump(maxVertexInputBindingStride                    );
        dump(maxVertexOutputComponents                      );
        dump(maxTessellationGenerationLevel                 );
        dump(maxTessellationPatchSize                       );
        dump(maxTessellationControlPerVertexInputComponents );
        dump(maxTessellationControlPerVertexOutputComponents);
        dump(maxTessellationControlPerPatchOutputComponents );
        dump(maxTessellationControlTotalOutputComponents    );
        dump(maxTessellationEvaluationInputComponents       );
        dump(maxTessellationEvaluationOutputComponents      );
        dump(maxGeometryShaderInvocations                   );
        dump(maxGeometryInputComponents                     );
        dump(maxGeometryOutputComponents                    );
        dump(maxGeometryOutputVertices                      );
        dump(maxGeometryTotalOutputComponents               );
        dump(maxFragmentInputComponents                     );
        dump(maxFragmentOutputAttachments                   );
        dump(maxFragmentDualSrcAttachments                  );
        dump(maxFragmentCombinedOutputResources             );
        dump(maxComputeSharedMemorySize                     );
        log_info("maxComputeWorkGroupCount: [{}, {}, {}]", props.limits.maxComputeWorkGroupCount[0],
            props.limits.maxComputeWorkGroupCount[1], props.limits.maxComputeWorkGroupCount[2]);
        dump(maxComputeWorkGroupInvocations                 );
        log_info("maxComputeWorkGroupSize: [{}, {}, {}]", props.limits.maxComputeWorkGroupSize[0],
            props.limits.maxComputeWorkGroupSize[1], props.limits.maxComputeWorkGroupSize[2]);
        dump(subPixelPrecisionBits                          );
        dump(subTexelPrecisionBits                          );
        dump(mipmapPrecisionBits                            );
        dump(maxDrawIndexedIndexValue                       );
        dump(maxDrawIndirectCount                           );
        dump(maxSamplerLodBias                              );
        dump(maxSamplerAnisotropy                           );
        dump(maxViewports                                   );
        log_info("maxViewportDimensions: [{}, {}]", props.limits.maxViewportDimensions[0],
            props.limits.maxViewportDimensions[1]);
        log_info("viewportBoundsRange: [{}, {}]", props.limits.viewportBoundsRange[0], props.limits.viewportBoundsRange[1]);
        dump(viewportSubPixelBits                           );
        dump(minMemoryMapAlignment                          );
        dump(minTexelBufferOffsetAlignment                  );
        dump(minUniformBufferOffsetAlignment                );
        dump(minStorageBufferOffsetAlignment                );
        dump(minTexelOffset                                 );
        dump(maxTexelOffset                                 );
        dump(minTexelGatherOffset                           );
        dump(maxTexelGatherOffset                           );
        dump(minInterpolationOffset                         );
        dump(maxInterpolationOffset                         );
        dump(subPixelInterpolationOffsetBits                );
        dump(maxFramebufferWidth                            );
        dump(maxFramebufferHeight                           );
        dump(maxFramebufferLayers                           );
        dump_flags(framebufferColorSampleCounts                   );
        dump_flags(framebufferDepthSampleCounts                   );
        dump_flags(framebufferStencilSampleCounts                 );
        dump_flags(framebufferNoAttachmentsSampleCounts           );
        dump(maxColorAttachments                            );
        dump_flags(sampledImageColorSampleCounts                  );
        dump_flags(sampledImageIntegerSampleCounts                );
        dump_flags(sampledImageDepthSampleCounts                  );
        dump_flags(sampledImageStencilSampleCounts                );
        dump_flags(storageImageSampleCounts                       );
        dump(maxSampleMaskWords                             );
        dump(timestampComputeAndGraphics                    );
        dump(timestampPeriod                                );
        dump(maxClipDistances                               );
        dump(maxCullDistances                               );
        dump(maxCombinedClipAndCullDistances                );
        dump(discreteQueuePriorities                        );
        log_info("pointSizeRange: [{}, {}]", props.limits.pointSizeRange[0], props.limits.pointSizeRange[1]);
        log_info("lineWidthRange: [{}, {}]", props.limits.lineWidthRange[0], props.limits.lineWidthRange[1]);
        dump(pointSizeGranularity                           );
        dump(lineWidthGranularity                           );
        dump(strictLines                                    );
        dump(standardSampleLocations                        );
        dump(optimalBufferCopyOffsetAlignment               );
        dump(optimalBufferCopyRowPitchAlignment             );
        dump(nonCoherentAtomSize                            );

#undef dump_flags
#undef dump
    }
}
