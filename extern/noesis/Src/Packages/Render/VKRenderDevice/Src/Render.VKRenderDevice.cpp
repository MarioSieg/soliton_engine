////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Package.h>
#include <NsRender/Texture.h>
#include <NsRender/VKFactory.h>

#include "VKRenderDevice.h"


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderDevice> VKFactory::CreateDevice(bool sRGB, const InstanceInfo& info)
{
    return *new VKRenderDevice(sRGB, info);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKFactory::SetCommandBuffer(RenderDevice* device_, const RecordingInfo& info)
{
    VKRenderDevice* device = (VKRenderDevice*)device_;
    device->SetCommandBuffer(info);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKFactory::SetRenderPass(Noesis::RenderDevice* device_, VkRenderPass renderPass, uint32_t sampleCount)
{
    VKRenderDevice* device = (VKRenderDevice*)device_;
    device->SetRenderPass(renderPass, (VkSampleCountFlagBits)sampleCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKFactory::WarmUpRenderPass(RenderDevice* device_, VkRenderPass renderPass, uint32_t sampleCount)
{
    VKRenderDevice* device = (VKRenderDevice*)device_;
    device->WarmUpRenderPass(renderPass, (VkSampleCountFlagBits)sampleCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> VKFactory::WrapTexture(VkImage image, uint32_t width, uint32_t height, uint32_t levels,
    VkFormat format, VkImageLayout layout, bool inverted, bool alpha)
{
    return VKRenderDevice::WrapTexture(image, width, height, levels, format, layout, inverted, alpha);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKFactory::UpdateLayout(Texture* texture, VkImageLayout layout)
{
    VKRenderDevice::UpdateLayout(texture, layout);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* VKFactory::CreatePixelShader(RenderDevice* device_, const char* label, uint8_t shader,
    const void* spirv, uint32_t size)
{
    VKRenderDevice* device = (VKRenderDevice*)device_;
    return device->CreatePixelShader(label, shader, spirv, size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKFactory::ClearPixelShaders(RenderDevice* device_)
{
    VKRenderDevice* device = (VKRenderDevice*)device_;
    device->ClearPixelShaders();
}

NS_BEGIN_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_REGISTER_REFLECTION(Render, VKRenderDevice)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_INIT_PACKAGE(Render, VKRenderDevice)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_SHUTDOWN_PACKAGE(Render, VKRenderDevice)
{
}

NS_END_COLD_REGION
