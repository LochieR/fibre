#include "Bloom.h"

namespace fibre {

    namespace utils {

        uint32_t getMipCount(uint32_t width, uint32_t height)
        {
            return std::floor(std::log2(std::max((float)width, (float)height)));
        }

        glm::vec2 getMipSize(uint32_t width, uint32_t height, uint32_t mipLevel)
        {
            return {
                (float)std::max(1u, width >> mipLevel),
                (float)std::max(1u, height >> mipLevel)
            };
        }

    }

    struct DownsamplePushConstants
    {
        glm::vec2 InputSize;
        glm::vec2 OutputSize;
        float Threshold;
    };

    Bloom::Bloom(const std::shared_ptr<wire::Device>& device, const std::shared_ptr<wire::Framebuffer>& image, uint32_t width, uint32_t height)
        : m_Device(device), m_Input(image), m_Width(width), m_Height(height)
    {
        m_InputTexture = m_Input->asTexture2D();

        m_MipCount = std::max(utils::getMipCount(width, height), 7u);

        wire::FramebufferDesc framebufferDesc{};
        framebufferDesc.Format = wire::AttachmentFormat::RGBA32_SFloat;
        framebufferDesc.Usage = wire::AttachmentUsage::TransferDst | wire::AttachmentUsage::TransferSrc | wire::AttachmentUsage::Storage | wire::AttachmentUsage::Sampled;
        framebufferDesc.Layout = wire::AttachmentLayout::Undefined;
        framebufferDesc.Extent = { (float)width, (float)height };
        framebufferDesc.MipCount = m_MipCount;
        framebufferDesc.HasDepth = false;

        m_Intermediate = device->createFramebuffer(framebufferDesc, "Bloom::m_Intermediate");
        m_IntermediateTexture = m_Intermediate->asTexture2D();

        wire::SamplerDesc samplerInfo{
            .MinFilter = wire::SamplerFilter::Linear,
            .MagFilter = wire::SamplerFilter::Linear,
            .AddressModeU = wire::AddressMode::ClampToEdge,
            .AddressModeV = wire::AddressMode::ClampToEdge,
            .AddressModeW = wire::AddressMode::ClampToEdge,
            .EnableAnisotropy = false,
            .MaxAnisotropy = device->getMaxAnisotropy(),
            .BorderColor = wire::BorderColor::IntOpaqueBlack,
            .MipmapMode = wire::MipmapMode::Linear,
            .CompareEnable = false,
            .MinLod = 0.0f,
            .MaxLod = 0.0f,
            .NoMaxLodClamp = false,
            .MipLodBias = 0.0f
        };

        m_Sampler = device->createSampler(samplerInfo, "Bloom::m_Sampler");

        wire::ShaderResourceLayoutInfo downsampleLayout{};
        downsampleLayout.Sets = {
            wire::ShaderResourceSetInfo{
                .Resources = {
                    wire::ShaderResourceInfo{
                        .Binding = 0,
                        .Type = wire::ShaderResourceType::Sampler,
                        .ArrayCount = 1,
                        .Stage = wire::ShaderType::Compute
                    }
                }
            },
            wire::ShaderResourceSetInfo{
                .Resources = {
                    wire::ShaderResourceInfo{
                        .Binding = 0,
                        .Type = wire::ShaderResourceType::SampledImage,
                        .ArrayCount = 1,
                        .Stage = wire::ShaderType::Compute
                    },
                    wire::ShaderResourceInfo{
                        .Binding = 1,
                        .Type = wire::ShaderResourceType::StorageImage,
                        .ArrayCount = 1,
                        .Stage = wire::ShaderType::Compute
                    }
                }
            }
        };

        m_DownsampleLayout = device->createShaderResourceLayout(downsampleLayout, "Bloom::m_DownsampleLayout");

        wire::ComputeInputLayout downsampleInputLayout{};
        downsampleInputLayout.ResourceLayout = m_DownsampleLayout;
        downsampleInputLayout.PushConstantInfos = {
            wire::PushConstantInfo{
                .Size = sizeof(DownsamplePushConstants),
                .Offset = 0,
                .Shader = wire::ShaderType::Compute
            }
        };

        wire::ComputePipelineDesc downsamplePipelineInfo{};
        downsamplePipelineInfo.Layout = downsampleInputLayout;
        downsamplePipelineInfo.ShaderPath = "shadercache://downsample.compute.hlsl";

        m_DownsamplePipeline = device->createComputePipeline(downsamplePipelineInfo, "Bloom::m_DownsamplePipeline");
        m_DownsampleResources = device->createShaderResource(0, m_DownsampleLayout, "Bloom::m_DownsampleResources");

        m_DownsampleResources->update(m_Sampler, 0, 0);

        m_DownsampleImageResources.resize(m_MipCount);

        m_DownsampleImageResources[0] = device->createShaderResource(1, m_DownsampleLayout, "Bloom::m_DownsampleOutputResources");
        m_DownsampleImageResources[0]->update(m_InputTexture, 0, 0);
        m_DownsampleImageResources[0]->update(m_Intermediate, 1, 0, 0);

        for (uint32_t i = 1; i < m_MipCount - 1; i++)
        {
            m_DownsampleImageResources[i] = device->createShaderResource(1, m_DownsampleLayout, "Bloom::m_DownsampleOutputResources");
            m_DownsampleImageResources[i]->update(m_IntermediateTexture, 0, 0, i - 1);
            m_DownsampleImageResources[i]->update(m_Intermediate, 1, 0, i);
        }
    }

    void Bloom::apply(wire::CommandList& commandList)
    {
        constexpr uint32_t localSizeX = 16, localSizeY = 16;
        uint32_t groupCountX = (m_Width + localSizeX - 1) / localSizeX;
        uint32_t groupCountY = (m_Height + localSizeY - 1) / localSizeY;

        commandList.bindPipeline(m_DownsamplePipeline);
        commandList.bindShaderResource(0, m_DownsampleResources);

        commandList.imageMemoryBarrier(m_Intermediate, wire::AttachmentLayout::Undefined, wire::AttachmentLayout::General, 0, m_MipCount);
        commandList.clearImage(m_Intermediate, { 0.0f, 0.0f, 0.0f, 0.0f }, wire::AttachmentLayout::General, 0, m_MipCount);

        glm::vec2 lastMipSize = { (float)m_Width, (float)m_Height };
        for (uint32_t i = 0; i < m_MipCount - 1; i++)
        {
            DownsamplePushConstants pushConstants{
                .InputSize = lastMipSize,
                .OutputSize = utils::getMipSize(m_Width, m_Height, i),
                .Threshold = 1.0f
            };

            commandList.pushConstants(wire::ShaderType::Compute, pushConstants);
            commandList.bindShaderResource(1, m_DownsampleImageResources[i]);
            
            commandList.dispatch(groupCountX, groupCountY, 1);
            commandList.imageMemoryBarrier(m_Intermediate, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly, i, 1);

            lastMipSize = utils::getMipSize(m_Width, m_Height, i);
        }

        commandList.imageMemoryBarrier(m_Intermediate, wire::AttachmentLayout::ShaderReadOnly, wire::AttachmentLayout::General, 0, m_MipCount - 1);
    }

}
