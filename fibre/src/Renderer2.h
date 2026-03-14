#pragma once

#include "Ray.h"
#include "Scene.h"
#include "Camera.h"
#include "Bloom.h"

#include "Wire/Renderer/Device.h"

#include <memory>

namespace fibre {

    class Renderer2
    {
    public:
        struct Settings
        {
            bool Accumulate = true;
        };
    public:
        Renderer2() = default;
        Renderer2(const std::shared_ptr<wire::Device>& device);

        void render(const Scene& scene, const Camera& camera);

        std::shared_ptr<wire::Framebuffer> getRawFinalImage() const { return m_FinalImage; }
        std::shared_ptr<wire::Texture2D> getFinalImage() const { return m_FinalImageTexture; }
        std::shared_ptr<wire::Texture2D> getAccumulatedImage() const { return m_AccumulatedImageTexture; }
        glm::vec2 getImageSize() const { return { (float)m_ViewportWidth, (float)m_ViewportHeight }; };

        void resetFrameIndex() { m_FrameIndex = 1; }
        Settings& getSettings() { return m_Settings; }
    private:
        uint32_t m_ViewportWidth = 600, m_ViewportHeight = 600;

        uint32_t m_FrameIndex = 1;
        Settings m_Settings;

        Bloom m_Bloom;

        std::shared_ptr<wire::Device> m_Device;

        std::shared_ptr<wire::ShaderResourceLayout> m_PipelineLayout;
        std::shared_ptr<wire::ComputePipeline> m_Pipeline;
        std::shared_ptr<wire::ShaderResource> m_PipelineResource;
        std::shared_ptr<wire::Framebuffer> m_AccumulatedImage;
        std::shared_ptr<wire::Framebuffer> m_FinalImage;

        std::shared_ptr<wire::Buffer> m_CameraBuffer;
        std::shared_ptr<wire::Buffer> m_SceneBuffer;

        std::shared_ptr<wire::Texture2D> m_FinalImageTexture;
        std::shared_ptr<wire::Texture2D> m_AccumulatedImageTexture;

        wire::CommandList m_CommandList;
    };

}
