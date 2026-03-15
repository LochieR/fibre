#pragma once

#include "Wire/Renderer/Device.h"

namespace fibre {

    class Bloom
    {
    public:
        Bloom() = default;
        Bloom(const std::shared_ptr<wire::Device>& device, const std::shared_ptr<wire::Framebuffer>& image, uint32_t width, uint32_t height);

        void apply(wire::CommandList& commandList);
    private:
        uint32_t m_Width, m_Height;
        uint32_t m_MipCount;

        std::shared_ptr<wire::Device> m_Device;
        std::shared_ptr<wire::Framebuffer> m_Input;
        std::shared_ptr<wire::Texture2D> m_InputTexture;

        std::shared_ptr<wire::Framebuffer> m_Intermediate;
        std::shared_ptr<wire::Texture2D> m_IntermediateTexture;

        std::shared_ptr<wire::Sampler> m_Sampler;
        std::shared_ptr<wire::ComputePipeline> m_DownsamplePipeline;
        std::shared_ptr<wire::ShaderResourceLayout> m_DownsampleLayout;
        std::shared_ptr<wire::ShaderResource> m_DownsampleResources;
        std::vector<std::shared_ptr<wire::ShaderResource>> m_DownsampleImageResources;
    };

}
