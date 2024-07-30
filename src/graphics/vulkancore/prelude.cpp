// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "prelude.hpp"

namespace lu::vkb {
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
